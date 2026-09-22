#include "PX_Syntax_switch.h"
#include "PX_Syntax_base_operate.h"

PX_SYNTAX_FUNCTION(PX_Syntax_switch_begin)
{
	px_abi* plast_switch_block;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	if (PX_NULL == (plast_switch_block = PX_Syntax_NewAbi(pSyntax, "switch_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_begin: create switch_block abi failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_switch_block, "label_name", PX_Syntax_AllocUnnamed(pSyntax, "__switch_")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_AbiSet_bool(plast_switch_block, "has_default", PX_FALSE))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_begin out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_EnterScope(pSyntax))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_scope_block_merge)
{
	//scope_block ir is merged back into the enclosing switch scope
	px_abi* plast_ir_block = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* plast_scope_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_abi* plast_case_block = PX_Syntax_GetThirdLastAbi(pSyntax);
	const px_char* pir;
	const px_char* pir_library;

	PX_ASSERTIFX(!plast_ir_block || !PX_AbiExist_Type(plast_ir_block, "ir",PX_ABI_TYPE_STRING),"PX_Syntax_switch_scope_block_merge: last abi is not scope_block");
	PX_ASSERTIFX(!plast_scope_abi || !PX_Syntax_CheckAbiName(plast_scope_abi, "scope"), "PX_Syntax_switch_scope_block_merge: no scope abi found.");

	pir = PX_AbiGet_string(plast_ir_block, "ir");
	if (pir && !PX_AbiAppend_string(plast_scope_abi, "ir", pir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_scope_block_merge: merge scope ir failed.");
		return PX_FALSE;
	}
	pir_library = PX_AbiGet_string(plast_ir_block, "ir_library");
	if (pir_library && !PX_AbiAppend_string(plast_scope_abi, "ir_library", pir_library))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_scope_block_merge: merge scope ir_library failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop ir_block
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_condition_expr_merge)
{
	px_abi* plast_expr_abi;
	px_abi* plast_switch_block;
	px_abi* plast_scope;
	const px_char* poperand_type;
	const px_char* pexpr_ir;

	plast_expr_abi = PX_Syntax_GetLastAbi(pSyntax);
	plast_scope = PX_Syntax_GetSecondLastAbi(pSyntax);
	plast_switch_block = PX_Syntax_GetThirdLastAbi(pSyntax);
	PX_ASSERTIFX(!plast_switch_block || !PX_Syntax_CheckAbiName(plast_switch_block, "switch_block"), "PX_Syntax_switch_condition_expr_merge cannot find switch_block abi.");
	PX_ASSERTIFX(!plast_scope || !PX_Syntax_CheckAbiName(plast_scope, "scope"), "PX_Syntax_switch_condition_expr_merge cannot find scope abi.");
	PX_ASSERTIFX(!plast_expr_abi || !PX_Syntax_CheckAbiName(plast_expr_abi, "expr"), "PX_Syntax_switch_condition_expr_merge cannot find expr abi.");
	poperand_type = PX_AbiGet_string(plast_expr_abi, "type");

	if (!poperand_type || !PX_Syntax_TypeMatch(poperand_type, "ix"))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:switch condition must have integer type");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRLocation(pSyntax, "switch_block"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_condition_expr_merge: build final ir failed.");
		return PX_FALSE;
	}

	pexpr_ir = PX_AbiGet_string(plast_expr_abi, "ir");
	if (!PX_AbiAppend_string(plast_switch_block, "condition_ir", pexpr_ir ? pexpr_ir : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_condition_expr_merge: append expr ir to switch_block failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop expr
	return PX_TRUE;
}

static px_bool PX_Syntax_switch_case_value_exists(px_abi* pswitch_block, const px_char value[], px_int case_count)
{
	px_int i;
	for (i = 0; i < case_count; i++)
	{
		px_char payload[32] = { 0 };
		const px_char* pexist_value;
		PX_sprintf1(payload, sizeof(payload), "case_values.%1", PX_STRINGFORMAT_INT(i));
		pexist_value = PX_AbiGet_string(pswitch_block, payload);
		if (pexist_value && PX_strequ(pexist_value, value))
		{
			return PX_TRUE;
		}
	}
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_case)
{
	// dispatch_ir += mov r1,<case value> / eq r1,r0 / jnz r1,<label>_case_<n>
	// r0 carries the switch value across the whole dispatch chain, so the
	// comparison result must land in r1 instead of clobbering it.
	// scope ir    += <label>_case_<n>:
	px_abi* plast_const_abi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* plast_switch_block = PX_Syntax_GetAbiFromBackward(pSyntax, "switch_block");
	px_abi* plast_scope_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "scope");
	px_char payload[128] = { 0 };
	px_char case_label[80] = { 0 };
	px_char case_value_payload[32] = { 0 };
	const px_char* pcase_value;
	const px_char* plabel_name;
	px_int case_count;

	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	PX_ASSERTIFX(!plast_const_abi || !PX_Syntax_CheckAbiName(plast_const_abi, "const_int"), "PX_Syntax_switch_case: last abi is not const_int");
	PX_ASSERTIFX(!plast_switch_block, "PX_Syntax_switch_case: no switch_block abi found.");
	PX_ASSERTIFX(!plast_scope_abi, "PX_Syntax_switch_case: no scope abi found.");

	pcase_value = PX_AbiGetValue_string(plast_const_abi, "value");
	plabel_name = PX_AbiGetValue_string(plast_switch_block, "label_name");
	case_count = PX_AbiGet_PayloadMemberCount(plast_switch_block, "case_values");

	if (PX_Syntax_switch_case_value_exists(plast_switch_block, pcase_value, case_count))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:duplicated switch case value");
		return PX_FALSE;
	}
	PX_sprintf1(case_value_payload, sizeof(case_value_payload), "case_values.%1", PX_STRINGFORMAT_INT(case_count));
	if (!PX_AbiSet_string(plast_switch_block, case_value_payload, pcase_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_case: store case value failed.");
		return PX_FALSE;
	}

	PX_strcat_s(case_label, sizeof(case_label), plabel_name);
	PX_strcat_s(case_label, sizeof(case_label), "_case_");
	PX_strcat_s(case_label, sizeof(case_label), PX_itos(case_count, 10).data);

	PX_strcat_s(payload, sizeof(payload), "mov r1,");
	PX_strcat_s(payload, sizeof(payload), pcase_value);
	PX_strcat_s(payload, sizeof(payload), "\neq r1,r0\njnz r1,");
	PX_strcat_s(payload, sizeof(payload), case_label);
	PX_strcat_s(payload, sizeof(payload), "\n");
	if (!PX_AbiAppend_string(plast_switch_block, "dispatch_ir", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_case: append dispatch ir failed.");
		return PX_FALSE;
	}

	payload[0] = '\0';
	PX_strcat_s(payload, sizeof(payload), case_label);
	PX_strcat_s(payload, sizeof(payload), ":\n");
	if (!PX_AbiAppend_string(plast_scope_abi, "ir", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_case: append case label to scope ir failed.");
		return PX_FALSE;
	}

	PX_Syntax_PopAbi(pSyntax);//pop const_int
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_default)
{
	px_abi* plast_switch_block = PX_Syntax_GetAbiFromBackward(pSyntax, "switch_block");
	px_abi* plast_scope_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "scope");
	px_char payload[96] = { 0 };
	const px_char* plabel_name;

	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	PX_ASSERTIFX(!plast_switch_block, "PX_Syntax_switch_default: no switch_block abi found.");
	PX_ASSERTIFX(!plast_scope_abi, "PX_Syntax_switch_default: no scope abi found.");

	if (PX_AbiGetValue_bool(plast_switch_block, "has_default"))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:duplicated switch default label");
		return PX_FALSE;
	}

	plabel_name = PX_AbiGetValue_string(plast_switch_block, "label_name");
	PX_strcat_s(payload, sizeof(payload), plabel_name);
	PX_strcat_s(payload, sizeof(payload), "_default:\n");
	if (!PX_AbiAppend_string(plast_scope_abi, "ir", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_default: append default label to scope ir failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_bool(plast_switch_block, "has_default", PX_TRUE))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_default: update has_default failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_end)
{
	// condition ir
	// dispatch ir (mov r1,<value> / eq r1,r0 / jnz r1,<case label> ...)
	// jmp <default label> or <break label>
	// body ir (labels are embedded, so fall-through is natural)
	// break label
	px_abi* plast_scope_abi;
	px_abi* plast_switch_block;
	px_char label_payload[64] = { 0 };
	const px_char* plabel_name, * pcondition_ir, * pdispatch_ir, * pbody_ir;
	const px_char* pbody_ir_library;
	px_bool has_default;
	px_string build_final_ir;

	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	// Save the body before freeing the switch-local scope.
	plast_scope_abi = PX_Syntax_GetLastAbi(pSyntax);
	plast_switch_block = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(!plast_scope_abi||!PX_Syntax_CheckAbiName(plast_scope_abi,"scope"), "PX_Syntax_switch_end cannot find scope abi.");
	PX_ASSERTIFX(!plast_switch_block || !PX_Syntax_CheckAbiName(plast_switch_block, "switch_block"), "PX_Syntax_switch_end cannot find switch_block abi.");
	if (!PX_Syntax_LeaveScope(pSyntax))
	{
		PX_ASSERTX("PX_Syntax_switch_end leave scope failed.");
		return PX_FALSE;
	}
	//scope_block
	//switch_block
	plast_scope_abi= PX_Syntax_GetLastAbi(pSyntax);

	pbody_ir = PX_AbiGet_string(plast_scope_abi, "ir");
	if (!PX_AbiSet_string(plast_switch_block, "body_ir", pbody_ir ? pbody_ir : ""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end set body_ir failed.");
		return PX_FALSE;
	}
	pbody_ir_library = PX_AbiGet_string(plast_scope_abi, "ir_library");
	if (pbody_ir_library)
	{
		if (!PX_AbiSet_string(plast_switch_block, "ir_library", pbody_ir_library))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end set ir_library failed.");
			return PX_FALSE;
		}
	}

	pcondition_ir = PX_AbiGetValue_string(plast_switch_block, "condition_ir");
	pdispatch_ir = PX_AbiGet_string(plast_switch_block, "dispatch_ir");
	plabel_name = PX_AbiGetValue_string(plast_switch_block, "label_name");
	has_default = PX_AbiGetValue_bool(plast_switch_block, "has_default");

	if(!PX_StringInitialize(pSyntax->mp, &build_final_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: initialize final ir failed.");
		return PX_FALSE;
	}	
	if (!PX_StringCat(&build_final_ir, pcondition_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: build final ir failed.");
		goto _ERROR;
	}
	if (pdispatch_ir && !PX_StringCat(&build_final_ir, pdispatch_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: build final ir failed.");
		goto _ERROR;
	}

	PX_strcat_s(label_payload, sizeof(label_payload), "jmp ");
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), has_default ? "_default\n" : "_break\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: build final ir failed.");
		goto _ERROR;
	}
	if (!PX_StringCat(&build_final_ir, pbody_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: build final ir failed.");
		goto _ERROR;
	}

	label_payload[0] = '\0';
	PX_strcat_s(label_payload, sizeof(label_payload), plabel_name);
	PX_strcat_s(label_payload, sizeof(label_payload), "_break:\n");
	if (!PX_StringCat(&build_final_ir, label_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: build final ir failed.");
		goto _ERROR;
	}

	if (!PX_AbiAppend_string(plast_switch_block, "ir", PX_StringGetText(&build_final_ir)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_switch_end: append final ir to scope failed.");
		goto _ERROR;
	}
	PX_StringFree(&build_final_ir);
	PX_Syntax_PopAbi(pSyntax);//pop switch_block
	return PX_TRUE;

_ERROR:
	PX_StringFree(&build_final_ir);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_switch_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_switch_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_switch(PX_Syntax* pSyntax)
{
	// case/default labels are parsed separately from ordinary statements.
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'case'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'case' const_int ':'", 0, PX_Syntax_switch_case, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'case' const_int *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'case' *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'default'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'default' ':'", 0, PX_Syntax_switch_default, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_case_label = 'default' *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_unit = switch_case_label", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_unit = block", 0, PX_Syntax_switch_scope_block_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_unit = scope_block", 0, PX_Syntax_switch_scope_block_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_units = switch_unit ...", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_units = switch_unit *", 0, 0, 0))
		return PX_FALSE;

	// Also allow an entirely empty switch body.
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_units = *", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch'", 0, PX_Syntax_switch_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '('", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr", 0, PX_Syntax_switch_condition_expr_merge, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr ')'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr ')' '{'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr ')' *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr ')' '{' switch_units '}'", 0, PX_Syntax_switch_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "switch_block = 'switch' '(' expr ')' '{' switch_units *", 0, PX_Syntax_switch_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
