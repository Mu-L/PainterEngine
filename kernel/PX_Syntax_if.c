#include "PX_Syntax_if.h"
#include "PX_Syntax_base_operate.h"


PX_SYNTAX_FUNCTION(PX_Syntax_if_begin)
{
	px_abi* plast_if_block;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	if (PX_NULL==(plast_if_block=PX_Syntax_NewAbi(pSyntax,"if_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_begin: create if_block abi failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_if_block, "label_name", PX_Syntax_AllocUnnamed(pSyntax,"__if_")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_EnterScope(pSyntax)) 
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_begin: enter scope failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_if_condition_expr_merge)
{
	//expr
	//if_block

	px_abi* plast_expr_abi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* plast_scope = PX_Syntax_GetSecondLastAbi(pSyntax);
	const px_char* pexpr_ir;

	PX_ASSERTIFX(!plast_expr_abi, "PX_Syntax_if_condition_expr_merge: no expr abi found.");
	PX_ASSERTIFX(!plast_scope, "PX_Syntax_if_condition_expr_merge: no scope abi found.");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_expr_abi, "expr"), "PX_Syntax_if_condition_expr_merge: no expr abi found.");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_scope, "scope"), "PX_Syntax_if_condition_expr_merge: no scope abi found.");

	//merge to if_block
	pexpr_ir = PX_AbiGet_string(plast_expr_abi, "ir");
	if (!PX_AbiAppend_string(plast_scope, "condition_ir", pexpr_ir ? pexpr_ir : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: append expr ir to if_block failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop expr
	
	return PX_TRUE;
}

px_bool PX_Syntax_if_block_merge(PX_Syntax* pSyntax,px_bool is_true)
{
	px_abi* plast_abi = PX_Syntax_GetLastAbi(pSyntax);//<----scope_block or contain ir abi;
	px_abi* plast_scope_abi = PX_Syntax_GetSecondLastAbi(pSyntax);//<----scope abi;
	const px_char* ir_content,* ir_library_content;
	const px_char *merge_ir;
	const px_char *merge_ir_library;
	PX_ASSERTIFX(!plast_abi, "PX_Syntax_if_block_merge: no last abi found.");
	PX_ASSERTIFX(!plast_scope_abi, "PX_Syntax_if_block_merge: no scope abi found.");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_scope_abi, "scope"), "PX_Syntax_if_block_merge: no scope abi found.");

	if (!plast_abi)
	{
		PX_ASSERTX("PX_Syntax_if_block_merge cannot find scope abi.");
		return PX_FALSE;
	}
	merge_ir = is_true ? "true_ir" : "false_ir";
	merge_ir_library = is_true ? "true_ir_library" : "false_ir_library";
	ir_content = PX_AbiGetValue_string(plast_abi, "ir");
	ir_library_content = PX_AbiGet_string(plast_abi, "ir_library");

	if (!PX_AbiSet_string(plast_scope_abi, merge_ir, ir_content ? ir_content : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_merge_true set true_ir failed.");
		return PX_FALSE;
	}
	if (ir_library_content)
	{
		if (!PX_AbiSet_string(plast_scope_abi, merge_ir_library, ir_library_content))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_merge_true set true_ir_library failed.");
			return PX_FALSE;
		}
	}
	PX_Syntax_PopAbi(pSyntax);//pop scope_block or contain ir abi;
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_if_merge_true)
{
	return PX_Syntax_if_block_merge(pSyntax, PX_TRUE);
}



