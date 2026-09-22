#include "PX_Syntax_base_type.h"

PX_SYNTAX_FUNCTION(PX_Syntax_is_type_exec)
{
	px_int begin, end, begin_source_index, end_source_index;
	const px_char* ptype_mnemonic=PX_NULL,*ptype_define_type;
	px_abi * pnewabi;
	PX_SYNTAXLEXER_LEXEME_TYPE type = PX_Syntax_GetNextLexeme(pSyntax);
	if (type!=PX_SYNTAXLEXER_LEXEME_TYPE_TOKEN)
	{
		return PX_FALSE;
	}
	ptype_mnemonic = PX_Syntax_GetCurrentLexeme(pSyntax);
	if (PX_NULL==(ptype_define_type=PX_Syntax_GetTypeByMnemonic(pSyntax, ptype_mnemonic)))
	{
		return PX_FALSE;
	}
	
	pnewabi = PX_Syntax_NewAbi(pSyntax, "type");
	if (!pnewabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_is_type_exec Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pnewabi, "value", ptype_define_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_is_type_exec Memory Error2");
		return PX_FALSE;
	}

	while (PX_TRUE)
	{
		px_char payload[32] = { 0 };
		const px_char* pdeclare_prefix_value;
		px_abi* pdeclare_prefix_abi, * plast_abi;
		px_int declare_prefix_index = PX_Syntax_GetAbiIndexFromForward(pSyntax, "declare_prefix");
		if (declare_prefix_index==-1)
		{
			break;
		}
		pdeclare_prefix_abi = PX_Syntax_GetAbiByIndex(pSyntax, declare_prefix_index);
		plast_abi = PX_Syntax_GetLastAbi(pSyntax);
		pdeclare_prefix_value = PX_AbiGetValue_string(pdeclare_prefix_abi, "value");
		PX_Syntax_TypeDecorate(pSyntax, plast_abi, pdeclare_prefix_value);
		PX_Syntax_PopAbiIndex(pSyntax, declare_prefix_index);
	}


	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	if (begin_source_index != end_source_index)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:cross source type is not allowed");
		return PX_FALSE;
	}
	if (PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 199, 166, 230), ptype_define_type))
	{
		px_char content[128] = { 0 };
		PX_strcat_s(content,sizeof(content), ptype_mnemonic);
		PX_strcat_s(content, sizeof(content), " size:");
		PX_strcat_s(content, sizeof(content), PX_itos(PX_Syntax_GetMatchTypeSize(pSyntax, ptype_define_type), 10).data);
		if (!PX_Syntax_SetLastMapInfo(pSyntax, begin_source_index, content))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_is_type_exec Memory Error6");
			return PX_FALSE;
		}
	}
	return PX_TRUE;
}

const px_char* PX_Syntax_GetPointerBaseType(const px_char type_name[])
{
	px_int offset = 0;
	px_int counter = 0;
	if (!PX_Syntax_TypeMatch(type_name, "pointer"))
	{
		PX_ASSERTX("Error: PX_Syntax_GetPointerBaseType type_name is not pointer");
		return "";
	}
	while (type_name[offset] != 0)
	{
		if (type_name[offset] == '.')
		{
			if (counter==1)
			{
				return type_name + offset + 1;
			}
			else
			{
				counter++;
			}
		}
		offset++;
	}
	PX_ASSERTX("Error: PX_Syntax_GetPointerBaseType type_name is not pointer");
	return "";
	
}

const px_char* PX_Syntax_GetArrayBaseType(const px_char type_name[])
{
	px_int offset = 0;
	px_int counter = 0;
	if (!PX_Syntax_TypeMatch(type_name, "array"))
	{
		PX_ASSERTX("Error: PX_Syntax_GetArrayBaseType type_name is not array");
		return "";
	}
	while (type_name[offset] != 0)
	{
		if (type_name[offset] == '.')
		{
			if (counter == 1)
			{
				return type_name + offset + 1;
			}
			else
			{
				counter++;
			}
		}
		offset++;
	}
	PX_ASSERTX("Error: PX_Syntax_GetArrayBaseType type_name is not array");
	return "";
}

