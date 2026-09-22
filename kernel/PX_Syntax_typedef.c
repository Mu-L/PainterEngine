#include "PX_Syntax_typedef.h"

PX_SYNTAX_FUNCTION(PX_Syntax_Parse_typedef)
{
	px_abi* psecondlastabi, * plastabi;
	const px_char* ptype_source = PX_NULL;
	const px_char* ptype_target = PX_NULL;
	psecondlastabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(!psecondlastabi || PX_Syntax_CheckAbiName(psecondlastabi, "type") == PX_FALSE, "Unknow Error");
	plastabi = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!plastabi || PX_Syntax_CheckAbiName(plastabi, "identifier") == PX_FALSE, "Unknow Error");
	ptype_source = PX_AbiGet_string(psecondlastabi, "type_define.type");
	ptype_target = PX_AbiGet_string(plastabi, "value");
	if (PX_Syntax_GetTypeByMnemonic(pSyntax, ptype_target))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:typedef name already exists");
		return PX_FALSE;
	}
	if (!PX_Syntax_NewTypedef(pSyntax, ptype_source, ptype_target))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Parse_typedef Memory Error1");
		return PX_FALSE;
	}
	return PX_TRUE;

}

PX_SYNTAX_FUNCTION(PX_Syntax_Parse_typedef_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error: typedef statement error");
	return PX_FALSE;
}


px_bool PX_Syntax_load_typedef(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "typedef = 'typedef' ", 0, PX_Syntax_TokenRender,0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "typedef = 'typedef' type identifier ", 0, PX_Syntax_Parse_typedef,0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "typedef = 'typedef' type * ", 0, PX_Syntax_Parse_typedef_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "typedef = 'typedef' type identifier ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "typedef = 'typedef' type identifier *", 0, PX_Syntax_Parse_typedef_error, 0))
		return PX_FALSE;

	return PX_TRUE;
}
