#include "PX_Syntax_scope.h"

PX_SYNTAX_FUNCTION(PX_Syntax_scope_begin)
{
	if (!PX_Syntax_EnterScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_begin EnterScope Error");
		return PX_FALSE;
	}
	return PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRenderScope);
}

PX_SYNTAX_FUNCTION(PX_Syntax_base_scope_begin)
{
	if (!PX_Syntax_EnterBaseScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_base_scope_begin EnterScope Error");
		return PX_FALSE;
	}
	return PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRenderScope);
}

PX_SYNTAX_FUNCTION(PX_Syntax_scope_end)
{
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRenderScope);
	if (!PX_Syntax_LeaveScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end LeaveScope Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_scope_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_scope_error Syntax Error");
	return PX_FALSE;
}

px_bool PX_Syntax_load_scope_block(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "scope_block = '{'", 0, PX_Syntax_scope_begin, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "scope_block = '{' '}'", 0, PX_Syntax_scope_end, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "scope_block = '{' blocks '}'", 0, PX_Syntax_scope_end, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "scope_block = '{' *", 0, PX_Syntax_scope_error, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "scope_block = '{' blocks *", 0, PX_Syntax_scope_error, 0))
	{
		return PX_FALSE;
	}


	if (!PX_Syntax_Parse_PEBNF(pSyntax, "base_scope_block = '{'", 0, PX_Syntax_base_scope_begin, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "base_scope_block = '{' '}'", 0, PX_Syntax_scope_end, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "base_scope_block = '{' blocks '}'", 0, PX_Syntax_scope_end, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "base_scope_block = '{' *", 0, PX_Syntax_scope_error, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "base_scope_block = '{' blocks *", 0, PX_Syntax_scope_error, 0))
	{
		return PX_FALSE;
	}

	return PX_TRUE;
}
