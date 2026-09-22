#include "PX_Syntax_function.h"
#include "PX_Syntax_base_operate.h"
#include "PX_Syntax_declare_variable.h"

PX_SYNTAX_FUNCTION(PX_Syntax_begin_function_block)
{
	px_abi* pabi,*psecondlast_abi;
	const px_char* return_type;
	if ((pabi=PX_Syntax_NewAbi(pSyntax,"function_block")) ==PX_NULL)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_begin_function_block Memory Error1");
		return PX_FALSE;
	}
	psecondlast_abi = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(psecondlast_abi == PX_NULL, "Error: second last abi not found");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(psecondlast_abi, "type"), "Error: type abi expected");

	return_type = PX_AbiGetValue_string(psecondlast_abi, "value");
	if (!PX_AbiSet_string(pabi, "return_type", return_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_begin_function_block Memory Error2");
		return PX_FALSE;
	}
	PX_Syntax_PopAbiIndex(pSyntax, -2);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_name)
{
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pfunctionabi = PX_Syntax_GetAbiFromBackward(pSyntax, "function_block");
	px_int begin, end, begin_index, end_index;
	const px_char* pfunction_name;
	if (!pabi)
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pabi,"identifier"))
	{
		return PX_FALSE;
	}
	if (!pfunctionabi)
	{
		return PX_FALSE;
	}
	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	begin_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	end_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	pfunction_name = PX_AbiGetValue_string(pabi, "value");
	if (!PX_AbiSet_string(pfunctionabi, "function_name", pfunction_name))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_name Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pfunctionabi,"function_source_index", PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_name Memory Error2");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pfunctionabi, "function_source_line", PX_Syntax_GetCurrentLine(pSyntax)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_name Memory Error3");
		return PX_FALSE;
	}
	if (begin_index == end_index)
	{
		if (!PX_Syntax_NewStaticMapToken(pSyntax, begin_index, begin, end_index, end, PX_COLOR(255, 192, 255, 180), "function_name"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_name Memory Error4");
			return PX_FALSE;
		}

	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_enter_function)
{
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	if (!PX_Syntax_EnterBaseScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_enter_param Memory Error1");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_remove_type)
{
	px_abi* plastabi = PX_Syntax_GetLastAbi(pSyntax);
	if (!plastabi)
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(plastabi, "type"))
	{
		PX_ASSERTX("Assert Error: type abi expected");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_leave_params)
{
	px_abi* pscopeabi = PX_Syntax_GetLastScopeAbi(pSyntax);
	px_abi* pfunctionabi = PX_Syntax_GetAbiFromBackward(pSyntax, "function_block");
	px_abi param_abi;
	
	px_int params_size=0;
	px_int member_count;
	PX_ASSERTIFX(pfunctionabi == PX_NULL, "Error: function_block abi not found");
	PX_ASSERTIFX(pscopeabi == PX_NULL, "Error: scope abi not found");

	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);

	member_count = PX_AbiGet_PayloadMemberCount(pscopeabi, "variables");
	if (member_count)
	{
		px_int i;
		if (!PX_AbiGet_AbiReadOnly(pscopeabi, &param_abi, "variables"))
		{
			PX_ASSERTX("Assert Error: variables abi not found");
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_leave_params Memory Error2");
			return PX_FALSE;
		}
		
		if (!PX_AbiSet_Abi(pfunctionabi, "params", &param_abi))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_leave_params Memory Error2");
			return PX_FALSE;
		}
		for (i = 0; i < member_count; i++)
		{
			const px_char* pvariable_type;
			px_int size;
			px_char payload[16] = "[";
			PX_strcat(payload, PX_itos(i,10).data);
			PX_strcat(payload, "].type");
			pvariable_type = PX_AbiGetValue_string(&param_abi, payload);
			size=PX_Syntax_GetMatchTypeSize(pSyntax, pvariable_type);
			PX_ASSERTIFX(size == 0, "Error: type size not found");
			if(size<4)size=4;
			params_size += size;
		}
	}
	if (!PX_AbiSet_int(pfunctionabi, "params_size", params_size))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_leave_params Memory Error2");
		return PX_FALSE;
	}

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_predeclare_end)
{
	px_abi* plast_scopeabi = PX_Syntax_GetSecondLastScopeAbi(pSyntax);
	px_abi* pfunctionabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	const px_char* pfunction_name;
	px_string payload = { 0 };

	PX_ASSERTIFX(!pfunctionabi, "Error: function abi not found");
	PX_ASSERTIFX(!plast_scopeabi, "Error: scope abi not found");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pfunctionabi, "function_block"), "Error: function_block abi expected");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plast_scopeabi, "scope"), "Error: scope abi expected");
	PX_ASSERTIFX(!PX_AbiExist_Type(pfunctionabi, "function_name", PX_ABI_TYPE_STRING), "Error: function_name not found in function_block abi");
	
	pfunction_name = PX_AbiGetValue_string(pfunctionabi, "function_name");

	if(!PX_StringInitializeFormat1(pSyntax->mp, &payload, "functions.%1", PX_STRINGFORMAT_STRING(pfunction_name)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error1");
		return PX_FALSE;
	}
	if (PX_AbiExist(plast_scopeabi, PX_StringGetText(&payload)))
	{
		PX_StringFree(&payload);
		PX_Syntax_Terminate(pSyntax, "ast:error:Function name duplicated");
		return PX_FALSE;
	}
	if (!PX_AbiSet_Abi(plast_scopeabi, PX_StringGetText(&payload), pfunctionabi))
	{
		PX_StringFree(&payload);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_declare_end Memory Error1");
		return PX_FALSE;
	}
	PX_StringFree(&payload);
	return PX_TRUE;
}



