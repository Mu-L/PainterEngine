#include "PX_Syntax_declare_variable.h"

px_bool PX_Syntax_new_declare_variable_token(struct _PX_Syntax* pSyntax, const px_char* final_from)
{
	px_abi* pidentifier_abi, * ptype_abi;
	px_int type_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "type");
	px_int declare_token_prefix_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "declare_token_prefix");
	px_int identifier_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "identifier");
	px_int declare_array_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "declare_array");
	px_abi* pscopeabi = PX_NULL;
	const px_char* pdeclare_prefix_value = PX_NULL;
	const px_char* pidentifier_name = PX_NULL;

	px_int identifier_source_index = -1;
	px_int identifier_begin = -1, identifier_end = -1;
	px_string final_type;
	px_bool is_array = PX_FALSE;
	px_bool is_reference = PX_FALSE;
	px_string var_payload;

	const px_char* pbase_type;
	px_int pointer_level = 0;
	px_int array_d = 0, array_count = 1;
	px_int type_size = 0, final_offset = 0;
	//type_index identifier_index should exist, but declare_prefix_index and declare_token_prefix_index may not exist
	PX_ASSERTIFX(type_index == -1, "Error: type not found");
	PX_ASSERTIFX(identifier_index == -1, "Error: identifier not found");
	if(!PX_StringInitialize(pSyntax->mp, &final_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
		goto _ERROR;
	}
	if(!PX_StringInitialize(pSyntax->mp, &var_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
		goto _ERROR;
	}

	//get scope abi
	pscopeabi = PX_Syntax_GetLastScopeAbi(pSyntax);
	if (!pscopeabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token No Scope Found");
		goto _ERROR;
	}

	//get identifier name
	pidentifier_abi = PX_Syntax_GetAbiByIndex(pSyntax, identifier_index);
	pidentifier_name = PX_AbiGetValue_string(pidentifier_abi, "value");
	PX_ASSERTIFX(pidentifier_name == PX_NULL, "Error: identifier value not found");

	//get identifier token info
	identifier_source_index = PX_AbiGetValue_int(pidentifier_abi, "source_index");
	identifier_begin = PX_AbiGetValue_int(pidentifier_abi, "begin");
	identifier_end = PX_AbiGetValue_int(pidentifier_abi, "end");

	//get type
	ptype_abi = PX_Syntax_GetAbiByIndex(pSyntax, type_index);
	pbase_type = PX_AbiGetValue_string(ptype_abi, "value");
	
	//array
	if (declare_array_index != -1)
	{
		//build array type(prefix)
		px_int i;
		px_abi* pdeclare_array_abi = PX_Syntax_GetAbiByIndex(pSyntax, declare_array_index);
		array_d = PX_AbiGetValue_int(pdeclare_array_abi, "d");
		if (!PX_StringCat(&final_type, "array."))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}
		is_array = PX_TRUE;
		for (i = 0; i < array_d; i++)
		{
			px_int d_count;
			d_count = PX_AbiGetValue_int(pdeclare_array_abi, PX_itos(i + 1, 10).data);
			if (!PX_StringCat(&final_type, PX_itos(d_count, 10).data))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
				goto _ERROR;
			}
			if (i != array_d - 1)
			{
				if (!PX_StringCat(&final_type, "x"))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
					goto _ERROR;
				}
			}
		}
		if (!PX_StringCat(&final_type, "."))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}
		array_count = PX_AbiGetValue_int(pdeclare_array_abi, "count");
	}

	//pointer_level
	if (declare_token_prefix_index != -1)
	{
		px_abi* pdeclare_token_prefix_abi = PX_Syntax_GetAbiByIndex(pSyntax, declare_token_prefix_index);
		const px_char* pdeclare_token_prefix_value = PX_AbiGetValue_string(pdeclare_token_prefix_abi, "type");
		if (PX_strequ(pdeclare_token_prefix_value, "pointer"))
		{
			pointer_level = PX_AbiGetValue_int(pdeclare_token_prefix_abi, "pointer_level");
			if (!PX_StringCat(&final_type, "pointer.") || !PX_StringCat(&final_type, PX_itos(pointer_level, 10).data) || !PX_StringCat(&final_type, "."))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
				goto _ERROR;
			}
		}
		else if (PX_strequ(pdeclare_token_prefix_value, "reference"))
		{
			is_reference = PX_TRUE;
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token unknown declare_token_prefix value");
			goto _ERROR;
		}
	}

	//type
	if (!PX_StringCat(&final_type, pbase_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
		goto _ERROR;
	}



	/////////////////////////////////////////////////////////////////////
	//set scope_variable property
	/////////////////////////////////////////////////////////////////////
	
	//set identifier into scope variables
	
	if (!PX_StringFormat1(&var_payload, "variables.%1.identifier", PX_STRINGFORMAT_STRING(pidentifier_name))||\
		!PX_AbiSet_string(pscopeabi, PX_StringGetText(&var_payload), pidentifier_name))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error1");
		goto _ERROR;
	}


	//from
	
	if (!PX_StringFormat1(&var_payload, "variables.%1.from", PX_STRINGFORMAT_STRING(pidentifier_name))||\
		!PX_AbiSet_string(pscopeabi, PX_StringGetText(&var_payload), final_from))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error4");
		goto _ERROR;
	}

	if (!PX_StringFormat1(&var_payload, "variables.%1.type", PX_STRINGFORMAT_STRING(pidentifier_name))||\
		!PX_AbiSet_string(pscopeabi, PX_StringGetText(&var_payload), PX_StringGetText(&final_type)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error5");
		goto _ERROR;
	}

	//set offset
	type_size = PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&final_type));
	if (type_size==0)
	{
		PX_Syntax_Terminate(pSyntax, "could not get type size");
		goto _ERROR;
	}

	if(PX_strequ(final_from,"param"))
		final_offset = PX_Syntax_AllocParam(pSyntax, type_size);
	else if (PX_strequ(final_from, "global"))
		final_offset = PX_Syntax_AllocGlobal(pSyntax, type_size);
	else if(PX_strequ(final_from, "local"))
		final_offset = PX_Syntax_AllocLocal(pSyntax, type_size);
	else
	{
		PX_ASSERTX("Error: unknown variable from");
		goto _ERROR;
	}

	PX_StringFormat1(&var_payload, "variables.%1.offset", PX_STRINGFORMAT_STRING(pidentifier_name));
	if (!PX_AbiSet_int(pscopeabi, PX_StringGetText(&var_payload), final_offset))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error5");
		goto _ERROR;
	}

	//new token
	do
	{
		px_abi* pmapabi;
		px_string info;
		if (PX_NULL==(pmapabi= PX_Syntax_NewDynamicMapToken(pSyntax, identifier_source_index, identifier_begin, identifier_source_index, identifier_end, PX_Syntax_GetRandomColor(1515), "variable")))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error6");
			goto _ERROR;
		}
		//set token info

		if (!PX_AbiSet_string(pmapabi, "variable_name", pidentifier_name))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}

		if (!PX_AbiSet_string(pmapabi, "variable_type", PX_StringGetText(&final_type)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}
		
		if (!PX_AbiSet_string(pmapabi, "variable_from", final_from))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}

		if (!PX_AbiSet_dword(pmapabi, "variable_offset", (px_dword)final_offset))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}

		if (!PX_AbiSet_dword(pmapabi, "variable_size", (px_dword)type_size))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}

		if (!PX_StringInitialize(pSyntax->mp, &info))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error");
			goto _ERROR;
		}
		//from type size offset
		PX_StringFormat4(&info, "from:%1\ntype:%2\nsize:%3\noffset:%4", PX_STRINGFORMAT_STRING(final_from), PX_STRINGFORMAT_STRING(PX_StringGetText(&final_type)), PX_STRINGFORMAT_INT(type_size), PX_STRINGFORMAT_INT(final_offset));
		if (!PX_Syntax_SetLastMapInfo(pSyntax, identifier_source_index, PX_StringGetText(&info)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_variable_token Memory Error7");
			PX_StringFree(&info);
			goto _ERROR;
		}
		PX_StringFree(&info);
	} while (0);

	if (declare_array_index != -1)
	{
		PX_Syntax_PopAbiIndex(pSyntax, declare_array_index);
	}

	PX_Syntax_PopAbiIndex(pSyntax, identifier_index);

	if (declare_token_prefix_index != -1)
	{
		PX_Syntax_PopAbiIndex(pSyntax, declare_token_prefix_index);
	}

	PX_StringFree(&final_type);
	PX_StringFree(&var_payload);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&final_type);
	PX_StringFree(&var_payload);
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_new_variable_token)
{
	px_int lifetime = PX_Syntax_GetLifetime(pSyntax);
	return PX_Syntax_new_declare_variable_token(pSyntax, lifetime==1?"global":"local");
}



