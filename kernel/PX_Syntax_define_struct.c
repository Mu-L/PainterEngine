#include "PX_Syntax_define_struct.h"

static px_int unnamed_struct_count = 0;

PX_SYNTAX_FUNCTION(PX_Syntax_define_struct_identifier_to_struct_name)
{
	px_abi *pidentifier_abi;
	px_int source_index,begin,end;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "Error: struct name should be identifier");
	if (!PX_Syntax_RenameLastAbi(pSyntax, "struct_scope"))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:unexpected struct name");
		return PX_FALSE;
	}
	pidentifier_abi = PX_Syntax_GetLastAbi(pSyntax);
	source_index = PX_AbiGetValue_int(pidentifier_abi, "source_index");
	begin = PX_AbiGetValue_int(pidentifier_abi, "begin");
	end = PX_AbiGetValue_int(pidentifier_abi, "end");
	if(!PX_Syntax_NewStaticMapToken(pSyntax, source_index, begin,source_index, end, PX_Syntax_GetRandomColor(1232), "struct_scope"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_define_struct_unname)
{
	px_abi* pnew_abi;
	const px_char* punnamed = PX_Syntax_AllocUnnamed(pSyntax, "__scope_");
	if (PX_Syntax_PreviewNextChar(pSyntax)!='{')
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:expected '{' character");
		return PX_FALSE;
	}
	if (!punnamed)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	pnew_abi = PX_Syntax_NewAbi(pSyntax, "struct_scope");
	if (!pnew_abi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pnew_abi, "value", punnamed))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	return PX_TRUE;
}



PX_SYNTAX_FUNCTION(PX_Syntax_define_struct_type)
{
	px_string namedpayload;
	px_abi* plast_scope_abi = PX_Syntax_GetLastScopeAbi(pSyntax);
	px_abi* pname_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "struct_scope");
	px_abi* plast_abi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* new_type;
	px_int alloc_size;
	PX_ASSERTIFX(!plast_scope_abi, "Error: struct scope abi not found");
	PX_ASSERTIFX(!pname_abi, "Error: struct name abi not found");
	PX_ASSERTIFX(!plast_abi, "Error: struct unknown error");
	const px_char* pstruct_name = PX_AbiGetValue_string(pname_abi, "value");
	
	if (PX_Syntax_CheckAbiName(plast_abi,"scope_block")&&0!=(alloc_size = PX_AbiGetValue_int(plast_abi, "alloc_local")))
	{
		px_string buildpayload;
		if (!PX_StringInitialize(pSyntax->mp, &buildpayload))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			return PX_FALSE;
		}
		if (!PX_StringFormat1(&buildpayload, "type_defines.struct.%1", PX_STRINGFORMAT_STRING(pstruct_name)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			PX_StringFree(&buildpayload);
			return PX_FALSE;
		}

		if (!PX_AbiMerge_Abi(plast_scope_abi, plast_abi,PX_StringGetText(&buildpayload),"variables"))
		{
			PX_StringFree(&buildpayload);
			PX_Syntax_Terminate(pSyntax, "runtime:error:the struct definitions could not be merged");
			return PX_FALSE;
		}

		if (!PX_StringFormat1(&buildpayload, "type_defines.struct.%1.type_defines", PX_STRINGFORMAT_STRING(pstruct_name)))
		{
			PX_StringFree(&buildpayload);
			PX_Syntax_Terminate(pSyntax, "runtime:error:the struct definitions could not be merged");
			return PX_FALSE;
		}

		if (!PX_StringFormat1(&buildpayload, "struct.%1", PX_STRINGFORMAT_STRING(pstruct_name)))
		{
			PX_StringFree(&buildpayload);
			PX_Syntax_Terminate(pSyntax, "runtime:error:the struct definitions could not be merged");
			return PX_FALSE;
		}

		if (!PX_Syntax_NewType(pSyntax, PX_StringGetText(&buildpayload), pstruct_name, alloc_size, PX_NULL))
		{
			PX_StringFree(&buildpayload);
			return PX_FALSE;
		}

		PX_StringFree(&buildpayload);

		PX_Syntax_PopAbi(pSyntax);//pop scope_block
	}
	else
	{
		px_string buildpayload;
		if (!PX_StringInitialize(pSyntax->mp, &buildpayload))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			return PX_FALSE;
		}
		if (!PX_StringFormat1(&buildpayload, "struct.%1", PX_STRINGFORMAT_STRING(pstruct_name)))
		{
			PX_StringFree(&buildpayload);
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			return PX_FALSE;
		}
		if (!PX_Syntax_GetTypeAbi(pSyntax, PX_StringGetText(&buildpayload), PX_NULL))
		{
			if (!PX_Syntax_NewType(pSyntax, PX_StringGetText(&buildpayload), pstruct_name, 0, PX_NULL))
			{
				PX_StringFree(&buildpayload);
				return PX_FALSE;
			}
		}
		PX_StringFree(&buildpayload);
		return PX_TRUE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &namedpayload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
		return PX_FALSE;
	}
	if (!PX_StringFormat1(&namedpayload, "struct.%1", PX_STRINGFORMAT_STRING(pstruct_name)))
	{
		PX_StringFree(&namedpayload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
		return PX_FALSE;
	}
	
	PX_Syntax_PopAbi(pSyntax);//pop struct_name

	new_type = PX_Syntax_NewAbi(pSyntax, "type");
	if (!new_type)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_define_struct_leave_scope out of memory 3");
		PX_StringFree(&namedpayload);
		return PX_FALSE;
	}

	if (!PX_AbiSet_string(new_type, "value", PX_StringGetText(&namedpayload)))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:PX_Syntax_define_struct_leave_scope out of memory 5");
		PX_StringFree(&namedpayload);
		return PX_FALSE;
	}

	PX_StringFree(&namedpayload);
	return PX_TRUE;

}


PX_SYNTAX_FUNCTION(PX_Syntax_define_struct_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:unexpected define_struct");
	return PX_FALSE;
}


px_bool PX_Syntax_load_define_struct(PX_Syntax* pSyntax)
{
	//'struct' [struct_name] '{'<---(scope) define_struct_list '}' 

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "struct_name = identifier",0, PX_Syntax_define_struct_identifier_to_struct_name, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "struct_name = *",0, PX_Syntax_define_struct_unname, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "define_struct",0, PX_Syntax_define_struct_type,0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "define_struct = 'struct'",0, PX_Syntax_TokenRender,0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "define_struct = 'struct' struct_name base_scope_block ",0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "define_struct = 'struct' struct_name *",0, 0, 0))
		return PX_FALSE;


	return PX_TRUE;
}
