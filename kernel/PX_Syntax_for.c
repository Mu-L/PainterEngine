#include "PX_Syntax_for.h"
#include "PX_Syntax_base_operate.h"


PX_SYNTAX_FUNCTION(PX_Syntax_for_begin)
{
	px_abi* plast_for_block;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	if (PX_NULL == (plast_for_block = PX_Syntax_NewAbi(pSyntax, "for_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_begin: create for_block abi failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_for_block, "label_name", PX_Syntax_AllocUnnamed(pSyntax, "__for_")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_EnterScope(pSyntax))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_for_init_expr_merge)
{
	if(!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax,"for_block","ir", "init_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_init_expr_merge: merge init_ir failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopLastAbiName(pSyntax, "expr");
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_for_condition_expr_merge)
{
	PX_Syntax_ResetIRLocation(pSyntax);
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "for_block", "ir", "condition_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_condition_expr_merge: merge condition_ir failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopLastAbiName(pSyntax, "expr");
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_for_step_expr_merge)
{
	PX_Syntax_ResetIRLocation(pSyntax);
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "for_block", "ir", "step_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_step_expr_merge: merge step_ir failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopLastAbiName(pSyntax, "expr");
	return PX_TRUE;
}




PX_SYNTAX_FUNCTION(PX_Syntax_for_merge_body_end)
{
	//init ir
	//label_begin:
	//condition ir
	//jz r0,label_break
	//body ir
	//label_continue:
	//step ir
	//jmp label_begin
	//label_break:
	px_abi* plast_for_block;
	px_abi* plast_scope_block;
	px_abi* plast_scope;

	px_char label_payload[64] = { 0 };
	const px_char* plabel_name, * pinit_ir, * pcondition_ir, * pstep_ir, * pbody_ir;
	px_string build_final_ir;
	//merge body scope ir to for_block
	plast_scope_block = PX_Syntax_GetLastAbi(pSyntax);
	plast_scope = PX_Syntax_GetSecondLastAbi(pSyntax);
	plast_for_block = PX_Syntax_GetThirdLastAbi(pSyntax);

	PX_ASSERTIFX(!plast_scope_block || !PX_AbiExist_Type(plast_scope_block, "ir", PX_ABI_TYPE_STRING), "PX_Syntax_for_merge_body_end cannot find scope_block abi.");
	PX_ASSERTIFX(!plast_scope || !PX_Syntax_CheckAbiName(plast_scope, "scope"), "PX_Syntax_for_merge_body_end cannot find scope abi.");
	PX_ASSERTIFX(!plast_for_block || !PX_Syntax_CheckAbiName(plast_for_block, "for_block"), "PX_Syntax_for_merge_body_end cannot find for_block abi.");
	
	//scope_block
	//scope
	//for_block

	if(!PX_Syntax_LeaveScope(pSyntax))//leave for_scope
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: leave for_scope failed.");
		return PX_FALSE;
	}	

	//scope_block
	//for_block

	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "for_block", "ir", "body_ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_body_merge: merge body_ir failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "for_block", "ir_library", "ir_library"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_body_merge: merge body_ir failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop scope_block

	pinit_ir = PX_AbiGetValue_string(plast_for_block, "init_ir");
	pcondition_ir = PX_AbiGetValue_string(plast_for_block, "condition_ir");
	pstep_ir = PX_AbiGetValue_string(plast_for_block, "step_ir");
	pbody_ir = PX_AbiGetValue_string(plast_for_block, "body_ir");
	plabel_name = PX_AbiGetValue_string(plast_for_block, "label_name");

	if (!PX_StringInitialize(pSyntax->mp, &build_final_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		return PX_FALSE;
	}
	//init ir
	if (!PX_StringCat(&build_final_ir, pinit_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//label_begin: just label mark
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_begin:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//condition ir
	if (!PX_StringCat(&build_final_ir, pcondition_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//jz r0,label_break
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), "jz r0,");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//body ir
	if (!PX_StringCat(&build_final_ir, pbody_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//label_continue:
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_continue:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//step ir
	if (!PX_StringCat(&build_final_ir, pstep_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//jmp label_begin
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), "jmp ");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_begin\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	//label_break:
	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: build final ir failed.");
		goto _ERROR;
	}
	
	if (!PX_AbiAppend_string(plast_for_block, "ir", PX_StringGetText(&build_final_ir)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_merge_body_end: append final ir to scope failed.");
		goto _ERROR;
	}

	PX_StringFree(&build_final_ir);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&build_final_ir);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_for_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_for_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_for(PX_Syntax* pSyntax)
{

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_content = block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_content = scope_block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_expr_segment = expr ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_expr_segment = ';'", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_expr_segment = *", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for'", 0, PX_Syntax_for_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '('", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' *", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment", 0, PX_Syntax_for_init_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' *", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment", 0, PX_Syntax_for_condition_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment *", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment expr", 0, PX_Syntax_for_step_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment *", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment expr ')'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment expr ')' for_content", 0, PX_Syntax_for_merge_body_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "for_block = 'for' '(' for_expr_segment for_expr_segment expr ')' *", 0, PX_Syntax_for_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
