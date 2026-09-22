#include "PX_Syntax_declare_prefix.h"

PX_SYNTAX_FUNCTION(PX_Syntax_load_declare_prefix_exec)
{
	px_int begin, end;
	px_int begin_source_index, end_source_index;
	px_abi* pabi = PX_Syntax_NewAbi(pSyntax, "declare_prefix");
	if (!pabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_load_declare_prefix_exec Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi, "value", PX_Syntax_GetCurrentLexeme(pSyntax)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_load_declare_prefix_exec Memory Error2");
		return PX_FALSE;
	}

	//symbol ir
	begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	if (begin_source_index != end_source_index)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_load_declare_prefix_exec Memory Error1");
		return PX_FALSE;
	}
	if(!PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 232, 212, 166), "declare_prefix"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_load_declare_prefix_exec Memory Error3");
		return PX_FALSE;
	}

	return PX_TRUE;
}


px_bool PX_Syntax_load_declare_prefix(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefix = 'static'",0, PX_Syntax_load_declare_prefix_exec, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefix = 'const'",0, PX_Syntax_load_declare_prefix_exec, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefix = 'unsigned'", 0, PX_Syntax_load_declare_prefix_exec, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefixs = declare_prefix ", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefixs = declare_prefix ...", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_prefixs = declare_prefix *", 0, 0, 0))
		return PX_FALSE;

	return PX_TRUE;
}
