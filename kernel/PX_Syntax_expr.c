#include "PX_Syntax_expr.h"
#include "PX_Syntax_base_operate.h"

//static px_int expr_begin = -1;
//static px_int expr_end = -1;

static px_bool PX_Syntax_GetDeclareVariableAbiReadOnly(PX_Syntax* pSyntax, const px_char name[], px_abi* pout_abi)
{
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "scope"))
		{
			px_abi variables_abi;
			if (PX_AbiGet_AbiReadOnly(pabi, &variables_abi, "variables"))
			{
				if (PX_AbiExist_abi(&variables_abi, name))
				{
					*pout_abi = PX_AbiGetValue_abireadonly(&variables_abi, name);
					return PX_TRUE;
				}
			}

		}
	}
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_begin)
{
	//new abi
	px_int expr_begin = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "(", PX_SYNTAX_OPCODE_TYPE_BEGIN);
	px_abi* pabi = PX_Syntax_NewAbi(pSyntax, "expr");
	PX_ASSERTIFX(expr_begin == -1, "expr_begin missing");
	if (!pabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_begin NewAbi Memory Error");
		return PX_FALSE;
	}
	
	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexerLine(pSyntax);
	pSyntax->reg_expr_source_index = PX_Syntax_GetCurrentLexerIndex(pSyntax);
	
	//new opcode
	if (!PX_Syntax_ExecuteOpcode(pSyntax, expr_begin))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_update)
{
	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexerLine(pSyntax);
	pSyntax->reg_expr_source_index = PX_Syntax_GetCurrentLexerIndex(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_end)
{
	//check opcode stack
	px_abi* plast_abi_operand;
	px_int expr_end = PX_Syntax_GetOpcodeDefineIndex(pSyntax, ")", PX_SYNTAX_OPCODE_TYPE_END);
	const px_char* last_operand_type;
	px_int last_operand_type_size;
	px_abi* pexpr_abi;
	PX_ASSERTIFX(expr_end == -1, "expr_end missing");
	if (!PX_Syntax_ExecuteOpcode(pSyntax, expr_end))
	{
		return PX_FALSE;
	}

	plast_abi_operand = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_abi_operand, "operand"), "PX_Syntax_for_expr_merge: last abi is not operand");
	last_operand_type = PX_AbiGetValue_string(plast_abi_operand, "type");
	last_operand_type_size = PX_AbiGetValue_int(plast_abi_operand, "type_size");
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", pSyntax->reg_abi_stack.size - 1, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_expr_merge: map operand to common register failed.");
		return PX_FALSE;
	}
	pexpr_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(!pexpr_abi, "PX_Syntax_for_expr_merge: second last abi is null");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pexpr_abi, "expr"), "PX_Syntax_for_expr_merge: second last abi is not expr");
	if (PX_AbiSet_string(pexpr_abi, "type", last_operand_type) == PX_FALSE)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_expr_merge: set expr type failed.");
		return PX_FALSE;
	}
	if (PX_AbiSet_int(pexpr_abi, "type_size", last_operand_type_size) == PX_FALSE)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_for_expr_merge: set expr size failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop operand
	return PX_TRUE;
}