px_int PX_Syntax_GetArrayDimension(const px_char type_name[])
{
	px_int offset = 0;
	px_int counter = 0;
	if (!PX_Syntax_TypeMatch(type_name, "array"))
	{
		PX_ASSERTX("Error: PX_Syntax_GetArrayBaseType type_name is not array");
		return 0;
	}
	while (type_name[offset] != 0)
	{
		if (type_name[offset] == '.')
		{
			offset++;
			while (type_name[offset] != 0 && type_name[offset] != '.')
			{
				if (type_name[offset] == 'x')
				{
					counter++;
				}
				offset++;
			}
			return counter + 1;
		}
		offset++;
	}
	PX_ASSERTX("Error: PX_Syntax_GetArrayBaseType type_name is not array");
	return 0;
}

px_int PX_Syntax_GetTypePointerLevel(const px_char type_name[])
{
	px_char level_str[32] = { 0 };
	if (!PX_Syntax_TypeMatch(type_name,"pointer"))
	{
		return 0;
	}
	if (PX_strsubi(type_name, level_str, sizeof(level_str), '.', 1))
	{
		return PX_atoi(level_str);
	}
	return 0;
}

px_int PX_Syntax_GetArraySize(PX_Syntax* pSyntax, const px_char array_type_name[])
{
	const px_char* array_base_type;
	px_int base_type_size, array_count;
	if (!PX_Syntax_TypeMatch(array_type_name, "array"))
	{
		PX_ASSERTX("Error: PX_Syntax_GetArraySize array_type_name is not array");
		return 0;
	}
	array_base_type = PX_Syntax_GetArrayBaseType(array_type_name);
	array_count = PX_Syntax_array_parse_count(array_type_name);
	base_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, array_base_type);
	return base_type_size * array_count;
}

px_bool PX_Syntax_SetTypePointerLevel(px_string* ptype, px_int level)
{
	if (level==0)
	{
		if (PX_Syntax_TypeMatch(PX_StringGetText(ptype), "pointer"))
			PX_StringTrimLeftSubn(ptype, ".", 2);
	}
	else
	{
		px_char payload[64] = "pointer.";
		if (PX_Syntax_TypeMatch(PX_StringGetText(ptype), "pointer"))
			PX_StringTrimLeftSubn(ptype, ".", 2);
		else
		{
			PX_ASSERTIFX(PX_Syntax_TypeMatch(PX_StringGetText(ptype), "array"), "PX_Syntax_SetTypePointerLevel: the array is a higher-level type than a pointer, and an array type cannot be converted to a pointer type.");
		}
		PX_strcat(payload, PX_itos(level, 10).data);
		PX_strcat(payload, ".");
		return PX_StringInsert(ptype, 0, payload);
	}
	
	return PX_TRUE;
}

px_bool PX_Syntax_load_base_type(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "type = *", 0, PX_Syntax_is_type_exec, 0))
		goto _ERROR;

	if (!PX_Syntax_load_base_opcode(pSyntax))goto _ERROR;

	return PX_TRUE;
_ERROR:
	return PX_FALSE;
}



