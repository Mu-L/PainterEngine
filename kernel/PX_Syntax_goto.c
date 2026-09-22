#include "PX_Syntax_goto.h"
PX_SYNTAX_FUNCTION(PX_Syntax_goto_begin)
{
	px_abi* goto_block_abi;
	if (PX_NULL == (goto_block_abi = PX_Syntax_NewAbi(pSyntax, "goto_block")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_begin: create goto_block abi failed.");
		return PX_FALSE;
	}

	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_goto_identifier)
{
	px_abi* plast_base_scope_abi = PX_Syntax_GetLastBaseScopeAbi(pSyntax);
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_string payload;
	const px_char* pbase_scope_id, *pidentifier;
	px_int goto_routes_member_count;
	PX_ASSERTIFX(!plast_base_scope_abi || !PX_Syntax_CheckAbiName(plast_base_scope_abi, "scope"), "PX_Syntax_goto_identifier: last base scope abi is not base_scope");
	PX_ASSERTIFX(!pabi || !PX_Syntax_CheckAbiName(pabi, "identifier"), "PX_Syntax_goto_identifier: last abi is not identifier");
	pbase_scope_id = PX_AbiGetValue_string(plast_base_scope_abi, "id");
	pidentifier = PX_AbiGetValue_string(pabi, "value");

	goto_routes_member_count = PX_AbiGet_PayloadMemberCount(plast_base_scope_abi, "goto_routes");

	if (!PX_StringInitializeFormat1(pSyntax->mp, &payload, "goto_routes.[%1].label", PX_STRINGFORMAT_INT(goto_routes_member_count)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_base_scope_abi, PX_StringGetText(&payload), pidentifier))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label route_ir failed.");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	if (!PX_StringFormat1(&payload, "goto_routes.[%1].ir", PX_STRINGFORMAT_INT(goto_routes_member_count)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label initialization failed.");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(plast_base_scope_abi,PX_StringGetText(&payload),""))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label route_ir failed.");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	if (!PX_StringFormat1(&payload, "goto_routes.[%1]", PX_STRINGFORMAT_INT(goto_routes_member_count)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label initialization failed.");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	if (!PX_Syntax_GrabeVariablesAbi(pSyntax, plast_base_scope_abi, PX_StringGetText(&payload)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	PX_StringFree(&payload);

	if (!PX_Syntax_NewIRInstructionFormat2(pSyntax, "goto_block", "jmp _goto_route_%1_%2", PX_STRINGFORMAT_STRING(pbase_scope_id), PX_STRINGFORMAT_STRING(pidentifier)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_goto_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop identifier
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_label_identifier)
{
	px_abi *goto_block_abi;
	px_abi* plast_base_scope_abi = PX_Syntax_GetLastBaseScopeAbi(pSyntax);
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_string label;
	px_string payload;
	const px_char* pbase_scope_id, * pidentifier;
	PX_ASSERTIFX(!plast_base_scope_abi || !PX_Syntax_CheckAbiName(plast_base_scope_abi, "scope"), "PX_Syntax_goto_identifier: last base scope abi is not base_scope");
	PX_ASSERTIFX(!pabi || !PX_Syntax_CheckAbiName(pabi, "identifier"), "PX_Syntax_goto_identifier: last abi is not identifier");
	pbase_scope_id = PX_AbiGetValue_string(plast_base_scope_abi, "id");
	pidentifier = PX_AbiGetValue_string(pabi, "value");
	goto_block_abi= PX_Syntax_NewAbi(pSyntax, "label_block");
	if (PX_NULL == goto_block_abi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_begin: create for_block abi failed.");
		return PX_FALSE;
	}
	if(!PX_StringInitializeFormat2(pSyntax->mp, &label, "_%1_%2:", PX_STRINGFORMAT_STRING(pbase_scope_id), PX_STRINGFORMAT_STRING(pidentifier)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(goto_block_abi, "label_name", PX_StringGetText(&label)))
	{
		PX_StringFree(&label);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	if (!PX_StringInitializeFormat1(pSyntax->mp,&payload,"labels.%1.name", PX_STRINGFORMAT_STRING(pidentifier)))
	{
		PX_StringFree(&label);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(plast_base_scope_abi, &payload, PX_StringGetText(&label)))
	{
		PX_StringFree(&label);
		PX_StringFree(&payload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	if (!PX_StringFormat1(&payload, "labels.%1", PX_STRINGFORMAT_STRING(pidentifier)))
	{
		PX_StringFree(&label);
		PX_StringFree(&payload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_GrabeVariablesAbi(pSyntax, plast_base_scope_abi, PX_StringGetText(&payload)))
	{
		PX_StringFree(&label);
		PX_StringFree(&payload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_label_identifier: gen_label initialization failed.");
		return PX_FALSE;
	}

	PX_StringFree(&label);
	PX_StringFree(&payload);

	PX_Syntax_PopLastSecondAbi(pSyntax);//pop identifier
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_goto_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_goto_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_goto(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "goto_block = 'goto'", 0, PX_Syntax_goto_begin, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "goto_block = 'goto' identifier", 0, PX_Syntax_goto_identifier, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "goto_block = 'goto' identifier ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "goto_block = 'goto' identifier *", 0, PX_Syntax_goto_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "goto_block = 'goto' *", 0, PX_Syntax_goto_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "label_block = identifier", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "label_block = identifier ':'", 0, PX_Syntax_label_identifier, 0))
		return PX_FALSE;

	
	return PX_TRUE;
}