PX_SYNTAX_FUNCTION(PX_Syntax_declare_variable_end)
{
	px_int index;

	if ((index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "declare_token_prefixs")) != -1)
	{
		PX_Syntax_PopAbiIndex(pSyntax, index);
	}

	if ((index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "type")) != -1)
	{
		PX_Syntax_PopAbiIndex(pSyntax, index);
	}

	if ((index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "declare_prefix")) != -1)
	{
		PX_Syntax_PopAbiIndex(pSyntax, index);
	}

	return PX_TRUE;
}


px_bool PX_Syntax_load_declare_variable(PX_Syntax* pSyntax)
{
	// [declare_prefix] type {[declare_token_prefixs] identifier [declare_array],...}
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "identifier_token", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "identifier_token =identifier", 0,0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "identifier_token =identifier declare_token_suffix", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "identifier_token =identifier *", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "variable",0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "variable =declare_token_prefix identifier_token",0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "variable =identifier_token",0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_list = variable ',' ",0, PX_Syntax_new_variable_token, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_list = variable ',' ... ", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_list = variable ';' ", 0, PX_Syntax_new_variable_token, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_variable", 0, PX_Syntax_declare_variable_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "type_set = declare_prefixs type", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "type_set = type", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "type_set = define_struct", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_variable = type_set ';'", 0, PX_Syntax_declare_variable_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "declare_variable = type_set declare_list",0, 0, 0))
		return PX_FALSE;



	return PX_TRUE;
}
