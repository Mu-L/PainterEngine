#include "PX_Syntax_base_operate.h"

px_bool PX_Syntax_NewOperate(PX_Syntax* pSyntax,px_int opcode_index, const px_char type1[], const px_char type2[], const px_char type3[], PX_Syntax_Operate_Function opfun)
{
	PX_Syntax_opcode* popcode = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, opcode_index);
	px_int operate_count, new_operate_count = \
		(popcode->type == PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX || popcode->type == PX_SYNTAX_OPCODE_TYPE_UNARY_SUFFIX ?1 :\
			(popcode->type == PX_SYNTAX_OPCODE_TYPE_BINARY ? 2 : (popcode->type == PX_SYNTAX_OPCODE_TYPE_TERNARY ? 3 : 0)));
	px_string payload;
	px_int i,scope_index;
	px_abi* pscope_abi;

	if (new_operate_count>2)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport operate count");
		return PX_FALSE;
	}

	if(!PX_StringInitialize(pSyntax->mp, &payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewOperate payload initialization failed");
		goto _ERROR;
	}
	scope_index = PX_Syntax_GetTypeScopeAbiIndex(pSyntax, type1);
	if (scope_index==-1)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewOperate type1 scope not found");
		goto _ERROR;
	}
	pscope_abi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, scope_index);
	PX_StringFormat1(&payload, "type_defines.%1.operates", PX_STRINGFORMAT_STRING(type1));
	operate_count = PX_AbiGet_PayloadMemberCount(pscope_abi, PX_StringGetText(&payload));
	//find exist?
	for (i = 0; i < operate_count; i++)
	{
		px_abi type_define_operate_abi_readonly;
		px_int opcode_define_index;
		PX_StringFormat2(&payload, "type_defines.%1.operates.[%2]", PX_STRINGFORMAT_STRING(type1),PX_STRINGFORMAT_INT(i));
		type_define_operate_abi_readonly = PX_AbiGetValue_abireadonly(pscope_abi, PX_StringGetText(&payload));
		opcode_define_index = PX_AbiGetValue_int(&type_define_operate_abi_readonly, "opcode_index");
		if (opcode_define_index == opcode_index)
		{
			if (new_operate_count == 1)
			{
				PX_Syntax_Terminate(pSyntax, "ast:error:operate exist");
				return PX_FALSE;
			}
			else if (new_operate_count == 2)
			{
				const px_char* pother_type = PX_AbiGet_string(&type_define_operate_abi_readonly, "other_type");
				if (PX_strequ(type2, pother_type))
				{
					PX_Syntax_Terminate(pSyntax, "ast:error:operate exist");
					return PX_FALSE;
				}
			}
			else
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport operate count");
				return PX_FALSE;
			}
		}
		
	}
	//new operate type
	PX_StringFormat2(&payload, "type_defines.%1.operates.[%2].opcode_index", PX_STRINGFORMAT_STRING(type1), PX_STRINGFORMAT_INT(operate_count));
	if (!PX_AbiSet_int(pscope_abi, PX_StringGetText(&payload), opcode_index))
	{
		goto _ERROR;
	}

	//function
	PX_StringFormat2(&payload, "type_defines.%1.operates.[%2].function", PX_STRINGFORMAT_STRING(type1), PX_STRINGFORMAT_INT(operate_count));
	if(!PX_AbiSet_ptr(pscope_abi, PX_StringGetText(&payload), opfun))
	{
		goto _ERROR;
	}

	//other type
	if (new_operate_count == 2)
	{
		PX_StringFormat2(&payload, "type_defines.%1.operates.[%2].other_type", PX_STRINGFORMAT_STRING(type1), PX_STRINGFORMAT_INT(operate_count));
		if (!PX_AbiSet_string(pscope_abi, PX_StringGetText(&payload), type2))
		{
			goto _ERROR;
		}
	}

	//a type define which owns an operate function is an activated type
	PX_StringFormat1(&payload, "type_defines.%1.activate", PX_STRINGFORMAT_STRING(type1));
	if (!PX_AbiSet_bool(pscope_abi, PX_StringGetText(&payload), PX_TRUE))
	{
		goto _ERROR;
	}

	PX_StringFree(&payload);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&payload);
	return PX_FALSE;
}

px_void PX_Syntax_PopOperate2(PX_Syntax* pSyntax, px_int index1, px_int index2)
{
	px_int max, min;
	max = index1 > index2 ? index1 : index2;
	min = index1 < index2 ? index1 : index2;
	PX_ASSERTIFX(max == min, "Error:pop unary operate operand and opcode failed");
	PX_ASSERTIFX(max < 0 || max >= pSyntax->reg_abi_stack.size, "Error:pop unary operate operand and opcode failed");
	PX_ASSERTIFX(min < 0 || min >= pSyntax->reg_abi_stack.size, "Error:pop unary operate operand and opcode failed");
	PX_Syntax_PopAbiIndex(pSyntax, max);
	PX_Syntax_PopAbiIndex(pSyntax, min);
}

px_void PX_Syntax_PopLastAbiName(PX_Syntax* pSyntax, const px_char name[])
{
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, name))
		{
			PX_Syntax_PopAbiIndex(pSyntax, i);
			return;
		}
	}
	PX_ASSERTX( "Error:pop last abi name failed");
}


px_void PX_Syntax_PopOperate3(PX_Syntax* pSyntax, px_int index1, px_int index2, px_int index3)
{
	px_int max_to_min[3];
	px_int i, j, temp;
	max_to_min[0] = index1;
	max_to_min[1] = index2;
	max_to_min[2] = index3;
	//sort
	for (i = 0; i < 3 - 1; i++)
	{
		for (j = 0; j < 3 - i - 1; j++)
		{
			if (max_to_min[j] < max_to_min[j + 1])
			{
				temp = max_to_min[j];
				max_to_min[j] = max_to_min[j + 1];
				max_to_min[j + 1] = temp;
			}
		}
	}
	PX_ASSERTIFX(max_to_min[0] == max_to_min[1] || max_to_min[1] == max_to_min[2] || max_to_min[0] == max_to_min[2], "Error:pop binary operate operand and opcode failed");
	PX_ASSERTIFX(max_to_min[0] < 0 || max_to_min[0] >= pSyntax->reg_abi_stack.size, "Error:pop binary operate operand and opcode failed");
	PX_ASSERTIFX(max_to_min[1] < 0 || max_to_min[1] >= pSyntax->reg_abi_stack.size, "Error:pop binary operate operand and opcode failed");
	PX_ASSERTIFX(max_to_min[2] < 0 || max_to_min[2] >= pSyntax->reg_abi_stack.size, "Error:pop binary operate operand and opcode failed");
	for (i = 0; i < 3; i++)
		PX_Syntax_PopAbiIndex(pSyntax, max_to_min[i]);
}


px_bool PX_Syntax_NewBinaryOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], const px_char type2[], PX_Syntax_Operate_Function opfun)
{
	return PX_Syntax_NewOperate(pSyntax, opcode_index, type1, type2, "", opfun);
}

px_bool PX_Syntax_NewBeginEndOperate(PX_Syntax* pSyntax, px_int opcode_index, PX_Syntax_Operate_Function opfun)
{
	return PX_Syntax_NewOperate(pSyntax, opcode_index,  "", "", "", opfun);
}

