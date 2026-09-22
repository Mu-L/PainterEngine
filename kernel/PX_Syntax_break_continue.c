#include "PX_Syntax_break_continue.h"

//find the innermost loop(-or-switch for break) block abi from backward
static px_abi* PX_Syntax_break_continue_find_block(PX_Syntax* pSyntax, px_bool include_switch)
{
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "while_block") || \
			PX_Syntax_CheckAbiName(pabi, "do_while_block") || \
			PX_Syntax_CheckAbiName(pabi, "for_block"))
		{
			return pabi;
		}
		//continue can only be used in loop block, break can be used in loop or switch block
		if (include_switch && PX_Syntax_CheckAbiName(pabi, "switch_block"))
		{
			return pabi;
		}
	}
	return PX_NULL;
}

static px_bool PX_Syntax_break_continue_emit_jmp(PX_Syntax* pSyntax, px_bool is_break)
{
	px_abi* plast_emit_abi;
	const px_char* pemitName = is_break ? "break_block" : "continue_block";
	px_abi* ploop_block = PX_Syntax_break_continue_find_block(pSyntax, is_break);
	px_char payload[64] = { 0 };
	const px_char* plabel_name;
	if (is_break)
		plast_emit_abi = PX_Syntax_GetAbiFromBackward(pSyntax, pemitName);
	else
		plast_emit_abi = PX_Syntax_GetAbiFromBackward(pSyntax, pemitName);

	PX_ASSERTIFX(!plast_emit_abi, "PX_Syntax_break_continue_emit_jmp: no scope abi found.");
	if (!ploop_block)
	{
		if (is_break)
			PX_Syntax_Terminate(pSyntax, "ast:error:'break' should be in a loop or switch block");
		else
			PX_Syntax_Terminate(pSyntax, "ast:error:'continue' should be in a loop block");
		return PX_FALSE;
	}

	plabel_name = PX_AbiGetValue_string(ploop_block, "label_name");
	PX_strcat_s(payload, sizeof(payload), "jmp ");
	PX_strcat_s(payload, sizeof(payload), plabel_name);
	PX_strcat_s(payload, sizeof(payload), is_break ? "_break\n" : "_continue\n");
	pSyntax->reg_expr_source_index = PX_Syntax_GetCurrentSourceIndex(pSyntax);
	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexemeLine(pSyntax);
	if (!PX_Syntax_NewIRLocation(pSyntax, pemitName))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_break_continue_emit_jmp: new ir location failed.");
		return PX_FALSE;
	}
	if (!PX_AbiAppend_string(plast_emit_abi, "ir", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_break_continue_emit_jmp: append jmp ir to scope failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_break_continue_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_break_continue_error Syntax Error, ';' expected");
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_break_handle)
{
	px_abi* pnewabi;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	pnewabi = PX_Syntax_NewAbi(pSyntax, "break_block");
	if (!pnewabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_break_begin: create break_block abi failed.");
		return PX_FALSE;
	}
	return PX_Syntax_break_continue_emit_jmp(pSyntax, PX_TRUE);
}

PX_SYNTAX_FUNCTION(PX_Syntax_continue_handle)
{
	px_abi* pnewabi;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	pnewabi = PX_Syntax_NewAbi(pSyntax, "continue_block");
	if (!pnewabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_continue_begin: create continue_block abi failed.");
		return PX_FALSE;
	}
	return PX_Syntax_break_continue_emit_jmp(pSyntax, PX_FALSE);
}

px_bool PX_Syntax_load_break_continue(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "break_block = 'break'", 0, PX_Syntax_break_handle, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "break_block = 'break' ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "break_block = 'break' *", 0, PX_Syntax_break_continue_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "continue_block = 'continue'", 0, PX_Syntax_continue_handle, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "continue_block = 'continue' ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "continue_block = 'continue' *", 0, PX_Syntax_break_continue_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
