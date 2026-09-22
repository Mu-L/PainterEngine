#include "PX_Syntax_return.h"

PX_SYNTAX_FUNCTION(PX_Syntax_return_end)
{
	if (!PX_Syntax_NewIRInstruction0(pSyntax, "expr", "ret"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_Syntax_load_return(PX_Syntax* pSyntax)
{
	PX_Syntax_Parse_PEBNF(pSyntax, "return", 0, PX_Syntax_TokenRender, 0);

	PX_Syntax_Parse_PEBNF(pSyntax, "return = 'return'", 0, PX_Syntax_TokenRender, 0);

	PX_Syntax_Parse_PEBNF(pSyntax, "return = 'return' expr_block",0, PX_Syntax_return_end, 0);
	return PX_TRUE;
}