px_bool PX_Syntax_NewUnaryPrefixOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], PX_Syntax_Operate_Function opfun)
{
	return PX_Syntax_NewOperate(pSyntax, opcode_index,  type1, "", "", opfun);
}

px_bool PX_Syntax_NewUnarySuffixOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], PX_Syntax_Operate_Function opfun)
{
	return PX_Syntax_NewOperate(pSyntax, opcode_index,  type1, "", "", opfun);
}


px_bool PX_Syntax_GetStructMemberAbi(PX_Syntax* pSyntax, px_abi* pir_abi, const px_char struct_type[], const px_char member_name[])
{
	px_abi structabi;
	if (PX_Syntax_GetTypeAbi(pSyntax, struct_type, &structabi))
	{
		return PX_AbiGet_AbiReadOnly(&structabi, pir_abi, member_name);
	}
	return PX_FALSE;
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_positive_negative_ixfx)
{
	const px_char* poperand_type;
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	px_abi* popcode_abi = PX_Syntax_GetAbiByIndex(pSyntax, opcode_abi_index);
	px_int index = PX_AbiGetValue_int(popcode_abi, "index");
	PX_Syntax_opcode* pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, index);

	PX_ASSERTIFX(!pdefine_opcode, "Unknow positive_negative Error");
	poperand_type= PX_AbiGet_string(poperand_abi, "type");
	PX_ASSERTIFX(!poperand_type, "Unknow positive_negative Error");
	if (pdefine_opcode->opcode[0] == '+')
	{
		//remove opcode
		PX_Syntax_PopAbiIndex(pSyntax, opcode_abi_index);

		return PX_TRUE;
	}
	else if (pdefine_opcode->opcode[0] == '-')
	{
		if (PX_Syntax_TypeMatch2(poperand_type, "ix.const", "fx.const"))
		{
			px_char value[64] = { 0 };
			const px_char* pvalue = PX_AbiGet_string(poperand_abi, "value");
			if (pvalue[0] == '-')
			{
				PX_strcpy(value, pvalue + 1, sizeof(value));
			}
			else
			{
				value[0] = '-';
				PX_strcpy(value + 1, pvalue, sizeof(value) - 1);
			}
			if (!PX_AbiSet_string(poperand_abi, "value", value))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
				return PX_FALSE;
			}
			PX_Syntax_PopAbiIndex(pSyntax, opcode_abi_index);
			return PX_TRUE;
		}
		else if (PX_Syntax_TypeMatch(poperand_type, "fx"))
		{
			if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index,0))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error1");
				return PX_FALSE;
			}
			//r0->f0
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "f0", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error2");
				return PX_FALSE;
			}

			//f0=-f0
			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "fneg", "f0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error3");
				return PX_FALSE;
			}

			//f0->r0
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "r0", "f0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error4");
				return PX_FALSE;
			}

			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error3");
				return PX_FALSE;
			}

			//pop operand and push r0
			
			if (!PX_Syntax_PushOperand(pSyntax, poperand_type, PX_Syntax_GetMatchTypeSize(pSyntax, poperand_type), PX_SYNTAX_OPERAND_FROM_STACK))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error5");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(poperand_type, "ix"))
		{
			if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index,0))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error6");
				return PX_FALSE;
			}
			//r0=-r0
			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "neg", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error7");
				return PX_FALSE;
			}
			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error8");
				return PX_FALSE;
			}
			//pop operand and push r0
			
			if (!PX_Syntax_PushOperand(pSyntax, poperand_type, PX_Syntax_GetMatchTypeSize(pSyntax, poperand_type), PX_SYNTAX_OPERAND_FROM_STACK))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_positive_negative_ixuxfx Memory Error9");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type for positive_negative");
			return PX_FALSE;
		}
		//remove opcode
		PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
		return PX_TRUE;
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:Unknow positive_negative Error");
		return PX_FALSE;
	}

}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_const_folding_add_sub_mul_div)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* type1, * type2;
	const px_char* value1, * value2; 
	px_int opcode_index;
	px_char final_value[64] = { 0 };
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	PX_SYNTAX_OPCODE_TYPE opcode_type;
	px_int precedence;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	PX_VECTOR_CHECK_RANGE(&pSyntax->reg_abi_stack, opcode_abi_index);
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);

	popcode_str = pdefine_opcode->opcode;
	opcode_type = pdefine_opcode->type;
	precedence = pdefine_opcode->precedence;

	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");
	if (PX_strlen(value1)>=32)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:Const value too long");
		return PX_FALSE;
	}
	if (PX_strlen(value2) >= 32)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:Const value too long");
		return PX_FALSE;
	}

	PX_ASSERTIFX(!PX_Syntax_TypeMatch(type1, "ix.const") && !PX_Syntax_TypeMatch(type1, "fx.const"), "unsupport type");

	if(popcode_str[0]=='+')
	{
		//add
		if (!PX_StringNumeric_add(value1, value2, final_value, sizeof(final_value)))
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Const folding add error");
			return PX_FALSE;
		}
	}
	else if (popcode_str[0] == '-')
	{
		//sub
		if (!PX_StringNumeric_sub(value1, value2, final_value, sizeof(final_value)))
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Const folding sub error");
			return PX_FALSE;
		}
	}
	else if (popcode_str[0] == '*')
	{
		//mul
		if (!PX_StringNumeric_mul(value1, value2, final_value, sizeof(final_value)))
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Const folding mul error");
			return PX_FALSE;
		}
	}
	else if (popcode_str[0] == '/')
	{
		//div
		if(PX_atof(value2)==0)
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Const folding div zero error");
			return PX_FALSE;
		}

		if (!PX_StringNumeric_div2(value1, value2, final_value, sizeof(final_value)))
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Const folding div error");
			return PX_FALSE;
		}
	}
	else
	{
	PX_Syntax_Terminate(pSyntax, "runtime:error:Const folding unsupport opcode");
		return PX_FALSE;
	}
	
	if (PX_strequ(type1,"fx.const")|| PX_strequ(type2, "fx.const"))
	{
		pabi_result = PX_Syntax_PushOperand(pSyntax, "fx.const", PX_Syntax_GetMatchTypeSize(pSyntax,"fx.f.32"), PX_SYNTAX_OPERAND_FROM_CONST);
		if (!pabi_result)
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_const_folding_add_sub_mul_div Memory Error1");
			return PX_FALSE;
		}
		// Set the result value
		if (!PX_AbiSet_string(pabi_result, "value", final_value))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_const_folding_add_sub_mul_div Memory Error2");
			return PX_FALSE;
		}
	}
	else 
	{
		pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
		if (!pabi_result)
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_const_folding_add_sub_mul_div Memory Error3");
			return PX_FALSE;
		}
		// Set the result value
		if (!PX_AbiSet_string(pabi_result, "value", final_value))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_const_folding_add_sub_mul_div Memory Error4");
			return PX_FALSE;
		}
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_opcode_pointer_assign)
{
	px_abi* poperand1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	const px_char* poperand1_type = PX_AbiGet_string(poperand1, "type");

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_pointer_assign map operand2 to register failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_OperandToMemory(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_pointer_assign map operand1 to memory failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstructionFormat0(pSyntax, "expr", "push r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_pointer_assign push result operand failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_PushOperand(pSyntax, poperand1_type, PX_Syntax_GetMatchTypeSize(pSyntax, poperand1_type), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_pointer_assign push result operand failed");
		return PX_FALSE;
	}

	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);

	return PX_TRUE;
}

PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_auto)
{
	return PX_TRUE;
}

PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_ix_to_fx)
{
	px_abi* pirabi = PX_Syntax_GetAbiFromBackward(pSyntax, pabi_name);
	const px_char* ppayload;
	if (!pirabi)
	{
		return PX_FALSE;
	}
	if (register_index == 0)
		ppayload =	"i2f f0,r0\nmov r0,f0\n";
	else if (register_index == 1)
		ppayload = "i2f f1,r1\nmov r1,f1\n";
	else if (register_index == 2)
		ppayload = "i2f f2,r2\nmov r2,f2\n";
	else if (register_index == 3)
		ppayload = "i2f f3,r3\nmov r3,f3\n";
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport register index");
		return PX_FALSE;
	}
	if (!PX_AbiAppend_string(pirabi, "ir", ppayload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_fx_to_ix)
{
	px_abi* pirabi = PX_Syntax_GetAbiFromBackward(pSyntax, pabi_name);
	const px_char* ppayload;
	if (!pirabi)
	{
		return PX_FALSE;
	}
	if (register_index == 0)
		ppayload = "mov f0,r0\nf2i r0,f0\n";
	else if (register_index == 1)
		ppayload = "mov f1,r1\nf2i r1,f1\n";
	else if (register_index == 2)
		ppayload = "mov f2,r2\nf2i r2,f2\n";
	else if (register_index == 3)
		ppayload = "mov f3,r3\nf2i r3,f3\n";
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport register index");
		return PX_FALSE;
	}
	if (!PX_AbiAppend_string(pirabi, "ir", ppayload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_opcode_ixfx_assign_ixfx)
{
	px_abi* operand1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	px_abi* operand2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	const px_char* op1_type = PX_AbiGet_string(operand1, "type");
	const px_char* op2_type = PX_AbiGet_string(operand2, "type");
	px_int op1_type_size = PX_AbiGetValue_int(operand1, "type_size");
	px_int op2_type_size = PX_AbiGetValue_int(operand2, "type_size");
	PX_SYNTAX_OPERAND_FROM op1_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand1, "from"));
	PX_SYNTAX_OPERAND_FROM op2_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(operand2, "from"));


	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_ixfx_assign_ixfx map operand2 to register failed");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_ConvertType(pSyntax, "expr", 0, op2_type, op1_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_ixfx_assign_ixfx convert operand2 to operand1 type failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_OperandToMemory(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_opcode_ixfx_assign_ixfx map operand1 to memory failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push","r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory err5");
		return PX_FALSE;
	}
	
	//push result operand
	if (!PX_Syntax_PushOperand(pSyntax, op1_type,op1_type_size,PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Opcode_pointer_equ out of memory");
		return PX_FALSE;
	}

	//pop operate stack
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);

	return PX_TRUE;
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ixfx_add_sub_mul_div)
{
	px_abi* pabi1, *pabi2;
	const px_char* type1, *type2;
	px_int type_size1, type_size2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	const px_char* result_type;
	px_int result_type_size;

	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;

	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;

	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;

	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	type_size1 = PX_AbiGetValue_int(pabi1, "type_size");
	type_size2 = PX_AbiGetValue_int(pabi2, "type_size");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}

	// now r0=operand1, r1=operand2
	if (PX_Syntax_TypeMatch(type1, "fx") || PX_Syntax_TypeMatch(type2, "fx"))
	{
		// float operation: convert both to float, use f0/f1
		if (PX_Syntax_TypeMatch(type1, "fx"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "f0", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err3");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(type1, "ix.i")||PX_Syntax_TypeMatch(type1, "ix.const"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "i2f", "f0", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err4");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(type1, "ix.u"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "u2f", "f0", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err5");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type for runtime add_sub_mul_div");
			return PX_FALSE;
		}

		if (PX_Syntax_TypeMatch(type2, "fx"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "f1", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err6");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(type2, "ix.i")||PX_Syntax_TypeMatch(type2, "ix.const"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "i2f", "f1", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err7");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_TypeMatch(type2, "ix.u"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "u2f", "f1", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err8");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport type for runtime add_sub_mul_div");
			return PX_FALSE;
		}

		// f0 op f1 -> f0
		if (popcode_str[0] == '+')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "fadd", "f0", "f1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err9");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '-')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "fsub", "f0", "f1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err10");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '*')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "fmul", "f0", "f1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err11");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '/')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "fdiv", "f0", "f1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err12");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport float opcode");
			return PX_FALSE;
		}

		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "r0", "f0"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err13");
			return PX_FALSE;
		}

		result_type = "fx.f.32";
		result_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, "fx.f.32");
	}
	else
	{
		// integer operation: r0 op r1 -> r0
		if (popcode_str[0] == '+')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "add", "r0", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err14");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '-')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "sub", "r0", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err15");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '*')
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mul", "r0", "r1"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err16");
				return PX_FALSE;
			}
		}
		else if (popcode_str[0] == '/')
		{
			if (PX_Syntax_TypeMatch(type1, "ix.u") && PX_Syntax_TypeMatch(type2, "ix.u"))
			{
				// both operands unsigned -> unsigned divide
				if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "div", "r0", "r1"))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err17");
					return PX_FALSE;
				}
			}
			else
			{
				// signed divide (default for signed int)
				if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "idiv", "r0", "r1"))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err18");
					return PX_FALSE;
				}
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport integer opcode");
			return PX_FALSE;
		}

		result_type = type1;
		result_type_size = type_size1;
	}

	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err19");
		return PX_FALSE;
	}

	if (!PX_Syntax_PushOperand(pSyntax, result_type, result_type_size, PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_runtime_add_sub_mul_div out of memory err20");
		return PX_FALSE;
	}

	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);

	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_add_sub_mul_div)
{
	px_abi* pabi1, *pabi2;
	const px_char* type1, *type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");

	if (PX_Syntax_TypeMatch2(type1,"fx.const", "ix.const") && PX_Syntax_TypeMatch2(type2, "fx.const", "ix.const"))
	{
		return PX_Syntax_operate_ixfx_const_folding_add_sub_mul_div(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	else
	{
		return PX_Syntax_operate_runtime_ixfx_add_sub_mul_div(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_add_sub_ix)
{
	px_abi* pabi1, * pabi2, * popcode_abi, * presult;
	const px_char* type1, * type2, * popcode_str, * pointee_type;
	px_int opcode_index, pointee_size, result_type_size;
	PX_Syntax_opcode* pdefine_opcode;
	px_string result_type;

	PX_ASSERTIFX(operand1_abi_index < 0 || operand2_abi_index < 0 || opcode_abi_index < 0, "unknown error");

	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	popcode_abi = PX_Syntax_GetAbiByIndex(pSyntax, opcode_abi_index);
	if (!pabi1 || !pabi2 || !popcode_abi)
		return PX_FALSE;

	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (!PX_Syntax_TypeMatch(type1, "pointer") || !PX_Syntax_TypeMatch(type2, "ix"))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:pointer arithmetic requires pointer and integer operands");
		return PX_FALSE;
	}
	PX_ASSERTIFX(type1[7] != '.' || type1[8] == '\0', "ast:error:pointer arithmetic requires a pointed type");

	opcode_index = PX_AbiGetValue_int(popcode_abi, "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	if (!pdefine_opcode || (!PX_strequ(pdefine_opcode->opcode, "+") && !PX_strequ(pdefine_opcode->opcode, "-")))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport pointer arithmetic opcode");
		return PX_FALSE;
	}
	popcode_str = pdefine_opcode->opcode;

	if (!PX_StringInitialize(pSyntax->mp, &result_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:pointer arithmetic out of memory");
		return PX_FALSE;
	}
	if (!PX_StringSet(&result_type, type1))
	{
		PX_StringFree(&result_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:pointer arithmetic out of memory");
		return PX_FALSE;
	}

	pointee_type = PX_Syntax_GetPointerBaseType(type1);
	pointee_size = PX_Syntax_GetMatchTypeSize(pSyntax, pointee_type);
	if (pointee_size <= 0)
	{
		PX_StringFree(&result_type);
		PX_Syntax_Terminate(pSyntax, "ast:error:pointer arithmetic requires a complete pointed type");
		return PX_FALSE;
	}

	if (operand2_abi_index > operand1_abi_index)
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1) ||
			!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
		{
			PX_StringFree(&result_type);
			return PX_FALSE;
		}
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0) ||
			!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1))
		{
			PX_StringFree(&result_type);
			return PX_FALSE;
		}
	}

	if (pointee_size == 0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:pointer arithmetic requires a complete pointed type");
		PX_StringFree(&result_type);
		return PX_FALSE;
	}

	if (pointee_size != 1 &&
		!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mul", "r1", PX_itos(pointee_size, 10).data))
	{
		PX_StringFree(&result_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:pointer arithmetic out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", popcode_str[0] == '+' ? "add" : "sub", "r0", "r1") ||
		!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_StringFree(&result_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:pointer arithmetic out of memory");
		return PX_FALSE;
	}

	result_type_size = PX_AbiGetValue_int(pabi1, "type_size");
	PX_ASSERTIFX(result_type_size <= 0,"runtime:error:pointer type size is not defined");

	presult = PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&result_type), result_type_size, PX_SYNTAX_OPERAND_FROM_STACK);
	PX_StringFree(&result_type);
	if (!presult)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:pointer arithmetic out of memory");
		return PX_FALSE;
	}

	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_array_add_ix)
{
	px_abi* poperand1, * poperand2;
	const px_char* type1, * type2,*pbase_type;
	px_int base_type_size;
	px_string fmt;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	poperand1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	poperand2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	type1 = PX_AbiGetValue_string(poperand1, "type");
	type2 = PX_AbiGetValue_string(poperand2, "type");
	PX_ASSERTIFX(!PX_Syntax_TypeMatch(type1, "array") || !PX_Syntax_TypeMatch(type2, "ix"), "ast:error:array arithmetic requires array and integer operands");
	if (PX_Syntax_GetArrayDimension(type1) != 1)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:array arithmetic requires one-dimensional array");
		return PX_FALSE;
	}
	pbase_type = PX_Syntax_GetArrayBaseType(type1);
	base_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, pbase_type);
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	if (!PX_Syntax_NewIRInstructionFormat1(pSyntax, "expr", "mul r1,%1\nadd r0,r1", PX_STRINGFORMAT_INT(base_type_size)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:array arithmetic out of memory");
		return PX_FALSE;
	}
	if (!PX_StringInitializeFormat0(pSyntax->mp,&fmt,type1))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:array arithmetic out of memory");
		return PX_FALSE;
	}
	PX_StringTrimLeftSubn(&fmt, ".", 2);
	if (!PX_StringInsert(&fmt, 0, "pointer.1."))
	{
		PX_StringFree(&fmt);
		PX_Syntax_Terminate(pSyntax, "runtime:error:array arithmetic out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&fmt), PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&fmt)), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:array arithmetic out of memory");
		PX_StringFree(&fmt);
		return PX_FALSE;
	}
	PX_StringFree(&fmt);
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_const_folding_mod)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* value1, * value2;
	px_int v1, v2, result;
	px_char final_value[64] = { 0 };
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");
	v1 = PX_atoi(value1);
	v2 = PX_atoi(value2);
	if (v2 == 0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:Const folding mod zero error");
		return PX_FALSE;
	}
	result = v1 % v2;
	PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
	pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!pabi_result)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding mod memory error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi_result, "value", final_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding mod memory error2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ix_mod)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}
	if (PX_Syntax_TypeMatch(type1, "ix.u") && PX_Syntax_TypeMatch(type2, "ix.u"))
	{
		// both operands unsigned -> unsigned modulo
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mod", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:mod out of memory");
			return PX_FALSE;
		}
	}
	else
	{
		// signed modulo (default for signed int)
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "imod", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:imod out of memory");
			return PX_FALSE;
		}
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:mod out of memory2");
		return PX_FALSE;
	}
	if (!PX_Syntax_PushOperand(pSyntax, type1, PX_Syntax_GetMatchTypeSize(pSyntax, type1), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:mod out of memory3");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_mod)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (PX_Syntax_TypeMatch(type1, "ix.const") && PX_Syntax_TypeMatch(type2, "ix.const"))
	{
		return PX_Syntax_operate_ix_const_folding_mod(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	return PX_Syntax_operate_runtime_ix_mod(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
}


static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_const_folding_shl_shr)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* value1, * value2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	px_int v1, v2, result;
	px_char final_value[64] = { 0 };
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");
	v1 = PX_atoi(value1);
	v2 = PX_atoi(value2);
	if (PX_strequ(popcode_str, "<<"))
		result = v1 << v2;
	else
		result = v1 >> v2;
	PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
	pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!pabi_result)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding shl_shr memory error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi_result, "value", final_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding shl_shr memory error2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ix_shl_shr)
{
	px_abi* pabi1;
	const px_char* type1;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	if (!pabi1) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}

	if (PX_strequ(popcode_str, "<<"))
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "shl", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:shl out of memory");
			return PX_FALSE;
		}
	}
	else
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "shr", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:shr out of memory");
			return PX_FALSE;
		}
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:shl_shr out of memory");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_PushOperand(pSyntax, type1, PX_Syntax_GetMatchTypeSize(pSyntax, type1), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:shl_shr out of memory2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_shl_shr)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (PX_Syntax_TypeMatch(type1, "ix.const") && PX_Syntax_TypeMatch(type2, "ix.const"))
	{
		return PX_Syntax_operate_ix_const_folding_shl_shr(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	return PX_Syntax_operate_runtime_ix_shl_shr(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
}


static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_const_folding_bitand_bitor_bitxor)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* value1, * value2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	px_int v1, v2, result;
	px_char final_value[64] = { 0 };
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");
	v1 = PX_atoi(value1);
	v2 = PX_atoi(value2);
	if (popcode_str[0] == '&')
		result = v1 & v2;
	else if (popcode_str[0] == '|')
		result = v1 | v2;
	else
		result = v1 ^ v2;
	PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
	pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!pabi_result)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding bitop memory error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi_result, "value", final_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding bitop memory error2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ix_bitand_bitor_bitxor)
{
	px_abi* pabi1;
	const px_char* type1;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	if (!pabi1) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}
	if (popcode_str[0] == '&')
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "and", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:bitand out of memory");
			return PX_FALSE;
		}
	}
	else if (popcode_str[0] == '|')
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "or", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:bitor out of memory");
			return PX_FALSE;
		}
	}
	else
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "xor", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:bitxor out of memory");
			return PX_FALSE;
		}
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:bitop out of memory");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_PushOperand(pSyntax, type1, PX_Syntax_GetMatchTypeSize(pSyntax, type1), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:bitop out of memory2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_bitand_bitor_bitxor)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (PX_Syntax_TypeMatch(type1, "ix.const") && PX_Syntax_TypeMatch(type2, "ix.const"))
	{
		return PX_Syntax_operate_ix_const_folding_bitand_bitor_bitxor(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	return PX_Syntax_operate_runtime_ix_bitand_bitor_bitxor(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
}


static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_const_folding_cmp)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* type1, * type2;
	const px_char* value1, * value2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	px_int result;
	px_char final_value[64] = { 0 };
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");

	if (PX_Syntax_TypeMatch(type1, "fx.const") || PX_Syntax_TypeMatch(type2, "fx.const"))
	{
		px_float f1 = PX_atof(value1);
		px_float f2 = PX_atof(value2);
		if (PX_strequ(popcode_str, ">")) result = f1 > f2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "<")) result = f1 < f2 ? 1 : 0;
		else if (PX_strequ(popcode_str, ">=")) result = f1 >= f2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "<=")) result = f1 <= f2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "==")) result = f1 == f2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "!=")) result = f1 != f2 ? 1 : 0;
		else { PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport cmp opcode"); return PX_FALSE; }
	}
	else
	{
		px_int i1 = PX_atoi(value1);
		px_int i2 = PX_atoi(value2);
		if (PX_strequ(popcode_str, ">")) result = i1 > i2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "<")) result = i1 < i2 ? 1 : 0;
		else if (PX_strequ(popcode_str, ">=")) result = i1 >= i2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "<=")) result = i1 <= i2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "==")) result = i1 == i2 ? 1 : 0;
		else if (PX_strequ(popcode_str, "!=")) result = i1 != i2 ? 1 : 0;
		else { PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport cmp opcode"); return PX_FALSE; }
	}
	PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
	pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!pabi_result)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding cmp memory error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi_result, "value", final_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding cmp memory error2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ixfx_cmp)
{
	px_abi* pabi1, * pabi2;
	px_bool float_cmp = PX_FALSE;
	px_bool unsigned_cmp = PX_FALSE;
	const px_char* setcc_opcode;
	const px_char* type1, * type2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}
	

	if (PX_Syntax_TypeMatch(type1, "fx") || PX_Syntax_TypeMatch(type2, "fx"))
	{
		float_cmp = PX_TRUE;
		if (PX_Syntax_TypeMatch(type1, "fx"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "f0", "r0")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else if (PX_Syntax_TypeMatch(type1, "ix.i")|| PX_Syntax_TypeMatch(type1, "ix.const"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "i2f", "f0", "r0")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else if (PX_Syntax_TypeMatch(type1, "ix.u"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "u2f", "f0", "r0")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport cmp operand type1");
			return PX_FALSE;
		}

		if (PX_Syntax_TypeMatch(type2, "fx"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "mov", "f1", "r1")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else if (PX_Syntax_TypeMatch(type2, "ix.i")|| PX_Syntax_TypeMatch(type2, "ix.const"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "i2f", "f1", "r1")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else if (PX_Syntax_TypeMatch(type2, "ix.u"))
		{
			if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "u2f", "f1", "r1")) { PX_Syntax_Terminate(pSyntax, "runtime:error:cmp mem"); return PX_FALSE; }
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:unsupport cmp operand type2");
			return PX_FALSE;
		}

	}
	else
	{
		unsigned_cmp = (px_bool)(PX_Syntax_TypeMatch(type1, "ix.u") || PX_Syntax_TypeMatch(type2, "ix.u"));
	}

	//set-compare writes the 0/1 result straight into r0
	if (PX_strequ(popcode_str, ">"))
	{
		setcc_opcode = float_cmp ? "fgt" : (unsigned_cmp ? "ugt" : "gt");
	}
	else if (PX_strequ(popcode_str, "<"))
	{
		setcc_opcode = float_cmp ? "flt" : (unsigned_cmp ? "ult" : "lt");
	}
	else if (PX_strequ(popcode_str, ">="))
	{
		setcc_opcode = float_cmp ? "fge" : (unsigned_cmp ? "uge" : "ge");
	}
	else if (PX_strequ(popcode_str, "<="))
	{
		setcc_opcode = float_cmp ? "fle" : (unsigned_cmp ? "ule" : "le");
	}
	else if (PX_strequ(popcode_str, "=="))
	{
		//eq/neq are sign agnostic
		setcc_opcode = float_cmp ? "feq" : "eq";
	}
	else if (PX_strequ(popcode_str, "!="))
	{
		setcc_opcode = float_cmp ? "fneq" : "neq";
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:unsupport cmp opcode");
		return PX_FALSE;
	}

	if (float_cmp)
	{
		if (!PX_Syntax_NewIRInstruction3(pSyntax, "expr", setcc_opcode, "r0", "f0", "f1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:cmp result memory error");
			return PX_FALSE;
		}
	}
	else
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", setcc_opcode, "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:cmp result memory error");
			return PX_FALSE;
		}
	}

	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:cmp out of memory4");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_PushOperand(pSyntax, "ix.i.32", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:cmp out of memory5");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_cmp)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (PX_Syntax_TypeMatch2(type1, "fx.const", "ix.const") && PX_Syntax_TypeMatch2(type2, "fx.const", "ix.const"))
	{
		return PX_Syntax_operate_ixfx_const_folding_cmp(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	return PX_Syntax_operate_runtime_ixfx_cmp(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
}


static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_const_folding_logical_and_or)
{
	px_abi* pabi1, * pabi2, * pabi_result;
	const px_char* value1, * value2;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	px_int v1, v2, result;
	px_char final_value[64] = { 0 };
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	value1 = PX_AbiGetValue_string(pabi1, "value");
	value2 = PX_AbiGetValue_string(pabi2, "value");
	v1 = PX_atoi(value1);
	v2 = PX_atoi(value2);
	if (PX_strequ(popcode_str, "&&"))
		result = (v1 && v2) ? 1 : 0;
	else
		result = (v1 || v2) ? 1 : 0;
	PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
	pabi_result = PX_Syntax_PushOperand(pSyntax, "ix.const", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_CONST);
	if (!pabi_result)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding logical memory error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(pabi_result, "value", final_value))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:const folding logical memory error2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

static PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_runtime_ix_logical_and_or)
{
	px_abi* pabi1;
	const px_char* type1;
	px_int opcode_index;
	PX_Syntax_opcode* pdefine_opcode;
	const px_char* popcode_str;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	opcode_index = PX_AbiGetValue_int(PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, opcode_abi_index), "index");
	pdefine_opcode = PX_Syntax_GetOpcodeDefine(pSyntax, opcode_index);
	popcode_str = pdefine_opcode->opcode;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	if (!pabi1) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");

	if (operand2_abi_index > operand1_abi_index) // pop order: operand2, operand1
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	}
	else
	{
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
		if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) return PX_FALSE;
	}
	if (PX_strequ(popcode_str, "&&"))
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "andl", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:logical_and out of memory");
			return PX_FALSE;
		}
	}
	else
	{
		if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "orl", "r0", "r1"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:logical_or out of memory");
			return PX_FALSE;
		}
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:logical out of memory");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_PushOperand(pSyntax, "ix.i.32", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:logical out of memory2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_logical_and_or)
{
	px_abi* pabi1, * pabi2;
	const px_char* type1, * type2;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	type1 = PX_AbiGetValue_string(pabi1, "type");
	type2 = PX_AbiGetValue_string(pabi2, "type");
	if (PX_Syntax_TypeMatch(type1, "ix.const") && PX_Syntax_TypeMatch(type2, "ix.const"))
	{
		return PX_Syntax_operate_ix_const_folding_logical_and_or(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	}
	return PX_Syntax_operate_runtime_ix_logical_and_or(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_logical_not)
{
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	const px_char* poperand_type;
	if (!poperand_abi) return PX_FALSE;
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");

	if (PX_Syntax_TypeMatch(poperand_type, "ix.const"))
	{
		const px_char* value = PX_AbiGetValue_string(poperand_abi, "value");
		px_int v = PX_atoi(value);
		px_int result = v ? 0 : 1;
		px_char final_value[64] = { 0 };
		PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
		if (!PX_AbiSet_string(poperand_abi, "value", final_value))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:logical_not memory error");
			return PX_FALSE;
		}
		PX_Syntax_PopAbiIndex(pSyntax, opcode_abi_index);
		return PX_TRUE;
	}

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "not", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:logical_not out of memory");
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:logical_not out of memory2");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	if (!PX_Syntax_PushOperand(pSyntax, "ix.i.32", PX_Syntax_GetMatchTypeSize(pSyntax, "ix.i.32"), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:logical_not out of memory3");
		return PX_FALSE;
	}
	return PX_TRUE;
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_bitwise_not)
{
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	const px_char* poperand_type;
	if (!poperand_abi) return PX_FALSE;
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");

	if (PX_Syntax_TypeMatch(poperand_type, "ix.const"))
	{
		const px_char* value = PX_AbiGetValue_string(poperand_abi, "value");
		px_int v = PX_atoi(value);
		px_int result = ~v;
		px_char final_value[64] = { 0 };
		PX_strcpy(final_value, PX_itos(result, 10).data, sizeof(final_value));
		if (!PX_AbiSet_string(poperand_abi, "value", final_value))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:bitwise_not memory error");
			return PX_FALSE;
		}
		PX_Syntax_PopAbiIndex(pSyntax, opcode_abi_index);
		return PX_TRUE;
	}

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) return PX_FALSE;
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "inv", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:bitwise_not out of memory");
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:bitwise_not out of memory2");
		return PX_FALSE;
	}
	//push result before pop: poperand_type points into operand1's buffer, which PopOperate2 frees
	if (!PX_Syntax_PushOperand(pSyntax, poperand_type, PX_Syntax_GetMatchTypeSize(pSyntax, poperand_type), PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:bitwise_not out of memory3");
		return PX_FALSE;
	}
	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	return PX_TRUE;
}


PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_inc)
{
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	const px_char* poperand_type;
	px_int operand_size;
	PX_ASSERTIFX(!poperand_abi || !PX_Syntax_CheckAbiName(poperand_abi, "operand"), "PX_Syntax_operate_ix_dec should not run here");
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");
	operand_size = PX_AbiGetValue_int(poperand_abi, "type_size");
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_inc could not convert operand to register");
		goto _ERROR;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, "expr", "add r0,1\npush r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_inc out of memory");
		goto _ERROR;
	}
	if (!PX_Syntax_OperandToMemory(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_inc could not convert operand to memory");
		goto _ERROR;
	}
	if (!PX_Syntax_PushOperand(pSyntax, poperand_type, operand_size, PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_inc could not push operand to stack");
		goto _ERROR;
	}

	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	return PX_TRUE;
_ERROR:
	return PX_FALSE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_dec)
{
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	const px_char* poperand_type;
	px_int operand_size;
	PX_ASSERTIFX(!poperand_abi || !PX_Syntax_CheckAbiName(poperand_abi, "operand"), "PX_Syntax_operate_ix_dec should not run here");
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");
	operand_size = PX_AbiGetValue_int(poperand_abi, "type_size");
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_dec could not convert operand to register");
		goto _ERROR;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, "expr", "sub r0,1\npush r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_dec out of memory");
		goto _ERROR;
	}
	if (!PX_Syntax_OperandToMemory(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_dec could not convert operand to memory");
		goto _ERROR;
	}
	if (!PX_Syntax_PushOperand(pSyntax, poperand_type, operand_size, PX_SYNTAX_OPERAND_FROM_STACK))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ix_dec could not push operand to stack");
		goto _ERROR;
	}

	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	return PX_TRUE;
