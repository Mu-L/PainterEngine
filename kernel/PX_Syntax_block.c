#include "PX_Syntax_block.h"

PX_SYNTAX_FUNCTION(PX_Syntax_empty_block)
{
	px_int counter = 0;
	while (PX_TRUE)
	{
		px_char ch = PX_Syntax_PreviewNextChar(pSyntax);
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		{
			PX_Syntax_GetNextChar(pSyntax);
			counter++;
			continue;
		}
		else
		{
			break;
		}
	}
	return counter != 0;
}


px_bool PX_Syntax_load_block(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "empty_block= *", PX_Syntax_empty_block, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= comment", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= include", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= define", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= declare_variable", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= function_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= if_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= while_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= do_while_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= for_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= switch_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= break_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= continue_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= return", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= scope_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= expr_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= goto_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "block= label_block", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "blocks= empty_block ...", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "blocks= empty_block *", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "blocks= block ...", 0, 0, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "blocks= block *", 0, 0, 0))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}