PX_SYNTAX_REGISTER_FUNCTION(PX_Syntax_ix_fx_pointer_register_function)
{
	px_char rx[3] = "rx";
	px_abi* operand = PX_Syntax_GetAbiByIndex(pSyntax, operand_index);
	const px_char* op_type = PX_AbiGetValue_string(operand, "type");
	px_int op_type_size = PX_AbiGetValue_int(operand, "type_size");
	PX_SYNTAX_OPERAND_FROM op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand, "from"));
	rx[1] = '0' + register_index;
	switch (op_from)
	{
	case PX_SYNTAX_OPERAND_FROM_CONST:
	{
		if (PX_Syntax_TypeMatch(op_type, "fx.const"))
		{
			const px_char* value = PX_AbiGetValue_string(operand, "value");
			if (!PX_Syntax_NewIRInstruction2(pSyntax, pabi_name, "movf", rx, value))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_MapOperandToRegister out of memory err1");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(op_type, "ix.const"))
		{
			const px_char* value = PX_AbiGetValue_string(operand, "value");
			if (!PX_Syntax_NewIRInstruction2(pSyntax, pabi_name, "mov", rx, value))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_MapOperandToRegister out of memory err2");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport const operand type");
			return PX_FALSE;
		}
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_STACK:
	{
		if (!PX_Syntax_NewIRInstruction1(pSyntax, pabi_name, "pop", rx))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err10");
			return PX_FALSE;
		}
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_MEMBER:
	{
		px_int offset = PX_AbiGetValue_int(operand, "offset");
		if (!PX_Syntax_NewIRInstructionFormat2(pSyntax, pabi_name, "store32 r%1,bp+8\nadd r0,%2\nstore r%1", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err10");
			return PX_FALSE;
		}
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_REFERENCE:
	{
		const px_char* opcode_instr;
		if (!PX_Syntax_NewIRInstruction1(pSyntax, pabi_name, "pop", rx))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err10");
			return PX_FALSE;
		}

		if (PX_Syntax_TypeMatch3(op_type, "ix.u", "fx.f", "pointer"))
		{
			switch (op_type_size)
			{
			case 4:
				opcode_instr = "loadu32";
				break;
			case 2:
				opcode_instr = "loadu16";
				break;
			case 1:
				opcode_instr = "loadu8";
				break;
			default:
				PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(op_type, "ix.i"))
		{
			switch (op_type_size)
			{
			case 4:
				opcode_instr = "loadi32";
				break;
			case 2:
				opcode_instr = "loadi16";
				break;
			case 1:
				opcode_instr = "loadi8";
				break;
			default:
				PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
			return PX_FALSE;
		}

		if (!PX_Syntax_NewIRInstruction1(pSyntax, pabi_name, opcode_instr, rx))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err11");
			return PX_FALSE;
		}
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
	case PX_SYNTAX_OPERAND_FROM_RDATA:
	case PX_SYNTAX_OPERAND_FROM_GLOBAL:
	case PX_SYNTAX_OPERAND_FROM_LOCAL:
	case PX_SYNTAX_OPERAND_FROM_PARAM:
	case PX_SYNTAX_OPERAND_FROM_TEMP:
	{
		if (PX_Syntax_TypeMatch3(op_type, "fx", "ix", "pointer"))
		{
			//eg ix.i.8, ix.u.16, fx.f.32 pointer
			const px_char* opcode_instr = "";
			px_char operand2_str[16] = { 0 };
			px_int offset = PX_AbiGetValue_int(operand, "offset");
			if (op_from == PX_SYNTAX_OPERAND_FROM_MODULEBASE)
				PX_sprintf1(operand2_str, sizeof(operand2_str), "mp+%1", PX_STRINGFORMAT_INT(offset));
			else if (op_from == PX_SYNTAX_OPERAND_FROM_RDATA)
				PX_sprintf1(operand2_str, sizeof(operand2_str), "rp+%1", PX_STRINGFORMAT_INT(offset));
			else if (op_from == PX_SYNTAX_OPERAND_FROM_GLOBAL)
				PX_sprintf1(operand2_str, sizeof(operand2_str), "gp+%1", PX_STRINGFORMAT_INT(offset));
			else if (op_from == PX_SYNTAX_OPERAND_FROM_LOCAL|| op_from == PX_SYNTAX_OPERAND_FROM_TEMP)
				PX_sprintf1(operand2_str, sizeof(operand2_str), "bp-%1", PX_STRINGFORMAT_INT(offset + PX_AbiGetValue_int(operand, "type_size")));
			else if (op_from == PX_SYNTAX_OPERAND_FROM_PARAM)
				PX_sprintf1(operand2_str, sizeof(operand2_str), "bp+%1", PX_STRINGFORMAT_INT(8+offset));
			else
			{
				PX_ASSERTX("Error:unsupport operand from");
				PX_Syntax_Terminate(pSyntax, "ast:error:unsupport operand from");
				return PX_FALSE;
			}
			if (PX_Syntax_TypeMatch3(op_type, "ix.u", "fx.f", "pointer"))
			{
				switch (op_type_size)
				{
				case 4:
					opcode_instr = "loadu32";
					break;
				case 2:
					opcode_instr = "loadu16";
					break;
				case 1:
					opcode_instr = "loadu8";
					break;
				default:
					PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
					return PX_FALSE;
				}
			}
			else if (PX_Syntax_TypeMatch(op_type, "ix.i"))
			{
				switch (op_type_size)
				{
				case 4:
					opcode_instr = "loadi32";
					break;
				case 2:
					opcode_instr = "loadi16";
					break;
				case 1:
					opcode_instr = "loadi8";
					break;
				default:
					PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
					return PX_FALSE;
				}
			}
			else
			{
				PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
				return PX_FALSE;
			}
			if (!PX_Syntax_NewIRInstruction2(pSyntax, pabi_name, opcode_instr, rx, operand2_str))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err15");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport variable operand type");
			return PX_FALSE;
		}
	}
	break;
	default:
		PX_Syntax_Terminate(pSyntax, "ast:error:unsupport operand type");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_MEMORY_FUNCTION(PX_Syntax_ix_fx_pointer_memory_function)
{
	px_int offset,type_size;
	PX_SYNTAX_OPERAND_FROM op_from;
	px_abi* operand = PX_Syntax_GetAbiByIndex(pSyntax, operand_index);
	const px_char* popcode_mnemonic,*poperand_type;
	px_char payload[64] = {0};
	px_char rx[3] = "rx";
	rx[1] = '0' + register_index;
	PX_ASSERTIFX(!operand||!PX_Syntax_CheckAbiName(operand,"operand"), "Error: PX_Syntax_ix_fx_pointer_memory_function operand is null");
	
	op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand, "from"));
	poperand_type = PX_AbiGet_string(operand, "type");
	type_size = PX_AbiGetValue_int(operand, "type_size");

	if (type_size == 1)
		popcode_mnemonic = "store8";
	else if (type_size == 2)
		popcode_mnemonic = "store16";
	else if (type_size == 4)
		popcode_mnemonic = "store32";
	else
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type size");
		return PX_FALSE;
	}
	
	if (register_index!=1)
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, pabi_name, "mov","r1",rx))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err");
			return PX_FALSE;
		}
	}

	switch (op_from)
	{
	case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,mp+%1", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_RDATA:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,rp+%1", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_LOCAL:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,bp-%1",PX_STRINGFORMAT_INT(offset + type_size));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_GLOBAL:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,gp+%1", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_PARAM:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,bp+%1",  PX_STRINGFORMAT_INT(8+offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_TEMP:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,bp-%1",  PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_REFERENCE:
	{
		PX_sprintf0(payload, sizeof(payload), "pop r0");
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_MEMBER:
	{
		px_int offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf1(payload, sizeof(payload), "load32 r0,bp+8\nadd r0,%1", PX_STRINGFORMAT_INT(offset));
	}
	break;
	default:
	{
		PX_ASSERTX("Error:unsupport operand from");
		PX_Syntax_Terminate(pSyntax, "ast:error:unsupport operand from");
		return PX_FALSE;
	}
	break;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, pabi_name,payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstruction2(pSyntax, pabi_name, popcode_mnemonic,"r0","r1"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_REGISTER_FUNCTION(PX_Syntax_array_struct_register_function)
{
	px_int offset, type_size;
	PX_SYNTAX_OPERAND_FROM op_from;
	px_abi* operand = PX_Syntax_GetAbiByIndex(pSyntax, operand_index);
	px_char payload[64];
	PX_ASSERTIFX(!operand || !PX_Syntax_CheckAbiName(operand, "operand"), "Error: PX_Syntax_ix_fx_pointer_memory_function operand is null");
	
	op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand, "from"));
	type_size = PX_AbiGetValue_int(operand, "type_size");
	switch (op_from)
	{
		case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
		{
			offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "mov r%1,mp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_RDATA:
		{
			offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "mov r%1,rp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_GLOBAL:
		{
			offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "mov r%1,gp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_LOCAL:
		case PX_SYNTAX_OPERAND_FROM_TEMP:
		{
			offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "mov r%1,bp-%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset + type_size));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_PARAM:
		{
			offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "mov r%1,bp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(8+offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_REFERENCE:
		{
			PX_sprintf1(payload, sizeof(payload), "pop r%1", PX_STRINGFORMAT_INT(register_index));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_MEMBER:
		{
			px_int offset = PX_AbiGetValue_int(operand, "offset");
			PX_sprintf2(payload, sizeof(payload), "load32 r%1,bp+8\nadd r%1,%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_CONST:
		default:
		{
			PX_ASSERTX("Error:unsupport operand from");
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport operand from");
			return PX_FALSE;
		}
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, pabi_name, payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_struct_register_function out of memory err");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_MEMORY_FUNCTION(PX_Syntax_array_struct_memory_function)
{
	px_int offset, type_size;
	PX_SYNTAX_OPERAND_FROM op_from;
	px_abi* operand = PX_Syntax_GetAbiByIndex(pSyntax, operand_index);
	px_char payload[64];
	px_char rx[] = "rx";
	const px_char* ptype;
	PX_ASSERTIFX(!operand || !PX_Syntax_CheckAbiName(operand, "operand"), "Error: PX_Syntax_struct_memory_function operand is null");
	ptype = PX_AbiGetValue_string(operand, "type");
	op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand, "from"));
	type_size = PX_AbiGetValue_int(operand, "type_size");
	rx[1] = '0' + register_index;
	
	switch (op_from)
	{
	case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "mov r%1,mp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_RDATA:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "mov r%1,rp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_GLOBAL:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "mov r%1,gp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_LOCAL:
	case PX_SYNTAX_OPERAND_FROM_TEMP:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "mov r%1,bp-%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset + type_size));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_PARAM:
	{
		offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "mov r%1,bp+%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(8+offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_REFERENCE:
	{
		PX_sprintf1(payload, sizeof(payload), "pop r%1", PX_STRINGFORMAT_INT(register_index));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_MEMBER:
	{
		px_int offset = PX_AbiGetValue_int(operand, "offset");
		PX_sprintf2(payload, sizeof(payload), "load32 r%1,bp+8\nadd r%1,%2", PX_STRINGFORMAT_INT(register_index), PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_CONST:
	default:
		PX_ASSERTX("Error:unsupport operand from");
		PX_Syntax_Terminate(pSyntax, "ast:error:unsupport operand from");
		return PX_FALSE;
		break;
	}

	//size 
	if (!PX_Syntax_NewIRInstructions(pSyntax, pabi_name, payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_struct_register_function out of memory err");
		return PX_FALSE;
	}
	PX_sprintf1(payload, sizeof(payload), "mov r2,%1", PX_STRINGFORMAT_INT(type_size));
	if (!PX_Syntax_NewIRInstructions(pSyntax, pabi_name, payload)|| !PX_Syntax_NewIRInstructions(pSyntax, pabi_name, "movn"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_struct_register_function out of memory err");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_int PX_Syntax_array_parse_count(const px_char payload[])
{
	px_int count = 1, factor = 0;
	px_bool has_digit = PX_FALSE;
	const px_char* p = payload;
	
	PX_ASSERTIFX(!payload, "Error: PX_Syntax_array_parse_count payload is null"); 
	PX_ASSERTIFX(!PX_memequ(payload, "array.", 6), "Error: PX_Syntax_array_parse_count payload is not array");
	p += 6;
	while (PX_TRUE)
	{
		if (PX_charIsNumeric(*p))
		{
			factor = factor * 10 + (*p - '0');
			has_digit = PX_TRUE;
			p++;
			continue;
		}
		if (*p == 'x' || *p == '\0'|| *p == '.')
		{
			if (!has_digit || factor <= 0) return 0;
			count *= factor;
			if (*p == '\0'|| *p == '.') break;
			factor = 0;
			has_digit = PX_FALSE;
			p++;
			continue;
		}
		else
		{
			PX_ASSERTX("Error: PX_Syntax_array_parse_count payload is not valid array");
		}
		return 0;
	}
	return count;
}



PX_SYNTAX_TYPE_SIZE_FUNCTION(PX_Syntax_array_size_function)
{
	px_int base_type_size;
	const px_char* pbase_type = PX_Syntax_GetArrayBaseType(type_name);
	base_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, pbase_type);
	PX_ASSERTIFX(base_type_size<= 0, "Error: PX_Syntax_array_size_function base_type_size <= 0");
	return PX_Syntax_array_parse_count(type_name) * base_type_size;
}

PX_SYNTAX_TYPE_DECORATE_FUNCTION(PX_Syntax_ixfx_decorate_const)
{
	if (!PX_AbiAppend_string(ptype_declare_abi, "value", ".const"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ixfx_decorate_const out of memory err");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_Syntax_init_base_type(PX_Syntax* pSyntax)
{
	px_int operate_index;
	///////////////////////////////////////////////////////////////////////
	//type define
	///////////////////////////////////////////////////////////////////////
	if (!PX_Syntax_NewType(pSyntax, "ix.u.8", "u8", 1,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.u.16", "u16", 2,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.u.32", "u32", 4,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.i.8", "i8",  1,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.i.16", "i16",  2,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.i.32", "i32",  4,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "ix.const", "", 4,PX_NULL)) goto _ERROR;

	if (!PX_Syntax_NewTypeDecorate(pSyntax, "ix.i", "const", PX_Syntax_ixfx_decorate_const)) goto _ERROR;
	if (!PX_Syntax_NewTypeDecorate(pSyntax, "ix.u", "const", PX_Syntax_ixfx_decorate_const)) goto _ERROR;

	if (!PX_Syntax_NewType(pSyntax, "fx.f.32", "f32",  4,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "fx.const", "", 4,PX_NULL)) goto _ERROR;

	if (!PX_Syntax_NewTypeDecorate(pSyntax, "fx.f", "const", PX_Syntax_ixfx_decorate_const)) goto _ERROR;

	if (!PX_Syntax_NewType(pSyntax, "void", "void", 4,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "struct", "", 0,PX_NULL)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "array", "", 0, PX_Syntax_array_size_function)) goto _ERROR;
	if (!PX_Syntax_NewType(pSyntax, "pointer", "", 4,PX_NULL)) goto _ERROR;

	if (!PX_Syntax_NewTypeRegisterMemoryMap(pSyntax, "ix", PX_Syntax_ix_fx_pointer_register_function, PX_Syntax_ix_fx_pointer_memory_function)) goto _ERROR;
	if (!PX_Syntax_NewTypeRegisterMemoryMap(pSyntax, "fx", PX_Syntax_ix_fx_pointer_register_function, PX_Syntax_ix_fx_pointer_memory_function)) goto _ERROR;
	if (!PX_Syntax_NewTypeRegisterMemoryMap(pSyntax, "pointer", PX_Syntax_ix_fx_pointer_register_function, PX_Syntax_ix_fx_pointer_memory_function)) goto _ERROR;
	if (!PX_Syntax_NewTypeRegisterMemoryMap(pSyntax, "struct", PX_Syntax_array_struct_register_function, PX_Syntax_array_struct_memory_function)) goto _ERROR;
	if (!PX_Syntax_NewTypeRegisterMemoryMap(pSyntax, "array", PX_Syntax_array_struct_register_function, PX_Syntax_array_struct_memory_function)) goto _ERROR;


	if (!PX_Syntax_NewTypedef(pSyntax, "ix.u.32", "bool")) goto _ERROR;
	if (!PX_Syntax_NewTypedef(pSyntax, "ix.i.32", "int")) goto _ERROR;
	if (!PX_Syntax_NewTypedef(pSyntax, "fx.f.32", "float")) goto _ERROR;


	///////////////////////////////////////////////////////////////////////
	//operation define
	///////////////////////////////////////////////////////////////////////
	//&lvalue
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "&", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: unary opcode & not found");
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_address_of)) goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "fx", PX_Syntax_operate_address_of)) goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "pointer", PX_Syntax_operate_address_of)) goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "struct", PX_Syntax_operate_address_of)) goto _ERROR;

	//dereference *(pointer)
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "*", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode * not found");
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_dereference))goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "fx", PX_Syntax_operate_dereference))goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "struct", PX_Syntax_operate_dereference))goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "pointer", PX_Syntax_operate_dereference))goto _ERROR;

	//+ ix
	//- ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "+", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode + not found");
	if(!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_positive_negative_ixfx))goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "fx", PX_Syntax_operate_positive_negative_ixfx))goto _ERROR;
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "-", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode - not found");
    if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_positive_negative_ixfx)) goto _ERROR;
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "fx", PX_Syntax_operate_positive_negative_ixfx)) goto _ERROR;


	//ix = ix
	//ix = fx
	//fx = ix
	//fx = fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "=", PX_SYNTAX_OPCODE_TYPE_BINARY);
	if (operate_index == -1) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_opcode_ixfx_assign_ixfx)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_opcode_ixfx_assign_ixfx)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_opcode_ixfx_assign_ixfx)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_opcode_ixfx_assign_ixfx)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "pointer", "pointer", PX_Syntax_opcode_pointer_assign)) goto _ERROR;

	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.i.8", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.i.16", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.i.32", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.u.8", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.u.16", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "ix.u.32", PX_Syntax_convert_auto)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "ix", "fx.f.32", PX_Syntax_convert_ix_to_fx)) goto _ERROR;

	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx.const", "fx.f.32", PX_Syntax_convert_auto)) goto _ERROR;

	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.i.8", PX_Syntax_convert_fx_to_ix)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.i.16", PX_Syntax_convert_fx_to_ix)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.i.32", PX_Syntax_convert_fx_to_ix)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.u.8", PX_Syntax_convert_fx_to_ix)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.u.16", PX_Syntax_convert_fx_to_ix)) goto _ERROR;
	if (!PX_Syntax_NewTypeConvert(pSyntax, "fx", "ix.u.32", PX_Syntax_convert_fx_to_ix)) goto _ERROR;

	//[]
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "[*]", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode [] not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "array", "ix", PX_Syntax_operate_array_dereference_offset)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "pointer", "ix", PX_Syntax_operate_pointer_offset)) goto _ERROR;

	//ix + ix
	//ix + fx
	//fx + ix
	//fx + fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "+", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode + not found");
	//runtime: ix/fx + ix/fx
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "pointer", "ix", PX_Syntax_operate_pointer_add_sub_ix)) goto _ERROR;
	//ix - ix
	//ix - fx
	//fx - ix
	//fx - fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "-", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode - not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "pointer", "ix", PX_Syntax_operate_pointer_add_sub_ix)) goto _ERROR;
	//ix * ix
	//ix * fx
	//fx * ix
	//fx * fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "*", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode * not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;

	//ix / ix
	//ix / fx
	//fx / ix
	//fx / fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "/", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode / not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_add_sub_mul_div)) goto _ERROR;

	//ix % ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "%", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode % not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_mod)) goto _ERROR;

	//ix << ix
	//ix >> ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "<<", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode << not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_shl_shr)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, ">>", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode >> not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_shl_shr)) goto _ERROR;

	//ix > ix/fx, fx > ix/fx
	//ix < ix/fx, fx < ix/fx
	//ix >= ix/fx, fx >= ix/fx
	//ix <= ix/fx, fx <= ix/fx
	//ix == ix/fx, fx == ix/fx
	//ix != ix/fx, fx != ix/fx
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, ">", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode > not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "<", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode < not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "==", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode == not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "!=", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode != not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, ">=", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode >= not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "<=", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode <= not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "ix", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "fx", "fx", PX_Syntax_operate_ixfx_cmp)) goto _ERROR;

	//ix & ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "&", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode & not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_bitand_bitor_bitxor)) goto _ERROR;

	//ix ^ ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "^", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode ^ not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_bitand_bitor_bitxor)) goto _ERROR;

	//ix | ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "|", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode | not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_bitand_bitor_bitxor)) goto _ERROR;

	//ix && ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "&&", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode && not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_logical_and_or)) goto _ERROR;

	//ix || ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "||", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode || not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "ix", "ix", PX_Syntax_operate_ix_logical_and_or)) goto _ERROR;

	//!ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "!", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode ! not found");
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_ix_logical_not)) goto _ERROR;

	//~ix
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "~", PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode ~ not found");
	if (!PX_Syntax_NewUnaryPrefixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_ix_bitwise_not)) goto _ERROR;

	//ix++
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "++", PX_SYNTAX_OPCODE_TYPE_UNARY_SUFFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode ++ not found");
	if (!PX_Syntax_NewUnarySuffixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_ix_inc)) goto _ERROR;

	//ix--
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "--", PX_SYNTAX_OPCODE_TYPE_UNARY_SUFFIX);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode -- not found");
	if (!PX_Syntax_NewUnarySuffixOperate(pSyntax, operate_index, "ix", PX_Syntax_operate_ix_dec)) goto _ERROR;

	//struct.identifier
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, ".", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode . not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "struct", "identifier", PX_Syntax_operate_struct_member)) goto _ERROR;

	//pointer.struct->identifier
	operate_index = PX_Syntax_GetOpcodeDefineIndex(pSyntax, "->", PX_SYNTAX_OPCODE_TYPE_BINARY);
	PX_ASSERTIFX(operate_index == -1, "Error: opcode -> not found");
	if (!PX_Syntax_NewBinaryOperate(pSyntax, operate_index, "pointer", "identifier", PX_Syntax_operate_pointer_struct_offset)) goto _ERROR;

	return PX_TRUE;
_ERROR:
	return PX_FALSE;
}