PX_SYNTAX_FUNCTION(PX_Syntax_expr_parse_identifier_operand)
{
	px_abi* plastabi;
	px_abi ref_variable_abi_readonly, array_abi, decorate_abi;
	const px_char* type;
	px_int begin, end,offset,type_size, source_index;
	const px_char* variable_from;
	const px_char* pidentifier_value;
	
	plastabi = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!plastabi || PX_Syntax_CheckAbiName(plastabi, "identifier") == PX_FALSE, "Unknow Error");
	source_index = PX_AbiGetValue_int(plastabi, "source_index");
	begin = PX_AbiGetValue_int(plastabi, "begin");
	end = PX_AbiGetValue_int(plastabi, "end");
	pidentifier_value = PX_AbiGetValue_string(plastabi, "value");
	//find variable from backward
	//is last operand is struct and last opcode is '.' or '->'
	px_abi* plastoperand_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "operand");
	px_abi* plastopcode_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "opcode");
	if (plastopcode_abi&& plastoperand_abi)
	{
		const px_char* lastoperandtype = PX_AbiGetValue_string(plastoperand_abi, "type");
		px_int index = PX_AbiGetValue_int(plastopcode_abi, "index");
		PX_Syntax_opcode* popcode_define;
		PX_ASSERTIFX(!PX_VECTOR_IN_RANGE(&pSyntax->reg_expr_opcode_stack, index), "Opcode index out of range");
		popcode_define = PX_Syntax_GetOpcodeDefine(pSyntax, index);
		PX_ASSERTIFX(!popcode_define, "missing opcode definition");
		if (((PX_Syntax_TypeMatch(lastoperandtype,"struct")&&(PX_strequ(popcode_define->opcode,"."))||\
			(PX_Syntax_TypeMatch(lastoperandtype, "pointer.struct")&&PX_strequ(popcode_define->opcode, "->"))))\
		&& popcode_define->type ==PX_SYNTAX_OPCODE_TYPE_BINARY)
		{
			px_abi* poperand;
			px_abi member_abi;
			const px_char* member_type;
			px_int member_type_size,member_offset;
			px_string format_string;
			px_abi* pmapabi;
			if(!PX_Syntax_GetStructMemberAbi(pSyntax, &member_abi, lastoperandtype, pidentifier_value))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand GetStructMemberAbi failed");
				return PX_FALSE;
			}
			member_type = PX_AbiGetValue_string(&member_abi, "type");
			member_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, member_type);
			PX_ASSERTIFX(member_type_size == 0, "Error: type size not found");
			member_offset = PX_AbiGetValue_int(&member_abi, "offset");
			if (!PX_StringInitialize(pSyntax->mp, &format_string))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand StringInitialize Memory Error");
				return PX_FALSE;
			}
			if (!PX_StringFormat5(&format_string, "%1->%2:%3 offset:%4 size:%5", PX_STRINGFORMAT_STRING(lastoperandtype), \
				PX_STRINGFORMAT_STRING(pidentifier_value), \
				PX_STRINGFORMAT_STRING(member_type), PX_STRINGFORMAT_INT(member_offset), \
				PX_STRINGFORMAT_INT(member_type_size)))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand StringFormat5 Memory Error");
				PX_StringFree(&format_string);
				return PX_FALSE;
			}

			if (PX_NULL == (pmapabi = PX_Syntax_NewDynamicMapToken(pSyntax, source_index, begin, source_index, end, PX_COLOR(255, 255, 192, 188), member_type)))
			{
				PX_StringFree(&format_string);
				PX_Syntax_Terminate(pSyntax, "runtime:error:New IR Token Memory Error");
				return PX_FALSE;
			}
			if(!PX_Syntax_SetLastMapInfo(pSyntax,source_index, PX_StringGetText(&format_string)))
			{
				PX_StringFree(&format_string);
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand NewLastMapInfo Memory Error");
				return PX_FALSE;
			}
			PX_StringFree(&format_string);

			poperand = PX_Syntax_PushOperand(pSyntax,"identifier", PX_strlen(pidentifier_value), PX_SYNTAX_OPERAND_FROM_CONST);
			if (!poperand)
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand PushOperand Memory Error");
				return PX_FALSE;
			}
			if (!PX_AbiSet_string(poperand, "value", pidentifier_value))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand Set_string Memory Error");
				return PX_FALSE;
			}
			PX_Syntax_PopLastSecondAbi(pSyntax);
			return PX_TRUE;
		}
	}

	//identifier is variable name?
	if (PX_Syntax_GetDeclareVariableAbiReadOnly(pSyntax, pidentifier_value, &ref_variable_abi_readonly))
	{
		px_abi* poperand;
		PX_SYNTAX_OPERAND_FROM operand_from;
		px_string content = { 0 };
		px_abi* pmapabi;
		//mandatory variable
		variable_from = PX_AbiGetValue_string(&ref_variable_abi_readonly, "from");
		offset = PX_AbiGetValue_int(&ref_variable_abi_readonly, "offset");
		type = PX_AbiGetValue_string(&ref_variable_abi_readonly, "type");
		type_size = PX_Syntax_GetMatchTypeSize(pSyntax, type);

		if (PX_strequ(variable_from, "address"))
			operand_from = PX_SYNTAX_OPERAND_FROM_MODULEBASE;
		else if (PX_strequ(variable_from, "rdata"))
			operand_from = PX_SYNTAX_OPERAND_FROM_RDATA;
		else if (PX_strequ(variable_from, "global"))
			operand_from=PX_SYNTAX_OPERAND_FROM_GLOBAL;
		else if(PX_strequ(variable_from, "local"))
			operand_from=PX_SYNTAX_OPERAND_FROM_LOCAL;
		else if (PX_strequ(variable_from, "temp"))
			operand_from = PX_SYNTAX_OPERAND_FROM_TEMP;
		else if(PX_strequ(variable_from, "param"))
			operand_from=PX_SYNTAX_OPERAND_FROM_PARAM;
		else if (PX_strequ(variable_from, "reference"))
			operand_from = PX_SYNTAX_OPERAND_FROM_REFERENCE;
		else if (PX_strequ(variable_from, "member"))
			operand_from = PX_SYNTAX_OPERAND_FROM_MEMBER;
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand unknown from");
			return PX_FALSE;
		}

		poperand = PX_Syntax_PushOperand(pSyntax, type, type_size, operand_from);
		if (!poperand)
		{
			return PX_FALSE;
		}


		if (PX_AbiGet_AbiReadOnly(&ref_variable_abi_readonly, &array_abi, "array"))
		{
			if (!PX_AbiSet_Abi(poperand, "array", &array_abi))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand Set_Abi array Memory Error");
				return PX_FALSE;
			}
		}

		if (!PX_AbiSet_int(poperand, "offset", offset))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand Set_int offset Memory Error");
			return PX_FALSE;
		}

		if (PX_AbiGet_AbiReadOnly(&ref_variable_abi_readonly, &decorate_abi, "decorate"))
		{
			if (!PX_AbiSet_Abi(poperand, "decorate", &decorate_abi))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand Set_Abi decorate Memory Error");
				return PX_FALSE;
			}
		}


		if (PX_NULL==(pmapabi= PX_Syntax_NewDynamicMapToken(pSyntax, source_index,begin, source_index, end, PX_COLOR(255, 255, 255, 64), "variable")))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:New IR Token Memory Error");
			return PX_FALSE;
		}
		if (!PX_AbiSet_string(pmapabi, "variable_name", pidentifier_value))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}

		if (!PX_AbiSet_string(pmapabi, "variable_type", type))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}

		if (!PX_AbiSet_string(pmapabi, "variable_from", variable_from))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}

		if (!PX_AbiSet_dword(pmapabi, "variable_offset", (px_dword)offset))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}

		if (!PX_AbiSet_dword(pmapabi, "variable_size", (px_dword)type_size))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}

		

		if (!PX_StringInitialize(pSyntax->mp, &content))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand StringInitialize Memory Error");
			return PX_FALSE;
		}
		if (!PX_StringFormat4(&content, "[%1] from:%2 size:%3 offset:%4",\
			PX_STRINGFORMAT_STRING(type),\
			PX_STRINGFORMAT_STRING(variable_from),\
			 PX_STRINGFORMAT_INT(type_size),\
			PX_STRINGFORMAT_INT(offset)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand StringFormat Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}
		if (!PX_Syntax_SetLastMapInfo(pSyntax, source_index, PX_StringGetText(&content)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_parse_identifier_operand WriteIR Memory Error");
			PX_StringFree(&content);
			return PX_FALSE;
		}
		PX_StringFree(&content);


		PX_Syntax_PopLastSecondAbi(pSyntax);
	}
	else
	{
		return PX_FALSE;
	}
	
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_const_int_operand)
{
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* poperand;
	px_int begin, end, source_index;
	const px_char* pvalue;
	PX_ASSERTIFX(!pabi || PX_Syntax_CheckAbiName(pabi, "const_int") == PX_FALSE, "Unknow Error");
	//read every field from pabi BEFORE PushOperand: PushOperand inserts into reg_abi_stack
	//and can realloc the vector, leaving pabi (a pointer into the stack) dangling.
	pvalue = PX_AbiGet_string(pabi, "value");
	PX_ASSERTIF(pvalue == PX_NULL);
	source_index = PX_AbiGetValue_int(pabi, "source_index");
	begin = PX_AbiGetValue_int(pabi, "begin");
	end = PX_AbiGetValue_int(pabi, "end");
	//new abi operand
	poperand = PX_Syntax_PushOperand(pSyntax, "ix.const",PX_Syntax_GetMatchTypeSize(pSyntax,"ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!poperand)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_int_operand PushOperand Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(poperand, "value", pvalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_int_operand Set_string Memory Error");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewStaticMapToken(pSyntax, source_index, begin, source_index, end, PX_COLOR(255, 255, 255, 64), "ix.const"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_int_operand NewIRToken Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopLastSecondAbi(pSyntax);
	return PX_TRUE;

}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_const_string_operand)
{
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* poperand;
	const px_char* pvalue;
	px_memory payload_memory;
	px_int offset = 0,operand_value;
	PX_ASSERTIFX(!pabi || PX_Syntax_CheckAbiName(pabi, "const_string") == PX_FALSE, "Unknow Error");
	//read every field from pabi BEFORE PushOperand: PushOperand inserts into reg_abi_stack
	//and can realloc the vector, leaving pabi (a pointer into the stack) dangling.
	pvalue = PX_AbiGet_string(pabi, "value");
	PX_ASSERTIF(pvalue == PX_NULL);
	PX_MemoryInitialize(pSyntax->mp,&payload_memory);

	while (PX_TRUE)
	{
		if (pvalue[offset] == '\0')
		{
			break;
		}
		
		if (pvalue[offset]== '\\')
		{
			offset++;
			if (pvalue[offset] == 'n')
			{
				if (!PX_MemoryCat(&payload_memory, (px_byte*)"\n", 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}
			}
			else if (pvalue[offset] == 'r')
			{
				if (!PX_MemoryCat(&payload_memory, (px_byte*)"\r", 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}
			}
			else if (pvalue[offset] == 't')
			{
				if (!PX_MemoryCat(&payload_memory, (px_byte*)"\t", 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}
			}
			else if (pvalue[offset] == '0')
			{
				if (!PX_MemoryCat(&payload_memory, (px_byte*)"\0", 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}

			}
			else if (pvalue[offset] == 'x'||pvalue[offset] == 'X')
			{
				px_byte hex_value;
				px_char hex[3] = { 0 };
				if (!(PX_charIsHexadecimal(pvalue[offset + 1])&& PX_charIsHexadecimal(pvalue[offset + 2])))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:parser:error:PX_Syntax_expr_const_string_operand invalid hex escape sequence");
					return PX_FALSE;
				}
				hex[0] = pvalue[offset + 1];
				hex[1] = pvalue[offset + 2];
				offset += 2;
				hex_value = PX_htoi(hex);
				if (!PX_MemoryCat(&payload_memory, &hex_value, 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}
			}
			else if (pvalue[offset] == '\"'|| pvalue[offset] == '\\')
			{
				if (!PX_MemoryCat(&payload_memory, (px_byte*)&pvalue[offset], 1))
				{
					PX_MemoryFree(&payload_memory);
					PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
					return PX_FALSE;
				}
			}
			else
			{
				PX_MemoryFree(&payload_memory);
				PX_Syntax_Terminate(pSyntax, "ast:parser:error:PX_Syntax_expr_const_string_operand invalid escape sequence");
				return PX_FALSE;
			}
		}
		else
		{
			if (!PX_MemoryCat(&payload_memory, (px_byte*)&pvalue[offset], 1))
			{
				PX_MemoryFree(&payload_memory);
				PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Memory Error");
				return PX_FALSE;
			}
		}
		offset++;
	}

	operand_value=PX_Syntax_AllocRdata(pSyntax, PX_MemoryGetData(&payload_memory), PX_MemoryGetSize(&payload_memory));

	poperand = PX_Syntax_PushOperand(pSyntax, "array.ix.u.8", PX_MemoryGetSize(&payload_memory), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!poperand)
	{
		PX_MemoryFree(&payload_memory);
		PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand PushOperand Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(poperand, "value", operand_value))
	{
		PX_MemoryFree(&payload_memory);
		PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_expr_const_string_operand Set_int Memory Error");
		return PX_FALSE;
	}

	PX_MemoryFree(&payload_memory);
	PX_Syntax_PopLastSecondAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_const_float_operand)
{
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* poperand;
	px_int begin, end, source_index;
	const px_char* pvalue;
	PX_ASSERTIFX(!pabi || PX_Syntax_CheckAbiName(pabi, "const_float") == PX_FALSE, "Unknow Error");
	//read every field from pabi BEFORE PushOperand: PushOperand inserts into reg_abi_stack
	//and can realloc the vector, leaving pabi (a pointer into the stack) dangling.
	pvalue = PX_AbiGet_string(pabi, "value");
	PX_ASSERTIF(pvalue == PX_NULL);
	begin = PX_AbiGetValue_int(pabi, "begin");
	end = PX_AbiGetValue_int(pabi, "end");
	source_index = PX_AbiGetValue_int(pabi, "source_index");
	//new abi operand
	poperand = PX_Syntax_PushOperand(pSyntax, "fx.const",  PX_Syntax_GetMatchTypeSize(pSyntax,"fx.f.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!poperand)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_float_operand PushOperand Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(poperand, "value", pvalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_float_operand PX_AbiSet_string Memory Error");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewStaticMapToken(pSyntax, source_index, begin, source_index, end, PX_COLOR(255, 255, 255, 64),"fx.const"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_const_float_operand NewIRToken Memory Error");
		return PX_FALSE;
	}
	//pop
	PX_Syntax_PopLastSecondAbi(pSyntax);

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_expr_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:invalid expression");
	return PX_FALSE;
}

px_bool PX_Syntax_load_expr(PX_Syntax* pSyntax)
{
	//[expression]
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand ", 0, PX_Syntax_expr_update, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand = function_call", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand = identifier", 0, PX_Syntax_expr_parse_identifier_operand, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand = const_unsigned_float", 0, PX_Syntax_expr_const_float_operand, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand = const_unsigned_int", 0, PX_Syntax_expr_const_int_operand, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operand = const_string", 0, PX_Syntax_expr_const_string_operand, 0))
		return PX_FALSE;


	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_opcode_offset_container = expr_opcode_index_begin expr_cascade expr_opcode_index_end", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_opcode_offset_containers = expr_opcode_offset_container ...", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_opcode_offset_containers = expr_opcode_offset_container *", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "opcode_unary_suffixex = opcode_unary_suffix", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "opcode_unary_suffixex = expr_opcode_offset_containers", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = opcode_unary_prefix expr_operand opcode_unary_suffixex", 0, 0, 0)) //*1
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = opcode_unary_prefix expr_operand *", 0, 0, 0)) //*1
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = opcode_unary_prefix expr_operate_container opcode_unary_suffixex", 0, 0, 0)) //*(1)
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = opcode_unary_prefix expr_operate_container *", 0, 0, 0)) //*(1)
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = expr_operand opcode_unary_suffixex", 0, 0, 0)) //1*
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = expr_operand *", 0, 0, 0)) //(1)
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = expr_operate_container opcode_unary_suffixex", 0, 0, 0)) //(1)*
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_single = expr_operate_container *", 0, 0, 0)) //(1)*
		return PX_FALSE;


	

	//a|(..a)..* b|(..b)..*...
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade = expr_single opcode_binary ...", 0, 0, 0)) //a * b|(b) *.....
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade = expr_single *", 0, 0, 0))// a
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade = expr_operate_container opcode_binary ...", 0, 0, 0)) //(a)* b|(b)*.....
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade = expr_operate_container *", 0, 0, 0))// (a)
		return PX_FALSE;


	//a|(..a)..* b|(..b)..*...
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade_assignment = expr_single assign_opcode_binary ...", 0, 0, 0)) //a * b|(b) *.....
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade_assignment = expr_single *", 0, 0, 0))// a
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade_assignment = expr_operate_container assign_opcode_binary ...", 0, 0, 0)) //(a)* b|(b)*.....
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_cascade_assignment = expr_operate_container *", 0, 0, 0))// (a)
		return PX_FALSE;


	//a|(..a)..* b|(..b)..*... or (a|(..a)..* b|(..b)..*...)
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_operate_container = expr_opcode_begin expr_cascade expr_opcode_end", 0, 0, 0)) 
		return PX_FALSE;

	
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr", PX_Syntax_expr_begin, PX_Syntax_expr_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr= expr_cascade ", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "assign_expr", PX_Syntax_expr_begin, PX_Syntax_expr_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "assign_expr= expr_cascade_assignment ", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "expr_block = expr ';'", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	

	return PX_TRUE;
}



