#include "PX_Syntax_while.h"
#include "PX_Syntax_base_operate.h"


PX_SYNTAX_FUNCTION(PX_Syntax_while_begin)
{
	px_abi* plast_while_block;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	if (PX_NULL == (plast_while_block = PX_Syntax_NewAbi(pSyntax, "while_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_begin: create while_block abi failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_while_block, "label_name", PX_Syntax_AllocUnnamed(pSyntax, "__while_")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_EnterScope(pSyntax))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_while_condition_expr_merge)
{
	px_abi* plast_while_block = PX_Syntax_GetThirdLastAbi(pSyntax);
	px_abi* pwhile_scope_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_abi* pexpr_abi = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pexpr_ir;

	PX_ASSERTIFX(!plast_while_block, "PX_Syntax_while_condition_expr_merge: no while_block abi found.");
	PX_ASSERTIFX(!pwhile_scope_abi, "PX_Syntax_while_condition_expr_merge: no while_scope abi found.");
	PX_ASSERTIFX(!pexpr_abi, "PX_Syntax_while_condition_expr_merge: no expr abi found.");

	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pexpr_abi, "expr"), "PX_Syntax_while_condition_expr_merge: pexpr_abi is not expr");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pwhile_scope_abi, "scope"), "PX_Syntax_while_condition_expr_merge: pwhile_scope_abi is not scope");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_while_block, "while_block"), "PX_Syntax_while_condition_expr_merge: plast_while_block not while_block");

	//merge to while_block
	pexpr_ir = PX_AbiGetValue_string(pexpr_abi, "ir");
	if (!PX_AbiAppend_string(plast_while_block, "condition_ir", pexpr_ir ? pexpr_ir : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_condition_expr_merge: append expr ir to while_block failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop expr
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_while_merge_body_end)
{
	//label_continue:
	//condition ir
	//jz r0,label_break
	//body ir
	//jmp label_continue
	//label_break:
	px_abi* plast_while_block;
	px_abi* plast_while_scope_abi;
	px_abi* plast_scope_block_abi;
	px_char label_payload[64] = { 0 };
	const px_char* plabel_name, * pcondition_ir, * pbody_ir;
	px_string build_final_ir;

	plast_scope_block_abi = PX_Syntax_GetLastAbi(pSyntax);
	plast_while_scope_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	plast_while_block = PX_Syntax_GetThirdLastAbi(pSyntax);

	PX_ASSERTIFX(!plast_scope_block_abi||!PX_AbiExist_Type(plast_scope_block_abi, "ir",PX_ABI_TYPE_STRING), "PX_Syntax_while_merge_body_end cannot find scope_block abi.");
	PX_ASSERTIFX(!plast_while_scope_abi || !PX_Syntax_CheckAbiName(plast_while_scope_abi, "scope"), "PX_Syntax_while_merge_body_end cannot find while scope abi.");
	PX_ASSERTIFX(!plast_while_block || !PX_Syntax_CheckAbiName(plast_while_block, "while_block"), "PX_Syntax_while_merge_body_end cannot find while_block abi.");

	//scope_block
	//scope
	//while_block

	if(!PX_Syntax_LeaveScope(pSyntax))//leave while_scope
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: leave while_scope failed.");
		return PX_FALSE;
	}

	//scope_block
	//while_block

	//merge body ir from scope_block (or block) into while_block
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "while_block", "ir", "body_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: merge body_ir failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "while_block", "ir_library", "ir_library"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: merge body_ir_library failed.");
		return PX_FALSE;
	}

	PX_Syntax_PopAbi(pSyntax);//pop scope_block

	pcondition_ir = PX_AbiGetValue_string(plast_while_block, "condition_ir");
	pbody_ir = PX_AbiGetValue_string(plast_while_block, "body_ir");
	plabel_name = PX_AbiGetValue_string(plast_while_block, "label_name");

	if (!PX_StringInitialize(pSyntax->mp, &build_final_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: initialize final ir failed.");
		return PX_FALSE;
	}
	//label_continue:
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_continue:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//condition ir
	if (!PX_StringCat(&build_final_ir, pcondition_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//jz r0,label_break
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), "jz r0,");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//body ir
	if (!PX_StringCat(&build_final_ir, pbody_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//jmp label_continue
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), "jmp ");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_continue\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//label_break:
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: build final ir failed.");
		goto _ERROR;
	}

	if (!PX_AbiAppend_string(plast_while_block, "ir", PX_StringGetText(&build_final_ir)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_while_merge_body_end: append final ir to scope failed.");
		goto _ERROR;
	}
	PX_StringFree(&build_final_ir);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&build_final_ir);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_while_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_while_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_while(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_content = block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_content = scope_block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while'", 0, PX_Syntax_while_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '('", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' *", 0, PX_Syntax_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' expr", 0, PX_Syntax_while_condition_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' *", 0, PX_Syntax_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' expr ')'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' expr *", 0, PX_Syntax_while_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' expr ')' while_content", 0, PX_Syntax_while_merge_body_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "while_block = 'while' '(' expr ')' *", 0, PX_Syntax_while_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