_ERROR:
	return PX_FALSE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_struct_member)
{
	px_abi* pabi1, * pabi2;
	px_abi structabi = {0};
	PX_SYNTAX_OPERAND_FROM op_from;
	const px_char* struct_type, * member_type,*member_name;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	struct_type = PX_AbiGetValue_string(pabi1, "type");
	member_type = PX_AbiGetValue_string(pabi2, "type");
	op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(pabi1, "from"));
	PX_ASSERTIFX(!PX_Syntax_TypeMatch(struct_type, "struct") || !PX_Syntax_TypeMatch(member_type, "identifier"), "PX_Syntax_operate_struct_member should not run here");
	member_name = PX_AbiGetValue_string(pabi2, "value");
	if (PX_Syntax_GetTypeAbi(pSyntax, struct_type, &structabi))
	{
		px_abi memberabi;
		if (PX_AbiGet_AbiReadOnly(&structabi,&memberabi,member_name))
		{
			const px_char* member_type = PX_AbiGetValue_string(&memberabi, "type");
			px_int type_size = PX_Syntax_GetMatchTypeSize(pSyntax, member_type);
			px_int member_offset = PX_AbiGetValue_int(&memberabi, "offset");
			if (!PX_Syntax_PushOperand(pSyntax, member_type, type_size, PX_SYNTAX_OPERAND_FROM_REFERENCE))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory");
				return PX_FALSE;
			}
			if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory1");
				return PX_FALSE;
			}
			if (member_offset)
			{
				if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "add", "r0", PX_itos(member_offset, 10).data))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory2");
					return PX_FALSE;
				}
			}
			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory2");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:struct member not found");
			return PX_FALSE;
		}
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:struct member not found");
		return PX_FALSE;
	}

	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;

}
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_struct_offset)
{
	px_abi* pabi1, * pabi2;
	px_abi structabi = { 0 };
	PX_SYNTAX_OPERAND_FROM op_from;
	const px_char* struct_type, * member_type, * member_name;
	const px_char* operand1_type, * operand2_type;
	if (operand1_abi_index < 0 || operand2_abi_index < 0) return PX_FALSE;
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!pabi1 || !pabi2) return PX_FALSE;
	operand1_type = PX_AbiGetValue_string(pabi1, "type");
	operand2_type = PX_AbiGetValue_string(pabi2, "type");
	struct_type = PX_Syntax_GetPointerBaseType(operand1_type);
	member_type = operand2_type;
	op_from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(pabi1, "from"));
	PX_ASSERTIFX(!PX_Syntax_TypeMatch(struct_type, "struct") || !PX_Syntax_TypeMatch(member_type, "identifier"), "PX_Syntax_operate_pointer_struct_offset should not run here");
	member_name = PX_AbiGetValue_string(pabi2, "value");
	if (PX_Syntax_GetTypeAbi(pSyntax, struct_type, &structabi))
	{
		px_abi memberabi;
		if (PX_AbiGet_AbiReadOnly(&structabi, &memberabi, member_name))
		{
			const px_char* member_type = PX_AbiGetValue_string(&memberabi, "type");
			px_int type_size = PX_Syntax_GetMatchTypeSize(pSyntax, member_type);
			px_int member_offset = PX_AbiGetValue_int(&memberabi, "offset");
			
			if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory1");
				return PX_FALSE;
			}

			if (member_offset)
			{
				if (!PX_Syntax_NewIRInstruction2(pSyntax, "expr", "add", "r0", PX_itos(member_offset, 10).data))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory2");
					return PX_FALSE;
				}
			}

			if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory2");
				return PX_FALSE;
			}

			if (!PX_Syntax_PushOperand(pSyntax, member_type, type_size, PX_SYNTAX_OPERAND_FROM_REFERENCE))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:struct member out of memory");
				return PX_FALSE;
			}
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:struct member not found");
			return PX_FALSE;
		}
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:struct member not found");
		return PX_FALSE;
	}

	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_address_of)
{
	PX_SYNTAX_OPERAND_FROM from;
	px_string final_type;
	px_char payload[64]={0};
	px_int ptr_level;
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	px_abi* pnewoperand;
	const px_char* poperand_type;
	PX_ASSERTIFX(!poperand_abi || !PX_Syntax_CheckAbiName(poperand_abi, "operand"), "PX_Syntax_operate_address_of should not run here");
	from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(poperand_abi, "from"));
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");

	switch (from)
	{
		case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			PX_sprintf1(payload, sizeof(payload), "mov r0,mp+%1\npush r0", PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_RDATA:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			PX_sprintf1(payload, sizeof(payload), "mov r0,rp+%1\npush r0", PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_PARAM:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			PX_sprintf1(payload, sizeof(payload), "mov r0,bp+%1\npush r0", PX_STRINGFORMAT_INT(8+offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_GLOBAL:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			PX_sprintf1(payload, sizeof(payload), "mov r0,gp+%1\npush r0", PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_TEMP:
		case PX_SYNTAX_OPERAND_FROM_LOCAL:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			px_int size = PX_AbiGetValue_int(poperand_abi, "type_size");
			PX_sprintf1(payload, sizeof(payload), "mov r0,bp-%1\npush r0", PX_STRINGFORMAT_INT(offset+size));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_STACK:
		{
			PX_sprintf0(payload, sizeof(payload), "mov r0,sp\npush r0");
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_REFERENCE: 
			// do nothing, the reference is already an address
			break;
		case PX_SYNTAX_OPERAND_FROM_MEMBER:
		{
			px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
			PX_sprintf1(payload, sizeof(payload), "load32 r0,bp+8\nadd r0,%1\npush r0", PX_STRINGFORMAT_INT(offset));
		}
		break;
		case PX_SYNTAX_OPERAND_FROM_CONST:
		default:
			PX_Syntax_Terminate(pSyntax, "runtime:error:address_of unsupport operand from");
			return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, "expr", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:address_of out of memory");
		return PX_FALSE;
	}

	if (!PX_StringInitialize(pSyntax->mp, &final_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:address_of out of memory");
		return PX_FALSE;
	}
	if (!PX_StringSet(&final_type, poperand_type))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:address_of out of memory");
		return PX_FALSE;
	}
	ptr_level = PX_Syntax_GetTypePointerLevel(poperand_type) + 1;

	if (!PX_Syntax_SetTypePointerLevel(&final_type,ptr_level))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:address_of out of memory");
		return PX_FALSE;
	}

	pnewoperand = PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&final_type), PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&final_type)), PX_SYNTAX_OPERAND_FROM_STACK);
	PX_StringFree(&final_type);
	if (!pnewoperand)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:address_of out of memory");
		return PX_FALSE;
	}
	
	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_dereference)
{
	PX_SYNTAX_OPERAND_FROM from;
	const px_char* operand_type;
	px_string final_type;
	px_char payload[64] = {0};
	px_int ptr_level;
	px_abi* poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	px_abi* pnewoperand;
	
	PX_ASSERTIFX(!poperand_abi || !PX_Syntax_CheckAbiName(poperand_abi, "operand"), "PX_Syntax_operate_dereference should not run here");
	operand_type = PX_AbiGetValue_string(poperand_abi, "type");
	PX_ASSERTIFX(!PX_Syntax_TypeMatch(operand_type, "pointer"), "PX_Syntax_operate_dereference should not run here");
	
	from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(poperand_abi, "from"));
	ptr_level = PX_Syntax_GetTypePointerLevel(operand_type);

	//push address to stack
	switch (from)
	{
	case PX_SYNTAX_OPERAND_FROM_MODULEBASE:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,mp+%1\nload32 r0\npush r0", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_RDATA:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,rp+%1\nload32 r0\npush r0", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_PARAM:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,bp+%1\nload32 r0\npush r0", PX_STRINGFORMAT_INT(8+offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_GLOBAL:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		PX_sprintf1(payload, sizeof(payload), "mov r0,gp+%1\nload32 r0\npush r0", PX_STRINGFORMAT_INT(offset));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_TEMP:
	case PX_SYNTAX_OPERAND_FROM_LOCAL:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		px_int size = PX_AbiGetValue_int(poperand_abi, "type_size");
		PX_sprintf1(payload, sizeof(payload), "mov r0,bp-%1\nload32 r0\npush r0", PX_STRINGFORMAT_INT(offset + size));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_STACK:
	{
		//do nothing,address is already in stack
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_REFERENCE:
	{
		PX_sprintf0(payload, sizeof(payload), "pop r0\nload32 r0\npush r0");
	}
	break;

	case PX_SYNTAX_OPERAND_FROM_CONST:
	{
		px_int value = PX_AbiGetValue_int(poperand_abi, "value");
		PX_sprintf1(payload, sizeof(payload), "mov r0,%1\npush r0", PX_STRINGFORMAT_INT(value));
	}
	break;
	case PX_SYNTAX_OPERAND_FROM_MEMBER:
	{
		px_int offset = PX_AbiGetValue_int(poperand_abi, "offset");
		PX_sprintf1(payload, sizeof(payload), "load32 r0,bp+8\nadd r0,%1\npush r0", PX_STRINGFORMAT_INT(offset));
	}
	break;
	default:
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_dereference unsupport operand from");
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, "expr", payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_dereference out of memory");
		return PX_FALSE;
	}
	

	if (ptr_level == 1)
		ptr_level = 0;
	else
		ptr_level--;
	
	if (!PX_StringInitialize(pSyntax->mp, &final_type) || !PX_StringSet(&final_type, operand_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_dereference out of memory");
		return PX_FALSE;
	}
	
	if (!PX_Syntax_SetTypePointerLevel(&final_type,  ptr_level))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_dereference out of memory");
		return PX_FALSE;
	}

	pnewoperand = PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&final_type), PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&final_type)), PX_SYNTAX_OPERAND_FROM_REFERENCE);
	PX_StringFree(&final_type);
	if (!pnewoperand)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_dereference out of memory");
		return PX_FALSE;
	}
	
	PX_Syntax_PopOperate2(pSyntax, operand1_abi_index, opcode_abi_index);
	return PX_TRUE;
}

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_offset)
{
	PX_SYNTAX_OPERAND_FROM from;
	px_char payload[64] = { 0 };
	px_int ptr_level;
	px_abi* pnewoperand;
	px_abi* poperand1_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	px_abi* poperand2_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	const px_char* poperand1_type, * poperand2_type;
	px_string final_type;
	px_int final_type_size;

	PX_ASSERTIFX(!poperand1_abi || !PX_Syntax_CheckAbiName(poperand1_abi, "operand"), "PX_Syntax_operate_pointer_offset should not run here");
	poperand1_type = PX_AbiGetValue_string(poperand1_abi, "type");
	poperand2_type = PX_AbiGetValue_string(poperand2_abi, "type");

	PX_ASSERTIFX(!PX_Syntax_TypeMatch(poperand1_type, "pointer"), "PX_Syntax_operate_pointer_offset should not run here");
	from = (PX_SYNTAX_OPERAND_FROM)(PX_AbiGetValue_int(poperand1_abi, "from"));
	ptr_level = PX_Syntax_GetTypePointerLevel(poperand1_type);

	//decay pointer_level
	PX_ASSERTIFX(ptr_level <= 0, "PX_Syntax_operate_pointer_offset pointer level should not be zero");
	ptr_level--;

	if (!PX_StringInitialize(pSyntax->mp, &final_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset out of memory");
		return PX_FALSE;
	}

	if (!PX_StringSet(&final_type, poperand1_type))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset out of memory");
		return PX_FALSE;
	}

	if (!PX_Syntax_SetTypePointerLevel(&final_type, ptr_level))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset out of memory");
		return PX_FALSE;
	}
	final_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&final_type));

	PX_ASSERTIFX(final_type_size <= 0, "PX_Syntax_operate_pointer_offset size should not be zero");

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset operand to register failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset operand to register failed");
		return PX_FALSE;
	}


	if(!PX_Syntax_NewIRInstructionFormat1(pSyntax,"expr","mul r1,%1", PX_STRINGFORMAT_INT(final_type_size)))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset new IR instruction failed");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstructionFormat0(pSyntax, "expr", "add r0,r1\npush r0"))
	{
		PX_StringFree(&final_type);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset new IR instruction failed");
		return PX_FALSE;
	}

	if (!(pnewoperand=PX_Syntax_PushOperand(pSyntax,PX_StringGetText(&final_type), final_type_size, PX_SYNTAX_OPERAND_FROM_REFERENCE)))
	{
		PX_StringFree(&final_type);	
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_operate_pointer_offset push operand failed");
		return PX_FALSE;
	}
	PX_StringFree(&final_type);
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);

	return PX_TRUE;
}

