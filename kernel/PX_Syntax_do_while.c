#include "PX_Syntax_do_while.h"
#include "PX_Syntax_base_operate.h"


PX_SYNTAX_FUNCTION(PX_Syntax_do_while_begin)
{
	px_abi* plast_do_while_block;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRenderScope);

	if (PX_NULL == (plast_do_while_block = PX_Syntax_NewAbi(pSyntax, "do_while_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_begin: create do_while_block abi failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_do_while_block, "label_name", PX_Syntax_AllocUnnamed(pSyntax, "__dowhile_")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_EnterScope(pSyntax))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_do_while_merge_body)
{
	px_abi* plast_do_while_block = PX_Syntax_GetAbiFromBackward(pSyntax, "do_while_block");
	PX_ASSERTIFX(!plast_do_while_block, "PX_Syntax_do_while_merge_body cannot find do_while_block abi.");

	//merge body ir from scope_block (or block) into do_while_block
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "do_while_block", "ir", "body_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_merge_body: merge body_ir failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "do_while_block", "ir_library", "body_ir_library"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_merge_body: merge body_ir_library failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_LeaveScope(pSyntax))
	{
		PX_ASSERTX("PX_Syntax_do_while_merge_body leave scope failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_do_while_condition_expr_merge)
{
	px_abi* plast_expr_abi;
	px_abi* plast_do_while_block = PX_Syntax_GetAbiFromBackward(pSyntax, "do_while_block");
	px_abi* plast_abi_operand = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pexpr_ir;

	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_abi_operand, "operand"), "PX_Syntax_do_while_condition_expr_merge: last abi is not operand");
	PX_ASSERTIFX(!plast_do_while_block, "PX_Syntax_do_while_condition_expr_merge: no do_while_block abi found.");
	plast_expr_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(!plast_expr_abi, "PX_Syntax_do_while_condition_expr_merge: no expr abi found.");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_expr_abi, "expr"), "PX_Syntax_do_while_condition_expr_merge: no expr abi found.");
	//merge to do_while_block
	pexpr_ir = PX_AbiGet_string(plast_expr_abi, "ir");
	if (!PX_AbiAppend_string(plast_do_while_block, "condition_ir", pexpr_ir ? pexpr_ir : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_condition_expr_merge: append expr ir to do_while_block failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop operand
	PX_Syntax_PopAbi(pSyntax);//pop expr

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_do_while_end)
{
	//label_begin:
	//body ir
	//label_continue:
	//condition ir
	//jnz r0,label_begin
	//label_break:
	px_abi* plast_scope_abi;
	px_abi* plast_do_while_block = PX_Syntax_GetAbiFromBackward(pSyntax, "do_while_block");
	px_char label_payload[64] = { 0 };
	const px_char* plabel_name, * pcondition_ir, * pbody_ir;
	const px_char* pbody_ir_library;
	px_string build_final_ir;

	PX_ASSERTIFX(!plast_do_while_block, "PX_Syntax_do_while_end: cannot find do_while_block abi.");
	plast_scope_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "scope");
	PX_ASSERTIFX(!plast_scope_abi, "PX_Syntax_do_while_end cannot find scope abi.");
	pcondition_ir = PX_AbiGetValue_string(plast_do_while_block, "condition_ir");
	pbody_ir = PX_AbiGetValue_string(plast_do_while_block, "body_ir");
	plabel_name = PX_AbiGetValue_string(plast_do_while_block, "label_name");

	if (!PX_StringInitialize(pSyntax->mp, &build_final_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//label_begin:
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_begin:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//body ir
	if (!PX_StringCat(&build_final_ir, pbody_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//label_continue:
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_continue:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//condition ir
	if (!PX_StringCat(&build_final_ir, pcondition_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//jnz r0,label_begin
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), "jnz r0,");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_begin\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}
	//label_break:
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: build final ir failed.");
		goto _ERROR;
	}

	if (!PX_AbiAppend_string(plast_scope_abi, "ir", PX_StringGetText(&build_final_ir)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: append final ir to scope failed.");
		goto _ERROR;
	}
	PX_StringFree(&build_final_ir);
	//propagate ir_library from body to parent scope
	pbody_ir_library = PX_AbiGet_string(plast_do_while_block, "body_ir_library");
	if (pbody_ir_library)
	{
		if (!PX_AbiAppend_string(plast_scope_abi, "ir_library", pbody_ir_library))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_do_while_end: append body_ir_library to scope failed.");
			return PX_FALSE;
		}
	}
	//pop do_while_block abi
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&build_final_ir);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_do_while_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_do_while_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_do_while(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_content = block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_content = scope_block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do'", 0, PX_Syntax_do_while_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content", 0, PX_Syntax_do_while_merge_body, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '('", 0, PX_Syntax_TokenRenderScope, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' expr", 0, PX_Syntax_do_while_condition_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' expr ')'", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' expr *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' expr ')' ';'", 0, PX_Syntax_do_while_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "do_while_block = 'do' do_while_content 'while' '(' expr ')' *", 0, PX_Syntax_do_while_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