PX_SYNTAX_FUNCTION(PX_Syntax_if_end)
{
	//condition ir
	//jz r0,label_else
	//true ir
	//jmp label_end
	//label_else:
	//false ir
	//label_end:
	
	px_abi* plast_if_block = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_abi* plast_scope_abi = PX_Syntax_GetLastAbi(pSyntax);
	px_char pif_label_payload[64] = { 0 };
	const px_char* pif_label_name,*pcondition_ir,*ptrue_ir,*pfalse_ir;
	const px_char* ptrue_ir_library;
	const px_char* pfalse_ir_library;
	px_string build_final_ir;
	
	PX_ASSERTIFX(!plast_scope_abi || !PX_Syntax_CheckAbiName(plast_scope_abi, "scope"), "PX_Syntax_if_block_merge cannot find scope abi.");
	PX_ASSERTIFX(!plast_if_block||!PX_Syntax_CheckAbiName(plast_if_block, "if_block"), "PX_Syntax_if_condition_expr_merge: last abi is not if_block");

	//leave scope balance sp
	if (!PX_Syntax_LeaveScope(pSyntax))
	{
		PX_ASSERTX("PX_Syntax_if_merge_true leave scope failed.");
		return PX_FALSE;
	}

	if(!PX_StringInitialize(pSyntax->mp, &build_final_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}

	pcondition_ir = PX_AbiGetValue_string(plast_scope_abi, "condition_ir");
	ptrue_ir = PX_AbiGetValue_string(plast_scope_abi, "true_ir");
	pfalse_ir = PX_AbiGet_string(plast_scope_abi, "false_ir");

	PX_ASSERTIFX(!pcondition_ir, "PX_Syntax_if_condition_expr_merge: if_block has no condition_ir");
	PX_ASSERTIFX(!ptrue_ir, "PX_Syntax_if_condition_expr_merge: if_block has no true_ir");
	//condition ir
	if (!PX_StringCat(&build_final_ir, pcondition_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}
	//jz r0,label_else
	pif_label_name = PX_AbiGet_string(plast_if_block, "label_name");
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "jz r0,");
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), pif_label_name);
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "_else\n");
	if (!PX_StringCat(&build_final_ir, pif_label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}
	//true ir
	if (!PX_StringCat(&build_final_ir, ptrue_ir ))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}
	//jmp label_end
	pif_label_payload[0] = '\0';
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "jmp ");
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), pif_label_name);
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "_end\n");
	if (!PX_StringCat(&build_final_ir, pif_label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}
	//label_else:
	pif_label_payload[0] = '\0';
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), pif_label_name);
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "_else:\n");
	if (!PX_StringCat(&build_final_ir, pif_label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}

	//false ir
	if (pfalse_ir)
	{
		if (!PX_StringCat(&build_final_ir, pfalse_ir))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
			goto _ERROR;
		}
	}

	//label_end:
	pif_label_payload[0] = '\0';
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), pif_label_name);
	PX_strcat_s(pif_label_payload, sizeof(pif_label_payload), "_end:\n");

	if (!PX_StringCat(&build_final_ir, pif_label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: build final ir failed.");
		goto _ERROR;
	}

	if (!PX_AbiAppend_string(plast_if_block, "ir", PX_StringGetText(&build_final_ir)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_condition_expr_merge: append final ir to scope failed.");
		goto _ERROR;
	}
	PX_StringFree(&build_final_ir);

	//propagate ir_library from true/false branches to parent scope
	ptrue_ir_library = PX_AbiGet_string(plast_scope_abi, "true_ir_library");
	pfalse_ir_library = PX_AbiGet_string(plast_scope_abi, "false_ir_library");
	if (ptrue_ir_library)
	{
		if (!PX_AbiAppend_string(plast_if_block, "ir_library", ptrue_ir_library))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_end: append true_ir_library to scope failed.");
			return PX_FALSE;
		}
	}
	if (pfalse_ir_library)
	{
		if (!PX_AbiAppend_string(plast_if_block, "ir_library", pfalse_ir_library))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_if_end: append false_ir_library to scope failed.");
			return PX_FALSE;
		}
	}
	PX_Syntax_PopAbi(pSyntax);//pop scope
	return PX_TRUE;
_ERROR:
	PX_StringFree(&build_final_ir);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_if_merge_false_end)
{
	return PX_Syntax_if_block_merge(pSyntax, PX_FALSE);
}

PX_SYNTAX_FUNCTION(PX_Syntax_if_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_if_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_if(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_content = block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_content = scope_block", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block", 0, PX_Syntax_if_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if'", 0, PX_Syntax_if_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '('", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' *", 0, PX_Syntax_if_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr", 0, PX_Syntax_if_condition_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' *", 0, PX_Syntax_if_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr *", 0, PX_Syntax_if_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' if_content ", 0, PX_Syntax_if_merge_true, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' * ", 0, PX_Syntax_if_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' if_content 'else' ", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' if_content * ", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' if_content 'else' if_content", 0, PX_Syntax_if_merge_false_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "if_block = 'if' '(' expr ')' if_content 'else' *", 0, PX_Syntax_if_error, 0))
		return PX_FALSE;


	return PX_TRUE;
}
