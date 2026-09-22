#include "PX_Syntax_include.h"


PX_SYNTAX_FUNCTION(PX_Syntax_Parse_include)
{
	const px_char* pfilename;
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	pfilename = PX_AbiGet_string(plast, "value");
	if(PX_Syntax_CallSource(pSyntax, pfilename,PX_NULL)==PX_FALSE)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Parse_include: include file could not be loaded");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_Parse_include_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_Parse_include_error: include statement error");
	return PX_FALSE;
}


px_bool PX_Syntax_load_include(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "include = '#' 'include' ", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "include = '#' 'include' bcontainer '\n'",0, PX_Syntax_Parse_include, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "include = '#' 'include' const_string '\n'", 0, PX_Syntax_Parse_include, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "include = '#' 'include' * '\n'", 0, PX_Syntax_Parse_include_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