static px_int PX_Syntax_array_dxdxdx_parse_count(const px_char payload[])
{
	px_int count = 1, factor = 0;
	px_bool has_digit = PX_FALSE;
	const px_char* p = payload;

	PX_ASSERTIFX(!payload, "Error: PX_Syntax_array_parse_count payload is null");

	while (PX_TRUE)
	{
		if (PX_charIsNumeric(*p))
		{
			factor = factor * 10 + (*p - '0');
			has_digit = PX_TRUE;
			p++;
			continue;
		}
		if (*p == 'x' || *p == '\0')
		{
			if (!has_digit || factor <= 0) return 0;
			count *= factor;
			if (*p == '\0') break;
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

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_array_dereference_offset)
{
	px_abi* poperand1_abi, * poperand2_abi;
	const px_char* poperand1_type, * poperand2_type, * pbase_type;
	px_int base_type_size,array_offset;
	px_string build_xxx_payload, build_xxx_payload2;

	poperand1_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand1_abi_index);
	poperand2_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand2_abi_index);
	if (!poperand1_abi || !poperand2_abi) return PX_FALSE;

	poperand1_type = PX_AbiGetValue_string(poperand1_abi, "type");
	poperand2_type = PX_AbiGetValue_string(poperand2_abi, "type");
	PX_ASSERTIFX(!PX_Syntax_TypeMatch(poperand1_type, "array") || !PX_Syntax_TypeMatch(poperand2_type, "ix"), "PX_Syntax_operate_array_dereference_offset should not run here");

	//array.<d0>[x<d1>...].<base_type>
	pbase_type = PX_Syntax_GetArrayBaseType(poperand1_type);
	PX_ASSERTIFX(!pbase_type, "PX_Syntax_operate_array_dereference_offset base type should not be null");
	base_type_size = PX_Syntax_GetMatchTypeSize(pSyntax, pbase_type);
	PX_ASSERTIFX(base_type_size <= 0, "PX_Syntax_operate_array_dereference_offset base type size should not be zero");

	if (!PX_StringInitialize(pSyntax->mp, &build_xxx_payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:array_dereference_offset out of memory");
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &build_xxx_payload2))
	{
		PX_StringFree(&build_xxx_payload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:array_dereference_offset out of memory");
		return PX_FALSE;
	}
	if (!PX_StringSet(&build_xxx_payload, poperand1_type)) goto _ERROR;
	PX_StringSubi(&build_xxx_payload, ".", 1);
	if (!PX_StringCopy(&build_xxx_payload2, &build_xxx_payload)) goto _ERROR;
	PX_StringTrimLeftSubn(&build_xxx_payload2, "x", 1);
	if (PX_StringLen(&build_xxx_payload2) == 0)
	{
		if (!PX_StringSet(&build_xxx_payload, poperand1_type)) goto _ERROR;
		PX_StringTrimLeftSubn(&build_xxx_payload, ".",2 );
		array_offset = 1 * base_type_size;
	}
	else
	{
		if (!PX_StringSet(&build_xxx_payload, poperand1_type)) goto _ERROR;
		PX_StringTrimLeftSubn(&build_xxx_payload, ".", 2);
		if (!PX_StringInsert(&build_xxx_payload, 0, ".")) goto _ERROR;
		if (!PX_StringInsert(&build_xxx_payload, 0, PX_StringGetText(&build_xxx_payload2))) goto _ERROR;
		if (!PX_StringInsert(&build_xxx_payload, 0, "array.")) goto _ERROR;
		array_offset = PX_Syntax_array_dxdxdx_parse_count(PX_StringGetText(&build_xxx_payload2)) * base_type_size;
	}
		

	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand2_abi_index, 1)) goto _ERROR;
	if (!PX_Syntax_NewIRInstructionFormat1(pSyntax, "expr", "mul r1,%1", PX_STRINGFORMAT_INT(array_offset))) goto _ERROR;
	if (!PX_Syntax_OperandToRegister(pSyntax, "expr", operand1_abi_index, 0)) goto _ERROR;
	if (!PX_Syntax_NewIRInstructionFormat0(pSyntax, "expr", "add r0,r1\npush r0")) goto _ERROR;
	if (!PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&build_xxx_payload), \
		PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&build_xxx_payload)), PX_SYNTAX_OPERAND_FROM_REFERENCE)) 
		goto _ERROR;

	
	PX_Syntax_PopOperate3(pSyntax, operand1_abi_index, operand2_abi_index, opcode_abi_index);
	PX_StringFree(&build_xxx_payload);
	PX_StringFree(&build_xxx_payload2);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&build_xxx_payload);
	PX_StringFree(&build_xxx_payload2);
	return PX_FALSE;
}