PX_SYNTAX_FUNCTION(PX_Syntax_assign_expr_check_and_push)
{
	px_abi* pfunction_call_abi = PX_Syntax_GetAbiFromBackward(pSyntax, "function_call");
	px_abi* plastabi = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* plast_expr_type;
	px_int call_param_index;
	const px_char* accept_type;
	px_abi param_abi;

	PX_ASSERTIFX(pfunction_call_abi == PX_NULL, "Error: function_call abi not found");
	PX_ASSERTIFX(plastabi == PX_NULL, "Error: last abi not found");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(plastabi, "expr"), "Error: expr abi expected");

	plast_expr_type = PX_AbiGetValue_string(plastabi, "type");
	//check type match
	call_param_index = PX_AbiGetValue_int(pfunction_call_abi, "call_param_index");
	if(!PX_AbiGet_MemberByIndex(pfunction_call_abi, &param_abi, "function.params", call_param_index))
	{
		PX_ASSERTX("Assert Error: function params abi not found");
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_assign_expr_operand_push_to_stack Memory Error2");
		return PX_FALSE;
	}
	if (!PX_AbiExist_Type(&param_abi, "type", PX_ABI_TYPE_STRING))
	{
		PX_ASSERTX("Assert Error: param type not found");
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_assign_expr_operand_push_to_stack Memory Error3");
		return PX_FALSE;
	}
	accept_type = PX_AbiGetValue_string(&param_abi, "type");

	if (!PX_Syntax_ConvertType(pSyntax, "expr", 0, plast_expr_type, accept_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_assign_expr_operand_push_to_stack Memory Error4");
		return PX_FALSE;
	}

	if (!PX_Syntax_NewIRInstruction1(pSyntax, "expr", "push", "r0"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_assign_expr_operand_push_to_stack Memory Error4");
		return PX_FALSE;
	}

	if (!PX_AbiSet_int(pfunction_call_abi, "call_param_index", call_param_index + 1))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_assign_expr_check_and_push Memory Error5");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_new_function_call)
{
	px_abi* newfunctionabi;
	px_int i,begin,end,begin_index,end_index;
	const px_char* pfunction_name;
	px_abi* plast_identifier_abi = PX_Syntax_GetLastAbi(pSyntax);

	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	begin_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	end_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);

	PX_ASSERTIFX(!plast_identifier_abi, "Error: abi not found");
	if (!plast_identifier_abi)
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(plast_identifier_abi, "identifier"))
	{
		return PX_FALSE;
	}
	pfunction_name = PX_AbiGetValue_string(plast_identifier_abi, "value");
	//search function define
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_abi* pscope_abi = PX_Syntax_GetAbiByIndex(pSyntax, i);
		if (PX_Syntax_CheckAbiName(pscope_abi, "scope"))
		{
			px_abi function_abi;
			px_string payload = { 0 };
			if(!PX_StringInitializeFormat1(pSyntax->mp, &payload, "functions.%1", PX_STRINGFORMAT_STRING(pfunction_name)))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error1");
				return PX_FALSE;
			}
			if (PX_AbiExist_abi(pscope_abi,PX_StringGetText(&payload)))
			{
				if (PX_NULL== (newfunctionabi=PX_Syntax_NewAbi(pSyntax, "function_call")))
				{
					PX_StringFree(&payload);
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error1");
					return PX_FALSE;
				}
				//NewAbi could reallocate reg_abi_stack, pscope_abi must be refetched
				pscope_abi = PX_Syntax_GetAbiByIndex(pSyntax, i);

				if (!PX_AbiSet_int(newfunctionabi, "call_param_index", 0))
				{
					PX_StringFree(&payload);
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error2");
					return PX_FALSE;
				}

				if (PX_AbiGet_AbiReadOnly(pscope_abi, &function_abi, PX_StringGetText(&payload)))
				{
					if (!PX_AbiSet_Abi(newfunctionabi, "function", &function_abi))
					{
						PX_StringFree(&payload);
						PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error2");
						return PX_FALSE;
					}
					PX_Syntax_PopAbiIndex(pSyntax, -2);

					if (begin_index==end_index)
					{
						if (!PX_Syntax_NewStaticMapToken(pSyntax, begin_index, begin, end_index, end, PX_COLOR(255, 255, 155, 64), "function_call"))
						{
							PX_StringFree(&payload);
							PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error3");
							return PX_FALSE;
						}

					}
					if (!PX_Syntax_NewIRLocation(pSyntax, "function_call"))
					{
						PX_StringFree(&payload);
						PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_function_call Memory Error4");
						return PX_FALSE;
					}
					PX_StringFree(&payload);
					return PX_TRUE;
				}
			}
		}
	}
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_call_end)
{
	px_int call_function_size;
	while (pSyntax->reg_abi_stack.size)
	{
		px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
		if (PX_Syntax_CheckAbiName(pabi, "expr"))
		{
			if (!PX_Syntax_MergeLast2AbiValue(pSyntax, "expr", "ir", "function_call", "ir"))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_endline MergeLast2AbiValue Memory Error");
				return PX_FALSE;
			}
		}
		else if (PX_Syntax_CheckAbiName(pabi, "function_call"))
		{
			//generate call ir
			px_string payload, return_type;
			const px_char* function_name = PX_AbiGetValue_string(pabi, "function.function_name");
			px_int param_count = PX_AbiGet_PayloadMemberCount(pabi, "function.params");
			px_int param_index = PX_AbiGetValue_int(pabi, "call_param_index");
			//return_type points into pabi's buffer which is reallocated/freed below
			if(!PX_StringInitializeFormat0(pSyntax->mp, &return_type, PX_AbiGetValue_string(pabi, "function.return_type")))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error3");
				return PX_FALSE;
			}
			if (param_count!= param_index)
			{
				PX_StringFree(&return_type);
				PX_Syntax_Terminate(pSyntax, "ast:error:Function call argument count mismatch");
				return PX_FALSE;
			}
			if (!PX_StringInitialize(pSyntax->mp, &payload))
			{
				PX_StringFree(&return_type);
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error1");
				return PX_FALSE;
			}
			if(!PX_StringFormat1(&payload, "__function_%1", PX_STRINGFORMAT_STRING(function_name)))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error2");
				PX_StringFree(&return_type);
				PX_StringFree(&payload);
				return PX_FALSE;
			}

			if (!PX_Syntax_NewIRInstruction1(pSyntax, "function_call", "call", PX_StringGetText(&payload)))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error1");
				PX_StringFree(&payload);
				PX_StringFree(&return_type);
				return PX_FALSE;
			}

			PX_StringFree(&payload);
			call_function_size = PX_AbiGetValue_int(pabi, "function.params_size");
			if (call_function_size!=0)
			{
				//free params stack space before pushing the return value
				if (!PX_Syntax_NewIRInstruction2(pSyntax, "function_call", "add", "sp", PX_itos(call_function_size, 10).data))
				{
					PX_StringFree(&return_type);
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error3");
					return PX_FALSE;
				}
			}

			if (!PX_Syntax_NewIRInstruction1(pSyntax, "function_call", "push", "r0"))
			{
				PX_StringFree(&return_type);
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error2");
				return PX_FALSE;
			}

			if (!PX_Syntax_MergeLast2AbiValue(pSyntax, "function_call", "ir", "expr", "ir"))
			{
				PX_StringFree(&return_type);
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_expr_endline MergeLast2AbiValue Memory Error4");
				return PX_FALSE;
			}

			if (!PX_Syntax_PushOperand(pSyntax, PX_StringGetText(&return_type), PX_Syntax_GetMatchTypeSize(pSyntax, PX_StringGetText(&return_type)), PX_SYNTAX_OPERAND_FROM_STACK))
			{
				PX_StringFree(&return_type);
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_call_end Memory Error5");
				return PX_FALSE;
			}
			PX_StringFree(&return_type);
			return PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:Unexpected abi in function call");
			return PX_FALSE;
		}
	}

	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:Invalid function declaration");
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_implementation_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:Invalid function implementation");
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_function_block_prototype_end)
{
	px_abi* plastabi = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pfunction_block = PX_Syntax_GetSecondLastAbi(pSyntax);
	if (!plastabi || !pfunction_block ||
		!PX_Syntax_CheckAbiName(plastabi, "scope") ||
		!PX_Syntax_CheckAbiName(pfunction_block, "function_block"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_prototype_end invalid abi stack");
		return PX_FALSE;
	}
	if(!PX_Syntax_LeaveScope(pSyntax))//scope_block
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_prototype_end: leave scope failed.");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop scope_block
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_function_block_implementation)
{
	//scope_block
	//scope
	//function_block

	px_abi* pscope_block = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pscope = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_abi* pfunction_block = PX_Syntax_GetThirdLastAbi(pSyntax);
	const px_char* pfunction_name;
	px_int function_source_index, function_source_line;
	px_string function_label;

	PX_ASSERTIFX(!pfunction_block, "Error: function_block abi not found");
	PX_ASSERTIFX(!pscope_block, "Error: scope_block abi not found");
	PX_ASSERTIFX(!pscope, "Error: second last scope abi not found");
	//check name
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pfunction_block, "function_block"), "Error: function_block abi expected");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope_block, "scope_block"), "Error: scope_block abi expected");
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope, "scope"), "Error: scope abi expected");

	if(!PX_Syntax_LeaveScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_begin Memory Error");
		return PX_FALSE;
	}	
	pscope_block = PX_Syntax_GetLastAbi(pSyntax);
	pfunction_block = PX_Syntax_GetSecondLastAbi(pSyntax);
	PX_ASSERTIFX(!pscope_block||!PX_Syntax_CheckAbiName(pscope_block, "scope_block"), "Error: scope_block abi expected");
	PX_ASSERTIFX(!pfunction_block||!PX_Syntax_CheckAbiName(pfunction_block, "function_block"), "Error: function_block abi expected");
	//scope_block
	//function_block


	function_source_index = PX_AbiGetValue_int(pfunction_block, "function_source_index");
	function_source_line = PX_AbiGetValue_int(pfunction_block, "function_source_line");
	pfunction_name = PX_AbiGetValue_string(pfunction_block, "function_name");
	if (!PX_StringInitialize(pSyntax->mp, &function_label))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_begin Memory Error");
		return PX_FALSE;
	}
	//function_label
	//;@loc
	//mov bp,sp
	//<---scope
	if (!PX_StringFormat2(&function_label, "__function_%1:\n%2mov bp,sp\n", PX_STRINGFORMAT_STRING(pfunction_name), PX_STRINGFORMAT_STRING(PX_Syntax_BuildIRLocation(pSyntax, function_source_index, function_source_line).data)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_begin Memory Error");
		PX_StringFree(&function_label);
		return PX_FALSE;
	}
	//header
	if (!PX_AbiAppend_string(pfunction_block, "ir_library", PX_StringGetText(&function_label)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_begin Memory Error");
		PX_StringFree(&function_label);
		return PX_FALSE;
	}
	PX_StringFree(&function_label);

	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "function_block", "ir", "ir_library"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_begin Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiAppend_string(pfunction_block, "ir_library", "ret\n"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_function_block_implementation_end Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);//pop scope_block
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_new_param)
{
	px_abi* pidentifier = PX_Syntax_GetAbiFromBackward(pSyntax, "identifier");
	px_abi* pscope = PX_Syntax_GetLastScopeAbi(pSyntax);
	px_abi variables;
	const px_char* pparam_name;
	if (!pidentifier || !pscope || !PX_Syntax_CheckAbiName(pidentifier, "identifier"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_param invalid abi stack");
		return PX_FALSE;
	}
	pparam_name = PX_AbiGet_string(pidentifier, "value");
	if (!pparam_name)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_new_param parameter name not found");
		return PX_FALSE;
	}
	if (PX_AbiGet_AbiReadOnly(pscope, &variables, "variables") &&
		PX_AbiExist_abi(&variables, pparam_name))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:Function parameter name duplicated");
		return PX_FALSE;
	}
	return PX_Syntax_new_declare_variable_token(pSyntax, "param");
}
px_bool PX_Syntax_load_function(PX_Syntax* pSyntax)
{
	//declare_token_prefix



	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_identifiter =declare_token_prefix identifier", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_identifiter =identifier", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block_param", 0, PX_Syntax_function_block_remove_type, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block_param = type_set variable", 0, PX_Syntax_new_param, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block_params_list = function_block_param", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block_params_list = function_block_param ',' ...", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block_params_list = function_block_param *", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare", 0, PX_Syntax_function_block_predeclare_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set", 0, PX_Syntax_begin_function_block, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set function_identifiter", 0, PX_Syntax_function_block_name, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set function_identifiter '('", 0, PX_Syntax_function_block_enter_function, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set function_identifiter '(' function_block_params_list ')'", 0, PX_Syntax_function_block_leave_params, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set function_identifiter '(' ')'", 0, PX_Syntax_function_block_leave_params, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_declare = type_set function_identifiter '(' *", 0, PX_Syntax_function_block_error, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block = function_declare ';'", 0, PX_Syntax_function_block_prototype_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_block = function_declare scope_block", 0, PX_Syntax_function_block_implementation, 0))
		return PX_FALSE;


	if (!PX_Syntax_Parse_PEBNF(pSyntax, "call_function_name = identifier", 0, 0, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_param_list = assign_expr", 0, PX_Syntax_assign_expr_check_and_push, 0)) //push param
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_param_list = assign_expr ',' ...", 0, 0, 0)) //push param
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_param_list = assign_expr *", 0, 0, 0))
		return PX_FALSE;
	
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_call = call_function_name", 0, PX_Syntax_new_function_call, 0))
		return PX_FALSE;
	
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_call = call_function_name '(' ", 0, PX_Syntax_TokenRender, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_call = call_function_name '(' ')' ", 0, PX_Syntax_function_call_end, 0))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "function_call = call_function_name '(' function_param_list ')' ", 0, PX_Syntax_function_call_end, 0))
		return PX_FALSE;

	return PX_TRUE;
}
