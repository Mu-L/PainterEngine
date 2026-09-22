#include "PX_Syntax.h"
#include "PX_Syntax_base_type.h"

px_bool PX_Syntax_IsValidToken(const px_char token[])
{
	if (token[0] == '\0' || PX_charIsNumeric(token[0]))
	{
		return PX_FALSE;
	}

	while (*token)
	{
		if ((*token >= 'A' && *token <= 'Z') || (*token >= 'a' && *token <= 'z') || *token == '_' || *token == ':' || PX_charIsNumeric(*token)|| ((*token)&0x80))
		{
			token++;
			continue;
		}
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_Syntax_IsEndOfSource(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_IsEnd(&pSyntax->reg_syntaxlexer);
}

PX_Syntax_pebnf* PX_Syntax_GetPebnfByIndex(PX_Syntax* pSyntax, px_int index)
{
	if (PX_VectorCheckIndex(&pSyntax->pebnf, index))
	{
		return PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, index);
	}
	return PX_NULL;
}

px_int PX_Syntax_GetPebnfIndexByMnemonic(PX_Syntax* pSyntax, const px_char mnemonic[])
{
	px_int i;
	PX_Syntax_pebnf* ptype;
	for (i = 0; i < pSyntax->pebnf.size; i++)
	{
		ptype = PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, i);
		if (PX_strequ(PX_StringGetText(&ptype->mnenonic), mnemonic))
		{
			return i;
		}
	}
	return -1;
}

PX_Syntax_pebnf* PX_Syntax_GetPebnf(PX_Syntax* pSyntax, const px_char mnemonic[])
{
	px_int i;
	PX_Syntax_pebnf* ptype;
	for (i = 0; i < pSyntax->pebnf.size; i++)
	{
		ptype = PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, i);
		if (PX_strequ(PX_StringGetText(&ptype->mnenonic), mnemonic))
		{
			return ptype;
		}
	}
	return PX_NULL;
}

PX_Syntax_bnfnode* PX_Syntax_GetPebnfNode(PX_Syntax* pSyntax, const px_char mnemonic[])
{
	PX_Syntax_pebnf* ppebnf = PX_Syntax_GetPebnf(pSyntax, mnemonic);
	if (ppebnf)
	{
		return ppebnf->pbnfnode;
	}
	return PX_NULL;
}

PX_Syntax_bnfnode* PX_Syntax_GetOtherNode(PX_Syntax_bnfnode* pbnfnode, PX_SYNTAX_AST_TYPE ast_type, const px_char mnemonic[])
{
	while (pbnfnode)
	{
		if (pbnfnode->type == ast_type)
		{
			if (PX_strequ(PX_StringGetText(&pbnfnode->constant), mnemonic))
			{
				return pbnfnode;
			}
		}
		pbnfnode = pbnfnode->pothers;
	}
	return PX_NULL;
}

PX_Syntax_pebnf* PX_Syntax_SetPEBNF(PX_Syntax* pSyntax, const px_char mnemonic[])
{
	PX_Syntax_pebnf* ppebnf = PX_Syntax_GetPebnf(pSyntax, mnemonic);
	if (!ppebnf)
	{
		PX_Syntax_pebnf pebnf = {0};

		if (!PX_StringInitialize(pSyntax->mp, &pebnf.mnenonic))
			return PX_NULL;
		if (!PX_StringAppend(&pebnf.mnenonic, mnemonic))
		{
			PX_StringFree(&pebnf.mnenonic);
			return PX_NULL;
		}
			
		if (!PX_VectorPushback(&pSyntax->pebnf, &pebnf))
		{
			PX_StringFree(&pebnf.mnenonic);
			return PX_NULL;
		}
		return PX_VECTORLAST(PX_Syntax_pebnf, &pSyntax->pebnf);
	}
	return ppebnf;

}

px_bool PX_Syntax_Parse_NextPENNF(PX_Syntax* pSyntax, PX_Syntax_pebnf* ppebnf, PX_Syntax_bnfnode* pprevious, px_lexer* plexer, PX_Syntax_Function penterfunction, PX_Syntax_Function pleavefunction,px_void *userptr)
{
	PX_LEXER_LEXEME_TYPE type;
	while (PX_TRUE)
	{
		type = PX_LexerGetNextLexeme(plexer);
		if (type == PX_LEXER_LEXEME_TYPE_COMMENT)
			continue;
		if (type == PX_LEXER_LEXEME_TYPE_SPACER) 
			continue;
		break;
	}

	switch (type)
	{
	case PX_LEXER_LEXEME_TYPE_CONATINER:
	case PX_LEXER_LEXEME_TYPE_TOKEN:
	case PX_LEXER_LEXEME_TYPE_DELIMITER:
	{
		PX_SYNTAX_AST_TYPE ast_type;
		const px_char* pstr;
		PX_Syntax_bnfnode* psearchast;
		if (type == PX_LEXER_LEXEME_TYPE_CONATINER)
		{
			ast_type = PX_SYNTAX_AST_TYPE_CONSTANT;
			PX_LexerGetIncludedString(plexer, &plexer->CurLexeme);
			pstr = PX_LexerGetLexeme(plexer);
		}
		else if (type == PX_LEXER_LEXEME_TYPE_DELIMITER)
		{
			if (PX_LexerGetSymbol(plexer)=='&')
			{
				ast_type = PX_SYNTAX_AST_TYPE_CONTINUOUS;
				pstr = "&";
			}
			else
			{
				return PX_FALSE;
			}
		}
		else
		{
			pstr = PX_LexerGetLexeme(plexer);
			if (pstr[0] == '*' && pstr[1] == '\0')
			{
				ast_type = PX_SYNTAX_AST_TYPE_FUNCTION;
			}
			else if (PX_strequ(pstr,"..."))
			{
				ast_type = PX_SYNTAX_AST_TYPE_RECURSION;
			}
			else if (PX_strequ(pstr, "---"))
			{
				ast_type = PX_SYNTAX_AST_TYPE_LOOP;
			}
			else
			{
				ast_type = PX_SYNTAX_AST_TYPE_LINKER;
			}
		}

		if (pprevious)
		{
			psearchast = PX_Syntax_GetOtherNode(pprevious->pnext, ast_type, pstr);
		}
		else
		{
			psearchast = PX_Syntax_GetOtherNode(ppebnf->pbnfnode, ast_type, pstr);
		}

		if (!psearchast)
		{
			PX_Syntax_bnfnode* plast;
			PX_Syntax_bnfnode* pnewast = (PX_Syntax_bnfnode*)MP_Malloc(pSyntax->mp, sizeof(PX_Syntax_bnfnode));
			if (pnewast)
			{
				if (!PX_StringInitialize(pSyntax->mp, &pnewast->constant))
				{
					MP_Free(pSyntax->mp, pnewast);
					return PX_FALSE;
				}
				pnewast->type = ast_type;
				if (ast_type == PX_SYNTAX_AST_TYPE_RECURSION || ast_type == PX_SYNTAX_AST_TYPE_LOOP)
				{
					if (!PX_StringAppend(&pnewast->constant, PX_StringGetText(&ppebnf->mnenonic)))
					{
						PX_StringFree(&pnewast->constant);
						MP_Free(pSyntax->mp, pnewast);
						return PX_FALSE;
					}
				}
				else
				{
					if (!PX_StringAppend(&pnewast->constant, pstr))
					{
						PX_StringFree(&pnewast->constant);
						MP_Free(pSyntax->mp, pnewast);
						return PX_FALSE;
					}
				}
				
				pnewast->penterfunction = PX_NULL;
				pnewast->pleavefunction = PX_NULL;
				pnewast->pothers = PX_NULL;
				pnewast->pnext = PX_NULL;
				pnewast->userptr = PX_NULL;
			}
			else
			{
				PX_ASSERT();
				return PX_FALSE;
			}
			if (pprevious)
			{
				plast = pprevious->pnext;
				if (plast == PX_NULL)
				{
					pprevious->pnext = pnewast;
				}
				else
				{
					while (plast->pothers)
					{
						plast = plast->pothers;
					}
					plast->pothers = pnewast;
				}
			}
			else
			{
				plast = ppebnf->pbnfnode;
				if (plast == PX_NULL)
				{
					ppebnf->pbnfnode = pnewast;
				}
				else
				{
					while (plast->pothers)
					{
						plast = plast->pothers;
					}
					plast->pothers = pnewast;
				}
			}
			return PX_Syntax_Parse_NextPENNF(pSyntax, ppebnf, pnewast, plexer, penterfunction, pleavefunction,userptr);
		}
		else
		{
			return PX_Syntax_Parse_NextPENNF(pSyntax, ppebnf, psearchast, plexer, penterfunction, pleavefunction, userptr);
		}
	}
	break;
	case PX_LEXER_LEXEME_TYPE_END:
	{
		if(!pprevious->penterfunction)
			pprevious->penterfunction = penterfunction;
		else if (pprevious->penterfunction != penterfunction)
			PX_ASSERTX("Error:PEBNF Error, enter function already exist");

		if (!pprevious->pleavefunction)
			pprevious->pleavefunction = pleavefunction;
		else if(pprevious->pleavefunction!= pleavefunction)
			PX_ASSERTX("Error:PEBNF Error, leave function already exist");

		if (!pprevious->userptr)
			 pprevious->userptr = userptr;
		else if(pprevious->userptr != userptr)
			PX_ASSERTX("Error:PEBNF Error, userptr already exist");
		return PX_TRUE;
	}
	break;
	default:
		PX_ASSERTX( "Error:PEBNF Error");
		return PX_FALSE;
	}
}

px_bool PX_Syntax_Parse_PEBNF(PX_Syntax* pSyntax, const px_char PEBNF[], PX_Syntax_Function penterfunction, PX_Syntax_Function pleavefunction,px_void *userptr)
{
	px_lexer lexer;
	PX_LEXER_LEXEME_TYPE type;
	PX_LexerInitialize(&lexer, pSyntax->mp);
	PX_LexerRegisterSpacer(&lexer, ' ');
	PX_LexerRegisterContainer(&lexer, "'", "'");
	PX_LexerRegisterDelimiter(&lexer, '=');
	PX_LexerRegisterDelimiter(&lexer, '&');
	
	PX_LexerLoadSourceWithPresort(&lexer, PEBNF);
	while (PX_TRUE)
	{
		type = PX_LexerGetNextLexeme(&lexer);
		if (type == PX_LEXER_LEXEME_TYPE_COMMENT)
			continue;
		if (type == PX_LEXER_LEXEME_TYPE_SPACER)
			continue;
		break;
	}
	
	if (type == PX_LEXER_LEXEME_TYPE_TOKEN)
	{
		const px_char* plexeme = PX_LexerGetLexeme(&lexer);
		PX_Syntax_pebnf* ppebnf = PX_Syntax_SetPEBNF(pSyntax, plexeme);
		if (!ppebnf)
		{
			PX_ASSERTIFX(!ppebnf, "Error:PEBNF malloc Error");
			goto _ERROR;
		}
		while (PX_TRUE)
		{
			type = PX_LexerGetNextLexeme(&lexer);
			if (type == PX_LEXER_LEXEME_TYPE_COMMENT)
				continue;
			if (type == PX_LEXER_LEXEME_TYPE_SPACER)
				continue;
			break;
		}
		if (type==PX_LEXER_LEXEME_TYPE_END)
		{
			ppebnf->penterfunction = penterfunction;
			ppebnf->pleavefunction = pleavefunction;
			ppebnf->userptr = userptr;
			PX_LexerFree(&lexer);
			return PX_TRUE;
		}

		PX_ASSERTIFX(type != PX_LEXER_LEXEME_TYPE_DELIMITER, "Error:PEBNF Error");
		PX_ASSERTIFX(!PX_strequ(PX_LexerGetLexeme(&lexer), "="), "Error: = expected but not found");
		if (ppebnf)
		{
			if (PX_Syntax_Parse_NextPENNF(pSyntax, ppebnf, 0, &lexer, penterfunction,pleavefunction,userptr))
			{
				PX_LexerFree(&lexer);
				return PX_TRUE;
			}
			else
			{
				goto _ERROR;
			}
		}
		else
		{
			goto _ERROR;
		}
	}
	else
	{
		PX_ASSERTIFX(PX_FALSE, "Error:PEBNF Error");
		goto _ERROR;
	}

	PX_LexerFree(&lexer);
	return PX_TRUE;
_ERROR:
	PX_LexerFree(&lexer);
	PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_Parse_PEBNF invalid PEBNF");
	PX_ASSERTX("Error:PEBNF Error");
	return PX_FALSE;
}


px_bool PX_Syntax_ExecuteAstOpcode(PX_Syntax* pSyntax, const px_char type[])
{
	px_abi newabi;
	PX_AbiCreate_DynamicWriter(&newabi, pSyntax->mp);
	if (!PX_AbiSet_string(&newabi, "type", type))
	{
		PX_AbiFree(&newabi);
		return PX_FALSE;
	}
	if (!PX_VectorPushback(&pSyntax->reg_ast_instr_stack, &newabi))
	{
		PX_AbiFree(&newabi);
		return PX_FALSE;
	}
	return PX_TRUE;
}


static px_bool PX_Syntax_SolveExpression(PX_Syntax* pSyntax)
{
	while (PX_TRUE)
	{
		px_abi* plast_opcode_abi = PX_NULL, * psecondlast_opcode_abi = PX_NULL, type_define_abi = {0};
		px_int lastopcode_index = 0, secondlastopcode_index = 0, operate_count = 0;
		px_int plast_opcode_abi_index = -1, psecondlast_opcode_abi_index = -1;

		PX_Syntax_opcode* plast_define_opcode = PX_NULL, * psecondlast_define_opcode = PX_NULL;
		px_int second_last_opcount = 0;
		px_int i;
		for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
		{
			px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
			if (PX_Syntax_CheckAbiName(pabi, "opcode"))
			{
				if (!plast_opcode_abi)
				{
					plast_opcode_abi = pabi;
					plast_opcode_abi_index = i;
				}
				else if (!psecondlast_opcode_abi)
				{
					psecondlast_opcode_abi = pabi;
					psecondlast_opcode_abi_index = i;
					break;
				}
			}
		}
		if (plast_opcode_abi == PX_NULL)
		{
			return PX_TRUE;
		}
		if (psecondlast_opcode_abi == PX_NULL)
		{
			return PX_TRUE;
		}

		lastopcode_index = PX_AbiGetValue_int(plast_opcode_abi, "index");
		secondlastopcode_index = PX_AbiGetValue_int(psecondlast_opcode_abi, "index");

		PX_ASSERTIFX(!PX_VectorCheckIndex(&pSyntax->reg_expr_opcode_stack, lastopcode_index), "invalid opcode index");
		PX_ASSERTIFX(!PX_VectorCheckIndex(&pSyntax->reg_expr_opcode_stack, secondlastopcode_index), "invalid opcode index");

		plast_define_opcode = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, lastopcode_index);
		psecondlast_define_opcode = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, secondlastopcode_index);

		if (plast_define_opcode->precedence >= psecondlast_define_opcode->precedence)
		{
			if (psecondlast_define_opcode->type == PX_SYNTAX_OPCODE_TYPE_BEGIN)
			{
				if (plast_define_opcode->type == PX_SYNTAX_OPCODE_TYPE_END)
				{
					if (plast_define_opcode->pair!=psecondlast_define_opcode->opcode[0])
					{
						PX_Syntax_Terminate(pSyntax, "ast:error:invalid expression,parentheses pair mismatch");
						return PX_FALSE;
					}
					//pop opcodes
					if (plast_define_opcode->offset_end_redirect_opcode_index!=-1)
					{
						PX_AbiSet_int(plast_opcode_abi, "index", plast_define_opcode->offset_end_redirect_opcode_index);
						PX_Syntax_PopAbiIndex(pSyntax, psecondlast_opcode_abi_index);
					}
					else
						PX_Syntax_PopOperate2(pSyntax, psecondlast_opcode_abi_index, plast_opcode_abi_index);
					continue;
				}
				else
				{
					return PX_TRUE;
				}
			}

			second_last_opcount = \
				(psecondlast_define_opcode->type == PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX || psecondlast_define_opcode->type == PX_SYNTAX_OPCODE_TYPE_UNARY_SUFFIX) ? \
				1 : 2;
			if (second_last_opcount == 1)
			{
				px_int last_operand_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, "operand");
				px_abi* poperand;
				const px_char* operand_type = PX_NULL;
				px_string fallback_type;
				if (last_operand_index == -1)
				{
					PX_Syntax_Terminate(pSyntax, "ast:error:SolveExpression invalid expression,miss operand");
					return PX_FALSE;
				}
				poperand = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, last_operand_index);
				operand_type = PX_AbiGet_string(poperand, "type");
				PX_StringInitialize(pSyntax->mp, &fallback_type);
				PX_StringSet(&fallback_type, operand_type);
				while (PX_StringLen(&fallback_type))
				{
					//type_define_abi = PX_Syntax_GetTypeDefineAbiReadOnly(pSyntax, PX_StringGetText(&fallback_type));
					if (PX_Syntax_GetMatchTypeAbi(pSyntax, PX_StringGetText(&fallback_type), &type_define_abi))
					{
						operate_count = PX_AbiGet_PayloadMemberCount(&type_define_abi, "operates");
						for (i = 0; i < operate_count; i++)
						{
							px_char operate_payload[32] = { 0 };
							px_int opcode_index;
							PX_sprintf1(operate_payload, sizeof(operate_payload), "operates.[%1].opcode_index", PX_STRINGFORMAT_INT(i));
							opcode_index = PX_AbiGetValue_int(&type_define_abi, operate_payload);
							if (opcode_index == secondlastopcode_index)
							{
								PX_Syntax_Operate_Function pfunction;
								PX_sprintf1(operate_payload, sizeof(operate_payload), "operates.[%1].function", PX_STRINGFORMAT_INT(i));
								if (PX_AbiExist_Type(&type_define_abi, operate_payload, PX_ABI_TYPE_PTR))
								{
									pfunction = (PX_Syntax_Operate_Function)PX_AbiGetValue_ptr(&type_define_abi, operate_payload);
									if (pfunction)
									{
										if (!pfunction(pSyntax, last_operand_index, -1, psecondlast_opcode_abi_index))
										{
											PX_StringFree(&fallback_type);
											if (!PX_Syntax_IsTerminated(pSyntax))
												PX_Syntax_Terminate(pSyntax, "runtime:error:fail to operate opcode");
											return PX_FALSE;
										}
										break;
									}
								}
							}
						}
					}
					if (i != operate_count)
						break;
					//fallback to parent type: "ix.i.32" -> "ix.i" -> "ix"
					PX_StringTrimBackwardUntil(&fallback_type, '.');
				}
				if (PX_StringLen(&fallback_type)==0)
				{
					PX_StringFree(&fallback_type);
					PX_Syntax_Terminate(pSyntax, "ast:error:unsupport unary operate for type");
					return PX_FALSE;
				}
				PX_StringFree(&fallback_type);
			}
			else if (second_last_opcount == 2)
			{
				px_int last_operand_abi_index = -1, second_last_operand_abi_index = -1;
				px_abi* poperand1, * poperand2;
				const px_char* operand1_type = PX_NULL, * operand2_type = PX_NULL;
				px_int j = 0;
				px_string fallback_type;
				for (j = pSyntax->reg_abi_stack.size - 1; j >= 0; j--)
				{
					px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, j);
					if (PX_Syntax_CheckAbiName(pabi, "operand"))
					{
						if (last_operand_abi_index == -1)
						{
							last_operand_abi_index = j;
						}
						else
						{
							second_last_operand_abi_index = j;
							break;
						}
					}
				}
				if (last_operand_abi_index == -1 || second_last_operand_abi_index == -1)
				{
					PX_Syntax_Terminate(pSyntax, "ast:error:invalid expression,miss operand");
					return PX_FALSE;
				}
				poperand1 = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, second_last_operand_abi_index);
				poperand2 = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, last_operand_abi_index);
				operand1_type = PX_AbiGet_string(poperand1, "type");
				operand2_type = PX_AbiGet_string(poperand2, "type");

				PX_StringInitialize(pSyntax->mp, &fallback_type);
				PX_StringSet(&fallback_type, operand1_type);
				while (PX_StringLen(&fallback_type))
				{
					if (PX_Syntax_GetMatchTypeAbi(pSyntax, PX_StringGetText(&fallback_type), &type_define_abi))
					{
						operate_count = PX_AbiGet_PayloadMemberCount(&type_define_abi, "operates");
						for (i = 0; i < operate_count; i++)
						{
							px_char operate_payload[32] = { 0 };
							px_int opcode_index;
							PX_sprintf1(operate_payload, sizeof(operate_payload), "operates.[%1].opcode_index", PX_STRINGFORMAT_INT(i));
							opcode_index = PX_AbiGetValue_int(&type_define_abi, operate_payload);
							if (opcode_index == secondlastopcode_index)
							{
								const px_char* other_type_str;
								PX_sprintf1(operate_payload, sizeof(operate_payload), "operates.[%1].other_type", PX_STRINGFORMAT_INT(i));
								other_type_str = PX_AbiGetValue_string(&type_define_abi, operate_payload);
								if (PX_Syntax_TypeMatch(operand2_type, other_type_str))
								{
									PX_Syntax_Operate_Function pfunction;
									PX_sprintf1(operate_payload, sizeof(operate_payload), "operates.[%1].function", PX_STRINGFORMAT_INT(i));
									if (PX_AbiExist_Type(&type_define_abi, operate_payload, PX_ABI_TYPE_PTR))
									{
										pfunction = (PX_Syntax_Operate_Function)PX_AbiGetValue_ptr(&type_define_abi, operate_payload);
										if (pfunction)
										{
											if (!pfunction(pSyntax, second_last_operand_abi_index, last_operand_abi_index, psecondlast_opcode_abi_index))
											{
												PX_StringFree(&fallback_type);
												if (!PX_Syntax_IsTerminated(pSyntax))
													PX_Syntax_Terminate(pSyntax, "runtime:error:fail to operate opcode");
												return PX_FALSE;
											}
											break;
										}
									}
								}
							}
						}
					}
					if (i < operate_count)
						break;
					//fallback to parent type: "ix.i.32" -> "ix.i" -> "ix"
					PX_StringTrimBackwardUntil(&fallback_type, '.');
				}
				if (PX_StringLen(&fallback_type)==0)
				{
					PX_StringFormat1(&fallback_type, "ast:error:unsupport binary operate for type '%1'", PX_STRINGFORMAT_STRING(operand1_type));
					PX_Syntax_Terminate(pSyntax, PX_StringGetText(&fallback_type));
					PX_StringFree(&fallback_type);
					return PX_FALSE;
				}
				PX_StringFree(&fallback_type);
			}
			else
			{
				PX_Syntax_Terminate(pSyntax, "ast:error:unsupport opcount");
				return PX_FALSE;
			}
		}
		else
		{
			//continue
			return PX_TRUE;
		}
	}
	return PX_TRUE;
}


px_bool PX_Syntax_ExecuteOpcode(PX_Syntax* pSyntax, px_int define_opcode_index)
{
	px_abi* opcode_abi;
	const px_char* opcode_str;
	if (!PX_VectorCheckIndex(&pSyntax->reg_expr_opcode_stack, define_opcode_index))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOpcode invalid opcode index");
		return PX_FALSE;
	}
	opcode_abi = PX_Syntax_NewAbi(pSyntax, "opcode");
	if (!opcode_abi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOpcode out of memory1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(opcode_abi, "index", define_opcode_index))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOpcode out of memory2");
		return PX_FALSE;
	}
	opcode_str = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, define_opcode_index)->opcode;

	if (!PX_AbiSet_string(opcode_abi, "mnemonic", opcode_str))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOpcode out of memory3");
		return PX_FALSE;
	}

	if (!PX_Syntax_SolveExpression(pSyntax))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
}


px_int PX_Syntax_NewOpcodeDefine(PX_Syntax* pSyntax, const px_char opcode[], PX_SYNTAX_OPCODE_TYPE type, px_dword precedence)
{
	px_int i;
	PX_Syntax_opcode new_opcode = {0};
	new_opcode.offset_end_redirect_opcode_index = -1;
	for (i = 0; i < pSyntax->reg_expr_opcode_stack.size; i++)
	{
		PX_Syntax_opcode* popcode = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, i);
		if (PX_strequ(popcode->opcode, opcode) && popcode->type == type)
		{
			return i;
		}

	}
	new_opcode.type = type;
	new_opcode.precedence = precedence;
	PX_strcpy(new_opcode.opcode, opcode, sizeof(new_opcode.opcode));
	if (!PX_VectorPushback(&pSyntax->reg_expr_opcode_stack, &new_opcode))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewOpcodeAbi out of memory");
		return -1;
	}
	return pSyntax->reg_expr_opcode_stack.size - 1;
}



px_int PX_Syntax_GetOpcodeDefineIndex(PX_Syntax* pSyntax, const px_char opcode[], PX_SYNTAX_OPCODE_TYPE type)
{
	px_int i;
	for (i = pSyntax->reg_expr_opcode_stack.size - 1; i >= 0; i--)
	{
		PX_Syntax_opcode* popcode = PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, i);
		if (PX_strequ(popcode->opcode, opcode) && popcode->type == type)
		{
			return i;
		}
	}
	return -1;
}

PX_Syntax_opcode* PX_Syntax_GetOpcodeDefine(PX_Syntax* pSyntax, px_int index)
{
	if (!PX_VectorCheckIndex(&pSyntax->reg_expr_opcode_stack, index))
		return PX_NULL;

	return PX_VECTORAT(PX_Syntax_opcode, &pSyntax->reg_expr_opcode_stack, index);
}

px_int PX_Syntax_GetAbiCount(PX_Syntax* pSyntax)
{
	return pSyntax->reg_abi_stack.size;
}

px_bool PX_Syntax_NewTypeConvert(PX_Syntax* pSyntax, const px_char from_type[], const px_char to_type[],PX_Syntax_TypeConvertFunction convert_function)
{
	px_int ptype_scope_index = -1;
	px_abi* ptype_scope = PX_NULL;
	px_string payload;
	ptype_scope_index = PX_Syntax_GetTypeScopeAbiIndex(pSyntax, from_type);
	if (ptype_scope_index ==-1)
	{
		PX_ASSERTX("PX_Syntax_NewTypeConvert invalid type scope");
		return PX_FALSE;
	}
	ptype_scope = PX_Syntax_GetAbiByIndex(pSyntax, ptype_scope_index);
	PX_ASSERTIFX(!ptype_scope, "PX_Syntax_NewTypeConvert invalid type scope");
	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		return PX_FALSE;
	}
	//build payload
	if(!PX_StringFormat2(&payload, "type_defines.%1.convert.%2", PX_STRINGFORMAT_STRING(from_type), PX_STRINGFORMAT_STRING(to_type)))
		goto _ERROR;

	if (!PX_AbiSet_ptr(ptype_scope, PX_StringGetText(&payload), (px_void *)convert_function))
		goto _ERROR;

	//a type define which owns a convert function is an activated type
	if (!PX_StringFormat1(&payload, "type_defines.%1.activate", PX_STRINGFORMAT_STRING(from_type)))
		goto _ERROR;

	if (!PX_AbiSet_bool(ptype_scope, PX_StringGetText(&payload), PX_TRUE))
		goto _ERROR;

	PX_StringFree(&payload);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&payload);
	return PX_FALSE;
}

px_bool PX_Syntax_NewTypeDecorate(PX_Syntax* pSyntax, const px_char type[], const px_char Decorate[], PX_Syntax_TypeDecorateFunction Decorate_function)
{
	px_int ptype_scope_index = -1;
	px_abi* ptype_scope = PX_NULL;
	px_string payload;
	ptype_scope_index = PX_Syntax_GetTypeScopeAbiIndex(pSyntax, type);
	if (ptype_scope_index == -1)
	{
		PX_ASSERTX("PX_Syntax_NewTypeDecorate invalid type scope");
		return PX_FALSE;
	}
	ptype_scope = PX_Syntax_GetAbiByIndex(pSyntax, ptype_scope_index);
	PX_ASSERTIFX(!ptype_scope, "PX_Syntax_NewTypeDecorate invalid type scope");
	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		return PX_FALSE;
	}
	//build payload
	if (!PX_StringFormat2(&payload, "type_defines.%1.decorate.%2", PX_STRINGFORMAT_STRING(type), PX_STRINGFORMAT_STRING(Decorate)))
		goto _ERROR;
	if (!PX_AbiSet_ptr(ptype_scope, PX_StringGetText(&payload), (px_void*)Decorate_function))
		goto _ERROR;
	PX_StringFree(&payload);
	return PX_TRUE;

_ERROR:
	PX_StringFree(&payload);
	return PX_FALSE;
}

static px_bool PX_Syntax_ScopeTypeDecorate(PX_Syntax* pSyntax, px_int scope_index, px_abi* ptype_declare_abi, const px_char type[], const px_char decorate[])
{
	px_abi* pscope;
	px_string payload, type_string;
	px_bool matched = PX_FALSE;

	if (scope_index < 0 || scope_index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERTX("PX_Syntax_ScopeTypeDecorate invalid scope index");
		return PX_FALSE;
	}
	pscope = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, scope_index);
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope, "scope"), "PX_Syntax_ScopeTypeDecorate scope_index is not a scope abi");
	if (!PX_Syntax_CheckAbiName(pscope, "scope"))
		return PX_FALSE;

	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &type_string))
	{
		PX_StringFree(&payload);
		return PX_FALSE;
	}
	if (!PX_StringSet(&type_string, type))
		goto _END;

	while (PX_StringLen(&type_string) > 0)
	{
		PX_Syntax_TypeDecorateFunction decorate_function;
		if (!PX_StringFormat2(&payload, "type_defines.%1.decorate.%2", PX_STRINGFORMAT_STRING(PX_StringGetText(&type_string)), PX_STRINGFORMAT_STRING(decorate)))
			goto _END;

		if (!PX_AbiExist_Type(pscope, PX_StringGetText(&payload), PX_ABI_TYPE_PTR))
		{
			PX_StringTrimBackwardUntil(&type_string, '.');
			continue;
		}

		matched = PX_TRUE;
		decorate_function = (PX_Syntax_TypeDecorateFunction)PX_AbiGetValue_ptr(pscope, PX_StringGetText(&payload));
		if (!decorate_function(pSyntax, ptype_declare_abi))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:fail to decorate type");
		}
		goto _END;
	}
_END:
	PX_StringFree(&payload);
	PX_StringFree(&type_string);
	return matched;
}

px_void PX_Syntax_TypeDecorate(PX_Syntax* pSyntax, px_abi* ptype_declare_abi,  const px_char decorate[])
{
	px_abi type_abi;
	px_abi* pabi;
	px_int i;
	const px_char* type;
	PX_ASSERTIFX(!ptype_declare_abi || !PX_Syntax_CheckAbiName(ptype_declare_abi, "type"), "PX_Syntax_TypeDecorate invalid type declare abi");
	if (!ptype_declare_abi || !PX_Syntax_CheckAbiName(ptype_declare_abi, "type"))
		return;
	type = PX_AbiGet_string(ptype_declare_abi, "value");
	if (!PX_Syntax_GetMatchTypeAbi(pSyntax, type, &type_abi))
	{
		PX_ASSERTX("PX_Syntax_TypeDecorate invalid type");
		return;
	}
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (!PX_Syntax_CheckAbiName(pabi, "scope"))
			continue;

		if (PX_Syntax_ScopeTypeDecorate(pSyntax, i, ptype_declare_abi, type, decorate))
		{
			return;
		}
	}
}


static px_bool PX_Syntax_ScopeConvertRegisterType(PX_Syntax* pSyntax, px_int scope_index, const px_char pirabi_name[], px_int register_index, const px_char from_type[], const px_char to_type[], px_bool* pfunction_return)
{
	px_abi* pscope;
	px_string payload, from_type_string;
	px_bool matched = PX_FALSE;

	*pfunction_return = PX_FALSE;
	if (scope_index < 0 || scope_index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERTX("PX_Syntax_ScopeConvertRegisterType invalid scope index");
		return PX_FALSE;
	}
	pscope = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, scope_index);
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope, "scope"), "PX_Syntax_ScopeConvertRegisterType scope_index is not a scope abi");
	if (!PX_Syntax_CheckAbiName(pscope, "scope"))
		return PX_FALSE;

	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &from_type_string))
	{
		PX_StringFree(&payload);
		return PX_FALSE;
	}
	if (!PX_StringSet(&from_type_string, from_type))
		goto _END;

	//backtrack the composite type from bottom to top
	//eg. "pointer.ix.i.32" tries "pointer.ix.i.32","pointer.ix.i","pointer.ix" then matches "pointer"
	while (PX_StringLen(&from_type_string) > 0)
	{
		if (!PX_StringFormat2(&payload, "type_defines.%1.convert.%2", PX_STRINGFORMAT_STRING(PX_StringGetText(&from_type_string)), PX_STRINGFORMAT_STRING(to_type)))
			goto _END;

		if (!PX_AbiExist_Type(pscope, PX_StringGetText(&payload), PX_ABI_TYPE_PTR))
		{
			//cut the last '.' and everything after it,the loop ends when no '.' is left
			PX_StringTrimBackwardUntil(&from_type_string, '.');
			continue;
		}

		matched = PX_TRUE;
		{
			PX_Syntax_TypeConvertFunction convert_function = (PX_Syntax_TypeConvertFunction)PX_AbiGetValue_ptr(pscope, PX_StringGetText(&payload));
			*pfunction_return = convert_function(pSyntax, pirabi_name, register_index);
		}
		goto _END;
	}
_END:
	PX_StringFree(&payload);
	PX_StringFree(&from_type_string);
	return matched;
}

px_bool PX_Syntax_ConvertType(PX_Syntax* pSyntax, const px_char pirabi_name[], px_int register_index, const px_char from_type[], const px_char to_type[])
{
	px_abi type_abi;
	px_abi* pabi;
	px_int i;
	if (PX_strequ(from_type,to_type))
	{
		return PX_TRUE;
	}

	if (!PX_Syntax_GetMatchTypeAbi(pSyntax, from_type, &type_abi))
	{
		PX_ASSERTX("PX_Syntax_ConvertType invalid type");
		return PX_FALSE;
	}

	//search every scope from inner to outer
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_bool function_return = PX_FALSE;
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (!PX_Syntax_CheckAbiName(pabi, "scope"))
			continue;

		if (PX_Syntax_ScopeConvertRegisterType(pSyntax, i, pirabi_name, register_index, from_type, to_type, &function_return))
		{
			//the matched convert function has been called
			if (function_return == PX_TRUE)
				return PX_TRUE;
			return PX_FALSE;
		}
	}
	return PX_FALSE;
}

px_bool PX_Syntax_TypeMatch(const px_char source_type[], const px_char target_type[])
{
	//eg ix match ix,ix.i.8 match ix, ix.i.8 match ix.i.8, but ix match ix.i.8 is not match
	px_int i;
	for (i = 0; source_type[i] && target_type[i]; i++)
	{
		if (source_type[i] != target_type[i])
		{
			return PX_FALSE;
		}
	}
	if (target_type[i] == '\0' && (source_type[i] == '\0' || source_type[i] == '.'))
		return PX_TRUE;
	return PX_FALSE;
}

px_bool PX_Syntax_TypeMatch2(const px_char source_type[], const px_char target_type[], const px_char target_type2[])
{
	return PX_Syntax_TypeMatch(source_type, target_type) || PX_Syntax_TypeMatch(source_type, target_type2);
}

px_bool PX_Syntax_TypeMatch3(const px_char source_type[], const px_char target_type[], const px_char target_type2[], const px_char target_type3[])
{
	return PX_Syntax_TypeMatch(source_type, target_type) || PX_Syntax_TypeMatch(source_type, target_type2) || PX_Syntax_TypeMatch(source_type, target_type3);
}

px_bool PX_Syntax_NewType(PX_Syntax* pSyntax, const px_char type[],const px_char mnemonic[], const  px_int size, PX_Syntax_TypeSizeFunction sizefunction)
{
	px_abi* pscope=PX_NULL;
	px_int  typelen = PX_strlen(type);
	px_string payload = { 0 };
	//find scope abi
	pscope = PX_Syntax_GetLastScopeAbi(pSyntax);
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewType No Scope Found");
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp,&payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewType Memory Error 1");
		return PX_FALSE;
	}

	//the terminal node may already exist as the parent of another type(eg."ix" of "ix.i.32"),
	//only an activated terminal node means this type has already been defined
	PX_StringFormat1(&payload, "type_defines.%1.activate", PX_STRINGFORMAT_STRING(type));
	if (PX_AbiExist_bool(pscope, PX_StringGetText(&payload), PX_TRUE))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewType Type Already Defined");
		PX_StringFree(&payload);
		return PX_FALSE;
	}

	PX_StringFormat1(&payload, "type_defines.%1", PX_STRINGFORMAT_STRING(type));
	if (!PX_AbiExist_abi(pscope, PX_StringGetText(&payload)))
	{
		if (!PX_AbiSet_Abi(pscope, PX_StringGetText(&payload), PX_NULL))
			goto _ERROR;
	}

	PX_StringFormat1(&payload, "type_defines.%1.activate", PX_STRINGFORMAT_STRING(type));
	if (!PX_AbiSet_bool(pscope, PX_StringGetText(&payload), PX_TRUE))
		goto _ERROR;

	PX_StringFormat1(&payload, "type_defines.%1.size", PX_STRINGFORMAT_STRING(type));
	if (!PX_AbiSet_int(pscope, PX_StringGetText(&payload), size))
		goto _ERROR;
	
	if (sizefunction)
	{
		PX_StringFormat1(&payload, "type_defines.%1.size_function", PX_STRINGFORMAT_STRING(type));
		if (!PX_AbiSet_ptr(pscope, PX_StringGetText(&payload), (px_void*)sizefunction))
			goto _ERROR;
	}

	//build type_mnemonic
	PX_StringFormat1(&payload, "type_mnemonics.%1", PX_STRINGFORMAT_STRING(mnemonic));

	if (!PX_AbiSet_string(pscope, PX_StringGetText(&payload), type))
		goto _ERROR;

	PX_StringFree(&payload);
	return PX_TRUE;
_ERROR:
	PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewType Memory Error");
	PX_StringFree(&payload);
	return PX_FALSE;

}


px_bool PX_Syntax_NewTypeRegisterMemoryMap(PX_Syntax* pSyntax, const px_char type[], PX_Syntax_RegisterFunction memory_to_register,PX_Syntax_MemoryFunction register_to_memory)
{
	px_abi* pscope = PX_NULL;
	px_string payload = { 0 };
	//find scope abi
	pscope = PX_Syntax_GetTypeScopeAbi(pSyntax, type);
	if (!pscope)
	{
		PX_ASSERTX("runtime:error:PX_Syntax_NewTypeRegisterMap No Type Scope Found");
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewTypeRegisterMap No Type Scope Found");
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewTypeRegisterMap Memory Error 1");
		return PX_FALSE;
	}
	if (!PX_StringFormat1(&payload, "type_defines.%1.register_function", PX_STRINGFORMAT_STRING(type)))
		goto _ERROR;
	if (!PX_AbiSet_ptr(pscope, PX_StringGetText(&payload), (px_void*)memory_to_register))
		goto _ERROR;
	if (!PX_StringFormat1(&payload, "type_defines.%1.memory_function", PX_STRINGFORMAT_STRING(type)))
		goto _ERROR;
	if (!PX_AbiSet_ptr(pscope, PX_StringGetText(&payload), (px_void*)register_to_memory))
		goto _ERROR;
	//a type define which owns register/memory functions is an activated type
	if (!PX_StringFormat1(&payload, "type_defines.%1.activate", PX_STRINGFORMAT_STRING(type)))
		goto _ERROR;
	if (!PX_AbiSet_bool(pscope, PX_StringGetText(&payload), PX_TRUE))
		goto _ERROR;
	PX_StringFree(&payload);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&payload);
	return PX_FALSE;
}

px_bool PX_Syntax_GetTypeAbi(PX_Syntax* pSyntax, const px_char type[], px_abi* ptype_define_abi)
{
	px_abi* pabi;
	px_int i;
	px_string payload;
	px_bool found = PX_FALSE;
	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		PX_ASSERTX("out of memory");
		return PX_FALSE;
	}
	if (!PX_StringFormat1(&payload, "type_defines.%1", PX_STRINGFORMAT_STRING(type)))
		goto _END;

	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "scope"))
		{
			if (PX_AbiExist_abi(pabi, PX_StringGetText(&payload)))
			{
				if (ptype_define_abi)
				{
					*ptype_define_abi = PX_AbiGetValue_abireadonly(pabi, PX_StringGetText(&payload));
				}
				found = PX_TRUE;
				goto _END;
			}
		}
	}
_END:
	PX_StringFree(&payload);
	return found;
}

px_int PX_Syntax_GetTypeScopeAbiIndex(PX_Syntax* pSyntax, const px_char type[])
{
	px_abi* pabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "scope"))
		{
			px_char payload[256] = { 0 };
			PX_sprintf1(payload, sizeof(payload), "type_defines.%1", PX_STRINGFORMAT_STRING(type));
			if (PX_AbiExist_abi(pabi, payload))
			{
				return i;
			}
		}
	}
	return -1;
}

px_abi* PX_Syntax_GetTypeScopeAbi(PX_Syntax* pSyntax, const px_char type[])
{
	px_int index = PX_Syntax_GetTypeScopeAbiIndex(pSyntax, type);
	if (index == -1)
		return PX_NULL;
	return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
	
}

px_bool PX_Syntax_GetScopeMatchTypeAbi(PX_Syntax* pSyntax, px_int scope_index, const px_char type[], px_abi* ptype_define_abi)
{
	px_abi* pscope;
	px_string sub_type, payload;
	px_bool found = PX_FALSE;
	if (scope_index < 0 || scope_index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERTX("PX_Syntax_GetScopeMatchTypeAbi invalid scope index");
		return PX_FALSE;
	}
	pscope = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, scope_index);
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope, "scope"), "PX_Syntax_GetScopeMatchTypeAbi scope_index is not a scope abi");
	if (!PX_Syntax_CheckAbiName(pscope, "scope"))
		return PX_FALSE;

	if (!PX_StringInitialize(pSyntax->mp, &sub_type))
	{
		PX_ASSERTX("out of memory");
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		PX_ASSERTX("out of memory");
		PX_StringFree(&sub_type);
		return PX_FALSE;
	}
	if (!PX_StringSet(&sub_type, type)) goto _END;
	while (PX_StringLen(&sub_type) > 0)
	{
		if (!PX_StringFormat1(&payload, "type_defines.%1", PX_STRINGFORMAT_STRING(PX_StringGetText(&sub_type)))) goto _END;
		if (PX_AbiExist_abi(pscope, PX_StringGetText(&payload)))
		{
			px_abi match_abi = PX_AbiGetValue_abireadonly(pscope, PX_StringGetText(&payload));
			//only an activated terminal node is a real type
			if (PX_AbiExist_bool(&match_abi, "activate", PX_TRUE))
			{
				if (ptype_define_abi)
					*ptype_define_abi = match_abi;
				found = PX_TRUE;
				goto _END;
			}
		}
		//cut the last '.' and everything after it,the loop ends when no '.' is left
		PX_StringTrimBackwardUntil(&sub_type, '.');
	}
_END:
	PX_StringFree(&sub_type);
	PX_StringFree(&payload);
	return found;
}

px_bool PX_Syntax_GetMatchTypeAbi(PX_Syntax* pSyntax, const px_char type[], px_abi* ptype_define_abi)
{
	px_abi* pabi;
	px_int i;
	//search every scope from inner to outer
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (!PX_Syntax_CheckAbiName(pabi, "scope"))
			continue;

		if (PX_Syntax_GetScopeMatchTypeAbi(pSyntax, i, type, ptype_define_abi))
			return PX_TRUE;
	}
	return PX_FALSE;
}

px_bool PX_Syntax_MatchTypeExist(PX_Syntax* pSyntax, const px_char type[])
{
	px_abi type_define_abi;
	return PX_Syntax_GetMatchTypeAbi(pSyntax, type, &type_define_abi);
}

px_int PX_Syntax_GetTypeSize(PX_Syntax* pSyntax, const px_char type[])
{
	px_abi type_define_abi;
	px_int size;
	if (!PX_Syntax_GetTypeAbi(pSyntax, type, &type_define_abi))
		return 0;
	size = PX_AbiGetValue_int(&type_define_abi, "size");
	if (size)
		return size;
	if (PX_AbiExist_Type(&type_define_abi, "size_function", PX_ABI_TYPE_PTR))
	{
		PX_Syntax_TypeSizeFunction size_function = (PX_Syntax_TypeSizeFunction)PX_AbiGetValue_ptr(&type_define_abi, "size_function");
		return size_function(pSyntax, type, &type_define_abi);
	}
	return 0;
}

px_int PX_Syntax_GetMatchTypeSize(PX_Syntax* pSyntax, const px_char type[])
{
	px_abi type_define_abi;
	px_int size;
	if (!PX_Syntax_GetMatchTypeAbi(pSyntax, type, &type_define_abi))
		return 0;
	size = PX_AbiGetValue_int(&type_define_abi, "size");
	if (size)
		return size;
	if (PX_AbiExist_Type(&type_define_abi, "size_function", PX_ABI_TYPE_PTR))
	{
		PX_Syntax_TypeSizeFunction size_function = (PX_Syntax_TypeSizeFunction)PX_AbiGetValue_ptr(&type_define_abi, "size_function");
		return size_function(pSyntax, type, &type_define_abi);
	}
	return 0;
}

const px_char* PX_Syntax_GetTypeByMnemonic(PX_Syntax* pSyntax, const px_char mnemonic[])
{
	px_abi* pabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "scope"))
		{
			px_char payload[256] = { 0 };
			PX_sprintf1(payload, sizeof(payload), "type_mnemonics.%1", PX_STRINGFORMAT_STRING(mnemonic));
			if (PX_AbiExist_Type(pabi, payload, PX_ABI_TYPE_STRING))
			{
				return PX_AbiGet_string(pabi, payload);
			}
		}
	}
	return PX_NULL;
}

px_bool PX_Syntax_NewTypedef(PX_Syntax* pSyntax, const px_char type[], const px_char mnemonic[])
{
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, "scope"))
		{
			//find type define abi
			px_char payload[256] = { 0 };
			PX_sprintf1(payload, sizeof(payload), "type_defines.%1", PX_STRINGFORMAT_STRING(type));
			if (PX_AbiExist_abi(pabi, payload))
			{
				//new mnemonic
				PX_sprintf1(payload, sizeof(payload), "type_mnemonics.%1", PX_STRINGFORMAT_STRING(mnemonic));
				if (PX_AbiExist_Type(pabi,payload,PX_ABI_TYPE_STRING))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewTypedef Type Mnemonic Already Exist");
					return PX_FALSE;
				}
				return PX_AbiSet_string(pabi, payload, type);
			}
		}
	}
	PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_NewTypedef Type Not Found");
	return PX_FALSE;
}

px_bool PX_Syntax_EnterScope(PX_Syntax* pSyntax)
{
	px_abi* pnewabi;
	px_int alloc_offset=0;
	px_abi* pparent_scope = PX_Syntax_GetLastScopeAbi(pSyntax);
	px_int scope_count = 0;

	if (pparent_scope)
	{
		alloc_offset = PX_AbiGetValue_int(pparent_scope, "alloc_local_offset");
	}

	if (PX_NULL==( pnewabi=PX_Syntax_NewAbi(pSyntax, "scope")))
	{
		PX_ASSERTX("PX_Syntax_EnterScope memory error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pnewabi, "alloc_local_offset", alloc_offset))
	{
		PX_AbiFree(pnewabi);
		PX_ASSERTX("PX_Syntax_EnterScope memory error2");
		return PX_FALSE;
	}
	return PX_TRUE;
	
}

px_bool PX_Syntax_EnterBaseScope(PX_Syntax* pSyntax)
{
	if (PX_Syntax_EnterScope(pSyntax))
	{
		px_abi* plast_scope = PX_Syntax_GetLastScopeAbi(pSyntax);
		if (!PX_AbiSet_bool(plast_scope, "base", 1))
		{
			PX_ASSERTX("PX_Syntax_EnterBaseScope memory error");
			return PX_FALSE;
		}
		if (!PX_AbiSet_int(plast_scope, "alloc_local", 0))
		{
			PX_ASSERTX("PX_Syntax_EnterBaseScope memory error");
			return PX_FALSE;
		}
		if (!PX_AbiSet_int(plast_scope, "alloc_global", 0))
		{
			PX_ASSERTX("PX_Syntax_EnterBaseScope memory error");
			return PX_FALSE;
		}
		if (!PX_AbiSet_int(plast_scope, "alloc_param", 0))
		{
			PX_ASSERTX("PX_Syntax_EnterBaseScope memory error");
			return PX_FALSE;
		}
		if (!PX_AbiSet_string(plast_scope, "id", PX_Syntax_AllocUnnamed(pSyntax, "_scope_base_")))
		{
			PX_ASSERTX("PX_Syntax_EnterBaseScope memory error");
			return PX_FALSE;
		}
		return PX_TRUE;
	}
	return PX_FALSE;
}

px_bool PX_Syntax_LeaveScope(PX_Syntax* pSyntax)
{
	px_abi* pscope_abi = PX_Syntax_GetLastScopeAbi(pSyntax);
	px_int alloc_size=0;

	PX_ASSERTIFX(pscope_abi == PX_NULL, "PX_Syntax_scope_end:pscope_abi==PX_NULL");

	if(PX_AbiExist_bool(pscope_abi,"base",PX_TRUE))
		alloc_size = PX_AbiGetValue_int(pscope_abi, "alloc_local");

	if (alloc_size)
	{
		//sub sp,alloc_size
		px_char payload[32];
		PX_sprintf1(payload, sizeof(payload), "sub sp,%1\n", PX_STRINGFORMAT_INT(alloc_size));
		if (!PX_AbiAppend_string(pscope_abi, "ir", payload))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			return PX_FALSE;
		}
	}

	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "scope", "ir", "ir"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
		return PX_FALSE;
	}

	if (!PX_Syntax_MergeAbiFromBeginToEnd(pSyntax, "scope", "ir_library", "ir_library"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
		return PX_FALSE;
	}

	if (alloc_size)
	{
		//add sp,alloc_size
		px_char payload[32] = {0};
		PX_sprintf1(payload,sizeof(payload), "add sp,%1\n", PX_STRINGFORMAT_INT(alloc_size));
		if (!PX_AbiAppend_string(pscope_abi, "ir", payload))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_scope_end memory error");
			return PX_FALSE;
		}
	}
	PX_Syntax_PopAbiFromBeginNext(pSyntax, "scope");
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "scope"), "PX_Syntax_scope_end:Last Abi is not scope");
	PX_Syntax_RenameLastAbi(pSyntax, "scope_block");
	return PX_TRUE;
}

px_abi* PX_Syntax_GetLastScopeAbi(PX_Syntax* pSyntax)
{
	px_abi* plastabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			return plastabi;
		}
	}
	return PX_NULL;
	
}

px_abi* PX_Syntax_GetSecondLastScopeAbi(PX_Syntax* pSyntax)
{
	px_abi* plastabi;
	px_bool secondlast = PX_FALSE;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (secondlast)
				return plastabi;
			else
				secondlast = PX_TRUE;
		}
	}
	return PX_NULL;
}

px_abi* PX_Syntax_GetLastBaseScopeAbi(PX_Syntax* pSyntax)
{
	px_abi* plastabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (PX_AbiExist_bool(plastabi, "base", PX_TRUE))
			{
				return plastabi;
			}
		}
	}
	PX_ASSERTX("no base scope abi");
	return PX_NULL;
	
}

static px_int PX_Syntax_GetLastBaseScopeAlloc(PX_Syntax* pSyntax, const px_char member_payload[])
{
	px_abi* plastabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (PX_AbiExist_bool(plastabi, "base", PX_TRUE))
			{
				return PX_AbiGetValue_int(plastabi, member_payload);
			}
		}
	}
	PX_ASSERTX("no base scope abi");
	return 0;
}

static px_int PX_Syntax_GetLastBaseScopeAllocLocal(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetLastBaseScopeAlloc(pSyntax, "alloc_local");
}

static px_int PX_Syntax_GetLastBaseScopeAllocGlobal(PX_Syntax* pSyntax)
{
	px_abi* plastabi;
	px_int i;
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (PX_AbiExist_bool(plastabi, "base", PX_TRUE))
			{
				return PX_AbiGetValue_int(plastabi, "alloc_global");
			}
		}
	}
	return 0;
}

static px_int PX_Syntax_GetLastBaseScopeAllocParam(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetLastBaseScopeAlloc(pSyntax, "alloc_param");
}

static px_void PX_Syntax_SetLastBaseScopeAlloc(PX_Syntax* pSyntax, const px_char member_payload[],px_int value)
{
	px_abi* plastabi;
	px_int index;
	for (index = pSyntax->reg_abi_stack.size - 1; index >= 0; index--)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (PX_AbiExist_bool(plastabi, "base", PX_TRUE))
			{
				PX_AbiSet_int(plastabi, member_payload, value);
				return;
			}
		}
	}
	PX_ASSERTX("no base scope abi");
}

static px_void PX_Syntax_SetLastBaseScopeAllocLocal(PX_Syntax* pSyntax, px_int value)
{
	PX_Syntax_SetLastBaseScopeAlloc(pSyntax, "alloc_local", value);
}

static px_void PX_Syntax_SetLastBaseScopeAllocGlobal(PX_Syntax* pSyntax, px_int value)
{
	px_abi* plastabi;
	px_int i;
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		plastabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(plastabi, "scope"))
		{
			if (PX_AbiExist_bool(plastabi, "base", PX_TRUE))
			{
				PX_AbiSet_int(plastabi, "alloc_global", value);
				return;
			}
		}
	}
	PX_ASSERTX("no base scope abi");
}

static px_void PX_Syntax_SetLastBaseScopeAllocParam(PX_Syntax* pSyntax, px_int value)
{
	PX_Syntax_SetLastBaseScopeAlloc(pSyntax, "alloc_param", value);
}

px_int PX_Syntax_AllocLocal(PX_Syntax* pSyntax, px_int size)
{
	px_abi* pabi = PX_Syntax_GetAbiFromBackward(pSyntax, "scope");
	px_int  alloc, alloc_offset;
	if (!pabi)
	{
		PX_ASSERTX("no scope abi");
		return 0;
	}
	alloc = PX_Syntax_GetLastBaseScopeAllocLocal(pSyntax);
	alloc_offset = PX_AbiGetValue_int(pabi, "alloc_local_offset");

	PX_Syntax_SetLastBaseScopeAllocLocal(pSyntax, alloc + size);

	if (!PX_AbiSet_int(pabi, "alloc_local_offset", alloc_offset + size))
	{
		PX_ASSERTX("PX_Syntax_ScopeAllocLocal memory error");
		return 0;
	}
	return alloc_offset;
}

px_int PX_Syntax_AllocGlobal(PX_Syntax* pSyntax, px_int size)
{
	px_int alloc,offset;
	alloc = PX_Syntax_GetLastBaseScopeAllocGlobal(pSyntax);
	offset = alloc;
	PX_Syntax_SetLastBaseScopeAllocGlobal(pSyntax, alloc + size);
	return offset;
}

px_int PX_Syntax_AllocParam(PX_Syntax* pSyntax, px_int size)
{
	px_int alloc, offset;
	if(size<4)size=4;
	alloc = PX_Syntax_GetLastBaseScopeAllocParam(pSyntax);
	offset = alloc;
	PX_Syntax_SetLastBaseScopeAllocParam(pSyntax, alloc + size);
	return offset;
}

px_int PX_Syntax_AllocRdata(PX_Syntax* pSyntax, const px_byte* buffer, px_int size)
{
	px_abi* pabi = PX_Syntax_GetAbiFromBackward(pSyntax, "scope");
	px_int  alloc_offset,zero_append=4-(size&3);
	if (!pabi)
	{
		PX_ASSERTX("no scope abi");
		return 0;
	}
	alloc_offset = PX_AbiGet_buffer_size(pabi, "rdata");
	
	if (!PX_AbiAppend_buffer(pabi,"rdata",buffer,size))
	{
		PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_AllocRdata out of memory.");
		return alloc_offset;
	}
	if (zero_append)
	{
		px_byte zero[4] = { 0 };
		if (!PX_AbiAppend_buffer(pabi, "rdata", zero, zero_append))
		{
			PX_Syntax_Terminate(pSyntax, "ast:runtime:error:PX_Syntax_AllocRdata out of memory.");
			return alloc_offset;
		}
	}
	return alloc_offset;
}


px_abi* PX_Syntax_GetAbiByIndex(PX_Syntax* pSyntax, px_int index)
{
	if (index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERT();
		return PX_NULL;
	}
	if (index < -pSyntax->reg_abi_stack.size)
	{
		PX_ASSERT();
		return PX_NULL;
	}
	if(index>=0)
		return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
	else
		return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size + index);
	
}

px_abi* PX_Syntax_GetLastAbi(PX_Syntax* pSyntax)
{
	if (pSyntax->reg_abi_stack.size == 0)
	{
		return PX_NULL;
	}
	return PX_VECTORLAST(px_abi, &pSyntax->reg_abi_stack);
}

px_abi* PX_Syntax_GetSecondLastAbi(PX_Syntax* pSyntax)
{
	if (pSyntax->reg_abi_stack.size < 2)
	{
		return PX_NULL;
	}
	return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size - 2);
}

px_abi* PX_Syntax_GetThirdLastAbi(PX_Syntax* pSyntax)
{
	if (pSyntax->reg_abi_stack.size < 3)
	{
		return PX_NULL;
	}
	return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size - 3);
}

px_abi* PX_Syntax_NewAbiLifetime(PX_Syntax* pSyntax, const px_char name[], px_int lifetime)
{
	px_abi abi;
	px_int insert_index= pSyntax->reg_abi_stack.size-1;
	px_int new_index;
	PX_AbiCreate_DynamicWriter(&abi, pSyntax->mp);

	if (!PX_AbiSet_string(&abi, "name", name))
	{
		PX_AbiFree(&abi);
		return PX_NULL;
	}

	while (insert_index >= 0)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, insert_index);
		px_int current_lifetime = PX_Syntax_GetLifetime(pSyntax);

		if (lifetime <= current_lifetime)
		{
			break;
		}
		insert_index--;
	}
	
	new_index = insert_index + 1;
	if (!PX_VectorInsertAfter(&pSyntax->reg_abi_stack, insert_index, &abi))
	{
		PX_AbiFree(&abi);
		return PX_NULL;
	}
	
	return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, new_index);
}

px_abi* PX_Syntax_NewAbi(PX_Syntax* pSyntax, const px_char name[])
{
	px_abi abi;
	PX_AbiCreate_DynamicWriter(&abi, pSyntax->mp);
	if (!PX_AbiSet_string(&abi, "name", name))
	{
		PX_AbiFree(&abi);
		return PX_NULL;
	}
	if(!PX_VectorPushback(&pSyntax->reg_abi_stack, &abi))
	{
		PX_AbiFree(&abi);
		return PX_NULL;
	}
	return PX_VECTORLAST(px_abi, &pSyntax->reg_abi_stack);
}

px_void PX_Syntax_PopAbi(PX_Syntax* pSyntax)
{
	px_abi* pabi;
	if (pSyntax->reg_abi_stack.size == 0)
	{
		PX_ASSERT();
	}
	//free abi
	pabi = PX_VECTORLAST(px_abi, &pSyntax->reg_abi_stack);
	PX_AbiFree(pabi);
	PX_VectorPop(&pSyntax->reg_abi_stack);
}

px_void PX_Syntax_PopLastSecondAbi(PX_Syntax* pSyntax)
{
	px_abi* pabi;
	if (pSyntax->reg_abi_stack.size < 2)
	{
		PX_ASSERT();
	}
	pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size - 2);
	PX_AbiFree(pabi);
	PX_VectorErase(&pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size - 2);
}

px_void PX_Syntax_PopAbiIndex(PX_Syntax* pSyntax,px_int index)
{
	px_abi* pabi;
	if (index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERT();
	}
	if (index < -pSyntax->reg_abi_stack.size)
	{
		PX_ASSERT();
	}
	if (index >= 0)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
		PX_AbiFree(pabi);
		PX_VectorErase(&pSyntax->reg_abi_stack, index);
	}
	else
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size + index);
		PX_AbiFree(pabi);
		PX_VectorErase(&pSyntax->reg_abi_stack, pSyntax->reg_abi_stack.size + index);
	}
}

px_bool PX_Syntax_MergeLastAbi(PX_Syntax* pSyntax, const px_char new_name[])
{
	px_abi* ppnewabi = PX_Syntax_NewAbi(pSyntax, new_name);
	px_abi* poldlastabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	const px_char* pname;
	px_char content[128] = { 0 };
	if (!ppnewabi || !poldlastabi)
	{
		return PX_FALSE;
	}
	pname = PX_AbiGet_string(poldlastabi, "name");
	if (!pname)
	{
		return PX_FALSE;
	}
	PX_sprintf2(content, sizeof(content), "Merge %1 to %2\n", PX_STRINGFORMAT_STRING(pname), PX_STRINGFORMAT_STRING(new_name));
	PX_Syntax_Message(pSyntax, content);
	if (!PX_AbiSet_Abi(ppnewabi, pname, poldlastabi))
	{
		return PX_FALSE;
	}
	PX_Syntax_PopLastSecondAbi(pSyntax);
	
	return PX_TRUE;
}

px_bool PX_Syntax_MergeLast2AbiToSecondLast(PX_Syntax* pSyntax)
{
	px_abi* plastabi, * psecondlastabi;
	const px_char* plastname, * psecondlastname;
	px_char content[128] = { 0 };
	plastabi = PX_Syntax_GetLastAbi(pSyntax);
	psecondlastabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	if (!plastabi || !psecondlastabi)
	{
		return PX_FALSE;
	}
	plastname = PX_AbiGet_string(plastabi, "name");
	psecondlastname = PX_AbiGet_string(psecondlastabi, "name");
	if (!plastname || !psecondlastname)
	{
		return PX_FALSE;
	}
	PX_sprintf2(content, sizeof(content), "Merge %1 to %2\n", PX_STRINGFORMAT_STRING(plastname), PX_STRINGFORMAT_STRING(psecondlastname));
	PX_Syntax_Message(pSyntax, content);
	if (!PX_AbiSet_Abi(psecondlastabi, plastname, plastabi))
	{
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	
	return PX_TRUE;
}

px_bool PX_Syntax_RenameLastAbi(PX_Syntax* pSyntax, const px_char name[])
{
	px_abi* plastabi;
	const px_char* plastname;
	px_char content[128] = { 0 };
	plastabi = PX_Syntax_GetLastAbi(pSyntax);
	if (!plastabi)
	{
		return PX_FALSE;
	}
	plastname = PX_AbiGet_string(plastabi, "name");
	if (!plastname)
	{
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(plastabi, "name", name))
	{
		return PX_FALSE;
	}
	return PX_TRUE;
	
}

px_bool PX_Syntax_MergeLast2AbiWithNameToSecondLast(PX_Syntax* pSyntax,const px_char newname[])
{
	px_abi* plastabi, * psecondlastabi;
	const px_char* plastname, * psecondlastname;
	px_char content[128] = { 0 };
	plastabi = PX_Syntax_GetLastAbi(pSyntax);
	psecondlastabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	if (!plastabi || !psecondlastabi)
	{
		return PX_FALSE;
	}
	plastname = PX_AbiGet_string(plastabi, "name");
	psecondlastname = PX_AbiGet_string(psecondlastabi, "name");
	if (!plastname || !psecondlastname)
	{
		return PX_FALSE;
	}
	PX_sprintf4(content, sizeof(content), "Merge %1,%2 to %3 with name %4\n", PX_STRINGFORMAT_STRING(plastname), PX_STRINGFORMAT_STRING(psecondlastname),\
		PX_STRINGFORMAT_STRING(psecondlastname), PX_STRINGFORMAT_STRING(newname));
	PX_Syntax_Message(pSyntax, content);
	if (!PX_AbiSet_Abi(psecondlastabi, newname, plastabi))
	{
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	
	return PX_TRUE;
}

px_bool PX_Syntax_MergeAbiIndexToAbiIndexWithName(PX_Syntax* pSyntax, px_int src_index, px_int dst_index, const px_char name[])
{
	px_abi* pabi1, * pabi2;
	const px_char* name1, * name2;
	px_char content[128] = { 0 };
	pabi1 = PX_Syntax_GetAbiByIndex(pSyntax, src_index);
	pabi2 = PX_Syntax_GetAbiByIndex(pSyntax, dst_index);
	if (!pabi1 || !pabi2)
	{
		return PX_FALSE;
	}
	if (pabi1 == pabi2)
	{
		PX_ASSERTX("PX_Syntax_MergeAbiIndexToAbiIndexWithName: source and target are the same abi");
		return PX_FALSE;
	}
	name1 = PX_AbiGet_string(pabi1, "name");
	name2 = PX_AbiGet_string(pabi2, "name");
	if (!name1 || !name2)
	{
		return PX_FALSE;
	}
	PX_sprintf4(content, sizeof(content), "Merge %1,%2 to %3 with name %4\n", PX_STRINGFORMAT_STRING(name1), PX_STRINGFORMAT_STRING(name2), PX_STRINGFORMAT_INT(dst_index), PX_STRINGFORMAT_STRING(name));
	PX_Syntax_Message(pSyntax, content);
	if (!PX_AbiSet_Abi(pabi2, name, pabi1))
	{
		return PX_FALSE;
	}
	PX_Syntax_PopAbiIndex(pSyntax, src_index);
	return PX_TRUE;
}

px_bool PX_Syntax_Initialize(px_memorypool* _mp, PX_Syntax* pSyntax)
{
	PX_memset(pSyntax, 0, sizeof(PX_Syntax));
	pSyntax->mp = _mp;
	if(!PX_SyntaxLexer_Initialize(_mp,&pSyntax->reg_syntaxlexer))
		return PX_FALSE;
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, ',');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '.');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, ';');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, ':');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '+');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '-');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '*');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '/');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '%');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '&');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '^');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '~');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '(');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, ')');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '!');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '=');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '>');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '<');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '{');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '}');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '[');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, ']');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '#');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '|');
	PX_SyntaxLexer_RegisterDelimiter(&pSyntax->reg_syntaxlexer, '\"');
	PX_SyntaxLexer_RegisterDiscard(&pSyntax->reg_syntaxlexer, "\\\n");

	PX_SyntaxLexer_RegisterSpacer(&pSyntax->reg_syntaxlexer, ' ');
	PX_SyntaxLexer_RegisterSpacer(&pSyntax->reg_syntaxlexer, '\t');
	//PX_SyntaxLexer_RegisterComment(&pSyntax->reg_syntaxlexer, "//", "\n");
	//PX_SyntaxLexer_RegisterComment(&pSyntax->reg_syntaxlexer, "/*", "*/");

	if (!PX_StringInitialize(_mp, &pSyntax->message))
		return PX_FALSE;

	PX_VectorInitialize(_mp, &pSyntax->pebnf, sizeof(PX_Syntax_pebnf), 0);
	PX_VectorInitialize(_mp, &pSyntax->reg_ast_stack, sizeof(PX_Syntax_ast), 0);
	PX_VectorInitialize(_mp, &pSyntax->reg_abi_stack, sizeof(px_abi), 0);
	PX_VectorInitialize(_mp, &pSyntax->reg_ast_instr_stack, sizeof(px_abi), 0);
	PX_VectorInitialize(_mp, &pSyntax->reg_maptype_stack, sizeof(PX_Syntax_maptype), 0);
	PX_VectorInitialize(_mp, &pSyntax->reg_expr_opcode_stack, sizeof(PX_Syntax_opcode), 32);


	pSyntax->reg_lastsource_index = -1;
	pSyntax->reg_expr_begin_line = 0;
	pSyntax->reg_expr_source_index = -1;
	pSyntax->reg_write_expr_begin_line = -1;
	pSyntax->reg_write_expr_source_index = -1;

	PX_memset(&pSyntax->bnfnode_return, 0, sizeof(PX_Syntax_bnfnode));
	PX_StringInitialize(_mp, &pSyntax->bnfnode_return.constant);

	return PX_TRUE;
}

px_bool PX_Syntax_MergeLast2AbiValue(PX_Syntax* pSyntax,const px_char last_abi_name[],const px_char value_name[],const px_char second_last_abi_name[],const px_char value_name2[])
{
	px_int last_abi_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, last_abi_name);
	px_abi* plast_ir_abi = PX_Syntax_GetAbiByIndex(pSyntax, last_abi_index);
	px_abi* psecondlast_ir_abi = PX_Syntax_GetAbiFromBackward(pSyntax, second_last_abi_name);
	const px_char* pvalue;
	if (!plast_ir_abi)
	{
		PX_ASSERTX("last abi not found");
		return PX_FALSE;
	}
	pvalue = PX_AbiGet_string(plast_ir_abi, value_name);
	if (!pvalue)
	{
		PX_ASSERTX("value not found");
		return PX_FALSE;
	}
	if (!psecondlast_ir_abi)
	{
		PX_ASSERTX("second last abi not found");
		return PX_FALSE;
	}
	else
	{
		if (!PX_AbiAppend_string(psecondlast_ir_abi, value_name2, pvalue))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_AppendString Memory Error2");
			return PX_FALSE;
		}
		PX_Syntax_PopAbiIndex(pSyntax, last_abi_index);
	}
	return PX_TRUE;
}
px_bool PX_Syntax_MergeAbiFromBeginToEnd(PX_Syntax* pSyntax, const px_char begin_abi_name[], const px_char search_name[],const px_char merge_to_name[])
{
	px_int i;
	px_string IRs;
	px_abi* pbegin_abi;
	px_int begin_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, begin_abi_name);
	if (begin_index == -1) return PX_FALSE;
	if (!PX_StringInitialize(pSyntax->mp, &IRs)) return PX_FALSE;
	for (i = begin_index+1; i < pSyntax->reg_abi_stack.size; i++)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_AbiExist_Type(pabi, search_name,PX_ABI_TYPE_STRING))
		{
			if (!PX_StringCat(&IRs, PX_AbiGetValue_string(pabi, search_name)))
			{
				goto _ERROR;
			}
		}
	}

	pbegin_abi = PX_Syntax_GetAbiByIndex(pSyntax, begin_index);
	if (!PX_AbiAppend_string(pbegin_abi, merge_to_name, PX_StringGetText(&IRs)))
	{
		goto _ERROR;
	}

	PX_StringFree(&IRs);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&IRs);
	return PX_FALSE;
}
px_void PX_Syntax_PopAbiFromBeginNext(PX_Syntax* pSyntax, const px_char begin_abi_name[])
{
	px_int begin_index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, begin_abi_name);
	if (begin_index == -1) return;
	while (begin_index + 1 < pSyntax->reg_abi_stack.size)
	{
		px_abi* pabi = PX_VECTORLAST(px_abi, &pSyntax->reg_abi_stack);
		PX_ASSERTIFX(!pabi || PX_Syntax_CheckAbiName(pabi, "scope"), "the scope abi should not be popped directly");
		PX_Syntax_PopAbi(pSyntax);
	}
}

px_bool PX_Syntax_AppendBuffer(PX_Syntax* pSyntax, const px_char name[], const char data_name[], const px_byte data[], px_int datasize)
{
	px_abi* pabi = PX_Syntax_GetAbiFromBackward(pSyntax, name);
	if (!pabi)
	{
		PX_ASSERTX("could not found abi");
		return PX_FALSE;
	}

	if (!PX_AbiAppend_buffer(pabi, data_name, data, datasize))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_AppendData Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}



px_bool PX_Syntax_AppendString(PX_Syntax* pSyntax, const px_char name[],const char content_name[] ,const px_char content[])
{
	px_abi* pabi = PX_Syntax_GetAbiFromBackward(pSyntax, name);
	if (!pabi)
	{
		PX_ASSERTX("could not found abi");
		return PX_FALSE;
	}

	if (!PX_AbiGet_string(pabi, content_name))
	{
		if (PX_AbiSet_string(pabi,content_name,content))
		{
			return PX_TRUE;
		}
		else
		{
			return PX_FALSE;
		}
	}

	if (!PX_AbiAppend_string(pabi, content_name, content))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_AppendString Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}


static px_int PX_Syntax_CreateMap(PX_Syntax* pSyntax, PX_SyntaxLexer_Source* psource, px_int sourceindex, px_int begin, px_int end)
{
	px_abi wabi;
	PX_AbiCreate_DynamicWriter(&wabi, pSyntax->mp);
	if (!PX_AbiSet_int(&wabi, "source_index", sourceindex))
	{
		PX_AbiFree(&wabi);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_new Memory Error1");
		return -1;
	}

	if (!PX_AbiSet_int(&wabi, "begin", begin))
	{
		PX_AbiFree(&wabi);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_new Memory Error1");
		return -1;
	}
	if (!PX_AbiSet_int(&wabi, "end", end))
	{
		PX_AbiFree(&wabi);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_new Memory Error2");
		return -1;
	}
	if (!PX_VectorPushback(&psource->descriptor, &wabi))
	{
		PX_AbiFree(&wabi);
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_new Memory Error3");
		return -1;
	}
	return psource->last_descriptor_index = psource->descriptor.size - 1;
}

px_abi* PX_Syntax_NewMap(PX_Syntax* pSyntax, px_int sourceindex,px_int begin, px_int end, const px_char key[])
{
	px_int i,x;
	PX_SyntaxLexer_Source* psource = PX_SyntaxLexer_GetSourceByIndex(&pSyntax->reg_syntaxlexer, sourceindex);
	
	if (!psource)
	{
		PX_ASSERTX("could not found source");
		return PX_NULL;
	}
	i = psource->descriptor.size;
	x = 0;
	while (i>0&&x<=32)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &psource->descriptor, i - 1);
		px_int source_index = PX_AbiGetValue_int(pabi, "source_index");
		px_int tbegin = PX_AbiGetValue_int(pabi, "begin");
		px_int tend = PX_AbiGetValue_int(pabi, "end");
		if (source_index== sourceindex&&begin == tbegin && end == tend)
		{
			psource->last_descriptor_index = i - 1;
			return pabi;
		}
		i--;
		x++;
	}

	if (key&&key[0])
	{
		px_int index;
		if (PX_MapGetInt(&psource->descriptor_map,key,PX_strlen(key), &index))
		{
			PX_ASSERTIFX(!PX_VectorCheckIndex(&psource->descriptor, index), "PX_Syntax_NewMap: invalid descriptor index");
			psource->last_descriptor_index = index;
		}
		else
		{
			if((index=PX_Syntax_CreateMap(pSyntax, psource, sourceindex, begin, end)) < 0)
			{
				return PX_NULL;
			}
			PX_MapPutInt(&psource->descriptor_map, key, PX_strlen(key), index);
			psource->last_descriptor_index = index;
		}
	}
	else
	{
		if ((psource->last_descriptor_index=PX_Syntax_CreateMap(pSyntax, psource, sourceindex, begin, end)) < 0)
		{
			return PX_NULL;
		}
	}
	
	for (i = begin; i <= end; i++)
	{
		px_int map_cell_index;
		if (i >= psource->source_length)
		{
			PX_ASSERTX("Assert Error: invalid map cell index");
			return PX_NULL;
		}
		map_cell_index = psource->source_index_map_to_cell_index[i];
		if (map_cell_index < 0 || map_cell_index >= psource->cells_count)
		{
			PX_ASSERTX("Assert Error: invalid map cell index");
			return PX_NULL;
		}
		psource->cells[map_cell_index].abi_index = psource->last_descriptor_index;
	}
	return PX_VECTORAT(px_abi, &psource->descriptor, psource->last_descriptor_index);
}

static px_abi* PX_Syntax_NewMapTokenEx(PX_Syntax* pSyntax, px_int begin_sourceindex, px_int begin, px_int end_sourceindex, px_int end, px_color color, const px_char key[],const px_char type[])
{
	PX_SyntaxLexer_Source* psource;
	px_abi* pabi;
	if (type==PX_NULL)
	{
		type = "";
	}
	if (begin_sourceindex!= end_sourceindex)
	{
		return PX_NULL;
	}

	psource = PX_SyntaxLexer_GetSourceByIndex(&pSyntax->reg_syntaxlexer, begin_sourceindex);
	if (!psource)
	{
		PX_ASSERTX("could not found source");
		return PX_NULL;
	}
	if (PX_Syntax_NewMap(pSyntax, begin_sourceindex, begin, end, key) < 0)
		return PX_NULL;

	pabi = PX_VECTORAT(px_abi, &psource->descriptor, psource->last_descriptor_index);
	if (!PX_AbiSet_color(pabi, "color", color))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_color Memory Error");
		return PX_NULL;
	}
	if (!PX_AbiSet_string(pabi, "type", type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ide_type Memory Error1");
		return PX_NULL;
	}
	return pabi;
}

px_abi* PX_Syntax_NewStaticMapToken(PX_Syntax* pSyntax, px_int begin_sourceindex, px_int begin, px_int end_sourceindex, px_int end, px_color color, const px_char type[])
{
	return PX_Syntax_NewMapTokenEx(pSyntax, begin_sourceindex, begin, end_sourceindex, end, color, type, type);
}

px_abi* PX_Syntax_NewDynamicMapToken(PX_Syntax* pSyntax, px_int begin_sourceindex, px_int begin, px_int end_sourceindex, px_int end, px_color color, const px_char type[])
{
	return PX_Syntax_NewMapTokenEx(pSyntax, begin_sourceindex, begin, end_sourceindex, end, color, "", type);
}

px_bool PX_Syntax_SetLastMapInfo(PX_Syntax* pSyntax, px_int sourceindex, const px_char info[])
{
	PX_SyntaxLexer_Source* psource;
	px_abi* pabi;
	psource = PX_SyntaxLexer_GetSourceByIndex(&pSyntax->reg_syntaxlexer, sourceindex);
	if (!psource)
	{
		PX_ASSERTX("could not found source");
		return PX_FALSE;
	}
	if (!PX_VectorCheckIndex(&psource->descriptor, psource->last_descriptor_index))
	{
		PX_ASSERTX("PX_Syntax_NewLastMapInfo: invalid descriptor index");
		return PX_FALSE;
	}

	pabi = PX_VECTORAT(px_abi, &psource->descriptor, psource->last_descriptor_index);
	if (!PX_AbiSet_string(pabi, "info", info))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_LastMapInfo Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}



px_bool PX_Syntax_NewIRInstruction3(PX_Syntax* pSyntax,const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[], const px_char operand2[], const px_char operand3[])
{
	px_char content[64] = { 0 };
	if (pSyntax->reg_write_expr_begin_line!=pSyntax->reg_expr_begin_line|| pSyntax->reg_write_expr_source_index != pSyntax->reg_expr_source_index)
	{
		PX_sprintf2(content, sizeof(content), ";@loc %1 %2\n", PX_STRINGFORMAT_INT(pSyntax->reg_expr_source_index), PX_STRINGFORMAT_INT(pSyntax->reg_expr_begin_line));
		if (!PX_Syntax_AppendString(pSyntax, merge_to_abi,"ir", content))return PX_FALSE;
		pSyntax->reg_write_expr_begin_line = pSyntax->reg_expr_begin_line;
		pSyntax->reg_write_expr_source_index = pSyntax->reg_expr_source_index;
	}
	if (opcode[0]==0)
	{
		return PX_TRUE;
	}
	content[0] = '\0';
	PX_strcat_s(content,sizeof(content), opcode);
	if (operand1[0])
	{
		PX_strcat_s(content,sizeof(content), " ");
		PX_strcat_s(content,sizeof(content),operand1);
	}
	if (operand2[0])
	{
		PX_strcat_s(content,sizeof(content), ",");
		PX_strcat_s(content,sizeof(content), operand2);
	}
	if (operand3[0])
	{
		PX_strcat_s(content,sizeof(content), ",");
		PX_strcat_s(content,sizeof(content),operand3);
	}
	PX_strcat_s(content,sizeof(content), "\n");
	if (!PX_Syntax_AppendString(pSyntax, merge_to_abi, "ir", content))return PX_FALSE;
	return PX_TRUE;
}

px_bool PX_Syntax_NewIRLocation(PX_Syntax* pSyntax, const px_char merge_to_abi[])
{
	px_char content[64] = { 0 };
	PX_sprintf2(content, sizeof(content), ";@loc %1 %2\n", PX_STRINGFORMAT_INT(pSyntax->reg_expr_source_index), PX_STRINGFORMAT_INT(pSyntax->reg_expr_begin_line));
	if (!PX_Syntax_AppendString(pSyntax, merge_to_abi, "ir", content))return PX_FALSE;
	pSyntax->reg_write_expr_begin_line = pSyntax->reg_expr_begin_line;
	pSyntax->reg_write_expr_source_index = pSyntax->reg_expr_source_index;
	return PX_TRUE;
}

px_void PX_Syntax_ResetIRLocation(PX_Syntax* pSyntax)
{
	pSyntax->reg_write_expr_begin_line = -1;
	pSyntax->reg_write_expr_source_index = -1;
}

PX_RETURN_STRING PX_Syntax_BuildIRLocation(PX_Syntax* pSyntax, px_int source_index,px_int line)
{
	PX_RETURN_STRING str = {0};
	PX_sprintf2(str.data, sizeof(str.data), ";@loc %1 %2\n", PX_STRINGFORMAT_INT(source_index), PX_STRINGFORMAT_INT(line));
	return str;
}

px_bool PX_Syntax_NewIRInstructions(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char payload[])
{
	return PX_Syntax_NewIRInstruction3(pSyntax, merge_to_abi, payload, "", "", "");
}

px_bool PX_Syntax_NewIRInstruction0(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[])
{
	return PX_Syntax_NewIRInstruction3(pSyntax, merge_to_abi, opcode, "", "", "");
}

px_bool PX_Syntax_NewIRInstruction1(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[])
{
	return PX_Syntax_NewIRInstruction3(pSyntax, merge_to_abi, opcode, operand1, "","");
}
px_bool PX_Syntax_NewIRInstruction2(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[], const px_char operand2[])
{
	return PX_Syntax_NewIRInstruction3(pSyntax, merge_to_abi, opcode, operand1, operand2, "");
}

px_bool PX_Syntax_NewIRInstructionFormat0(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[])
{
	return PX_Syntax_NewIRInstructions(pSyntax, merge_to_abi, format);
}
px_bool PX_Syntax_NewIRInstructionFormat1(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[], px_stringformat fm1)
{
	px_string fmstr;
	if (!PX_StringInitialize(pSyntax->mp,&fmstr))
	{
		return PX_FALSE;
	}
	if (!PX_StringFormat1(&fmstr, format, fm1))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax,merge_to_abi,PX_StringGetText(&fmstr)))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	PX_StringFree(&fmstr);
	return PX_TRUE;
}

px_bool PX_Syntax_NewIRInstructionFormat2(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[], px_stringformat fm1, px_stringformat fm2)
{
	px_string fmstr;
	if (!PX_StringInitialize(pSyntax->mp, &fmstr))
	{
		return PX_FALSE;
	}
	if (!PX_StringFormat2(&fmstr, format, fm1, fm2))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, merge_to_abi, PX_StringGetText(&fmstr)))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	PX_StringFree(&fmstr);
	return PX_TRUE;
}

px_bool PX_Syntax_NewIRInstructionFormat3(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[], px_stringformat fm1, px_stringformat fm2, px_stringformat fm3)
{
	px_string fmstr;
	if (!PX_StringInitialize(pSyntax->mp, &fmstr))
	{
		return PX_FALSE;
	}
	if (!PX_StringFormat3(&fmstr, format, fm1, fm2, fm3))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	if (!PX_Syntax_NewIRInstructions(pSyntax, merge_to_abi, PX_StringGetText(&fmstr)))
	{
		PX_StringFree(&fmstr);
		return PX_FALSE;
	}
	PX_StringFree(&fmstr);
	return PX_TRUE;

}


px_void PX_Syntax_ResetSource(PX_Syntax* pSyntax)
{
	px_int i;
	px_abi* pabi;
	PX_VectorClear(&pSyntax->reg_ast_stack);
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		PX_AbiFree(pabi);
	}
	PX_VectorClear(&pSyntax->reg_abi_stack);

	PX_StringClear(&pSyntax->message);
	pSyntax->reg_unname_index = 0;
	PX_memset(pSyntax->reg_unname, 0, sizeof(pSyntax->reg_unname));
	pSyntax->reg_lastsource_index = -1;
	PX_SyntaxLexer_Reset(&pSyntax->reg_syntaxlexer);
}


px_void PX_Syntax_ClearState(PX_Syntax* pSyntax)
{
	px_int i;
	px_abi* pabi;
	px_abi* popcode;

	PX_VectorClear(&pSyntax->reg_ast_stack);
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		PX_AbiFree(pabi);
	}
	PX_VectorClear(&pSyntax->reg_abi_stack);

	for (i = 0; i < pSyntax->reg_ast_instr_stack.size; i++)
	{
		popcode = PX_VECTORAT(px_abi, &pSyntax->reg_ast_instr_stack, i);
		PX_AbiFree(popcode);
	}
	PX_VectorClear(&pSyntax->reg_ast_instr_stack);
	PX_VectorClear(&pSyntax->reg_expr_opcode_stack);
	PX_SyntaxLexer_Reset(&pSyntax->reg_syntaxlexer);

	for (i = 0; i < pSyntax->reg_maptype_stack.size; i++)
	{
		px_int j;
		PX_Syntax_maptype* pmaptype = PX_VECTORAT(PX_Syntax_maptype, &pSyntax->reg_maptype_stack, i);
		for (j = 0; j < pmaptype->members.size; j++)
		{
			px_string* pstr = PX_VECTORAT(px_string, &pmaptype->members, j);
			PX_StringFree(pstr);
		}
		PX_StringFree(&pmaptype->name);
	}
	PX_VectorClear(&pSyntax->reg_maptype_stack);
	PX_StringClear(&pSyntax->message);

	pSyntax->reg_unname_index = 0;
	PX_memset(pSyntax->reg_unname, 0, sizeof(pSyntax->reg_unname));
	pSyntax->reg_lastsource_index = -1;

}

px_void PX_Syntax_ClearSourceDescription(PX_Syntax* pSyntax)
{
	px_int i, j;
	
	for (i = 0; i < pSyntax->reg_syntaxlexer.sources.size; i++)
	{
		PX_SyntaxLexer_Source* psource = PX_VECTORAT(PX_SyntaxLexer_Source, &pSyntax->reg_syntaxlexer.sources, i);
		PX_MapClear(&psource->descriptor_map);
		for (j = 0; j < psource->descriptor.size; j++)
		{
			px_abi* pabi = PX_VECTORAT(px_abi, &psource->descriptor, j);
			PX_AbiFree(pabi);
		}
		PX_VectorClear(&psource->descriptor);
		psource->last_descriptor_index = -1;
		for (j = 0; j < psource->cells_count; j++)
		{
			psource->cells[j].abi_index = -1;
		}
	}
}

px_int PX_Syntax_GetLifetime(PX_Syntax* pSyntax)
{
	px_int i;
	px_int lifetime = 0;
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		px_abi* pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi,"scope"))
		{
			lifetime++;
		}
	}
	return lifetime;
}

px_void PX_Syntax_ClearSources(PX_Syntax* pSyntax)
{
	PX_SyntaxLexer_Clear(&pSyntax->reg_syntaxlexer);
}

px_color PX_Syntax_GetRandomColor(px_uint32 seed)
{
	px_color color;
	seed = PX_rand_lcg(seed);
	color._argb.r = 128+(px_byte)(seed % 128);
	seed = PX_rand_lcg(seed);
	color._argb.g = 64+(px_byte)(seed % 192);
	seed = PX_rand_lcg(seed);
	color._argb.b = 64+(px_byte)(seed % 192);
	color._argb.a = 255;
	return color;
}

static px_void PX_SyntaxFree_pebnfnode(PX_Syntax* pSyntax, PX_Syntax_bnfnode*pnode)
{
	if (pnode->pnext)
	{
		PX_SyntaxFree_pebnfnode(pSyntax, pnode->pnext);
	}
	if (pnode->pothers)
	{
		PX_SyntaxFree_pebnfnode(pSyntax, pnode->pothers);
	}
	PX_StringFree(&pnode->constant);
	MP_Free(pSyntax->mp, pnode);
}

px_void PX_Syntax_Free(PX_Syntax* pSyntax)
{
	px_int i;
	PX_Syntax_pebnf* ppebnf;
	PX_Syntax_ClearState(pSyntax);
	for (i = 0; i < pSyntax->pebnf.size; i++)
	{
		ppebnf = PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, i);
		if (ppebnf && ppebnf->pbnfnode)
		{
			PX_SyntaxFree_pebnfnode(pSyntax, ppebnf->pbnfnode);
		}
		PX_StringFree(&ppebnf->mnenonic);
	}

	PX_VectorFree(&pSyntax->reg_maptype_stack);
	PX_VectorFree(&pSyntax->pebnf);
	PX_VectorFree(&pSyntax->reg_ast_stack);
	PX_VectorFree(&pSyntax->reg_abi_stack);
	PX_VectorFree(&pSyntax->reg_ast_instr_stack);
	PX_VectorFree(&pSyntax->reg_expr_opcode_stack);
	PX_StringFree(&pSyntax->message);
	PX_StringFree(&pSyntax->bnfnode_return.constant);

	PX_SyntaxLexer_Free(&pSyntax->reg_syntaxlexer);
}

px_bool PX_Syntax_AddSource(PX_Syntax* pSyntax, const px_char name[], const px_char source[])
{
	return PX_SyntaxLexer_AddSource(&pSyntax->reg_syntaxlexer,name, source)!=-1;
}

PX_SyntaxLexer_Source* PX_Syntax_GetSourceByIndex(PX_Syntax* pSyntax, px_int index)
{
	return PX_SyntaxLexer_GetSourceByIndex(&pSyntax->reg_syntaxlexer, index);
}

px_int PX_Syntax_GetSourceCount(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetSourceCount(&pSyntax->reg_syntaxlexer);
}

typedef enum
{
	PX_SYNTAXLEXER_FILTER_NONE = 0,
	PX_SYNTAXLEXER_FILTER_NEWLINE=1,
	PX_SYNTAXLEXER_FILTER_SPACER=2,
}PX_SYNTAXLEXER_FILTER;

static PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexemeEx2(PX_Syntax* pSyntax,px_int filter)
{
	PX_SYNTAXLEXER_LEXEME_TYPE type;
	while (PX_TRUE)
	{
		type = PX_SyntaxLexer_GetNextLexeme(&pSyntax->reg_syntaxlexer);
		if (type == PX_SYNTAXLEXER_LEXEME_TYPE_COMMENT)
			continue;

		if ((filter & PX_SYNTAXLEXER_FILTER_NEWLINE) && type == PX_SYNTAXLEXER_LEXEME_TYPE_NEWLINE)
			continue;

		if ((filter & PX_SYNTAXLEXER_FILTER_SPACER) && type == PX_SYNTAXLEXER_LEXEME_TYPE_SPACER)
			continue;

		else if (type == PX_SYNTAXLEXER_LEXEME_TYPE_END)
		{
				return PX_SYNTAXLEXER_LEXEME_TYPE_END;
		}
		else
			return type;
	}
}

static PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexemeEx(PX_Syntax* pSyntax, px_int filter)
{
	px_int last_entry = -1;
	const px_char _macro_expand_tag[64] = {0};
	while (PX_TRUE)
	{
		return PX_Syntax_GetNextLexemeEx2(pSyntax, filter);
	}

}

px_int PX_Syntax_GetCurrentLexemeBeginSourceIndex(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetLexemeBeginSourceIndex(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexemeEndSourceIndex(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetLexemeEndSourceIndex(&pSyntax->reg_syntaxlexer);
}

const px_char* PX_Syntax_GetCurrentLexerSourceContentPointer(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentSourcePointer(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexerOffset(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentSourceOffset(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexemeBegin(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetLexemeBegin(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexemeEnd(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetLexemeEnd(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexemeLine(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetLexemeLine(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexerIndex(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentSourceIndex(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLexerLine(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentLine(&pSyntax->reg_syntaxlexer);
}

const px_char* PX_Syntax_GetCurrentLexerSourceName(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentSourceName(&pSyntax->reg_syntaxlexer);
}

const px_char* PX_Syntax_GetCurrentLexeme(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentLexeme(&pSyntax->reg_syntaxlexer);
}

const px_char* PX_Syntax_GetCurrentLexerEntrySourceName(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexerGetEntrySourceName(&pSyntax->reg_syntaxlexer);
}

px_char PX_Syntax_PreviewNextChar(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_PreviewNextChar(&pSyntax->reg_syntaxlexer);
}

px_char PX_Syntax_GetNextChar(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetNextChar(&pSyntax->reg_syntaxlexer);
}

px_void PX_Syntax_LexerForward(PX_Syntax* pSyntax, px_int offset)
{
	PX_SyntaxLexer_Forward(&pSyntax->reg_syntaxlexer, offset);
}

PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetNextLexemeEx(pSyntax, PX_SYNTAXLEXER_FILTER_NEWLINE| PX_SYNTAXLEXER_FILTER_SPACER);
}

PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme3(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetNextLexemeEx(pSyntax, PX_SYNTAXLEXER_FILTER_SPACER);
}

PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme4(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetNextLexemeEx(pSyntax, PX_SYNTAXLEXER_FILTER_NONE);
}

PX_SYNTAXLEXER_STATE PX_Syntax_GetLexerState(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetState(&pSyntax->reg_syntaxlexer);
}

px_void PX_Syntax_SetLexerState(PX_Syntax* pSyntax, PX_SYNTAXLEXER_STATE *pstate)
{
	PX_SyntaxLexer_SetState(&pSyntax->reg_syntaxlexer, pstate);
}

px_bool PX_Syntax_GrabeVariablesAbi(PX_Syntax* pSyntax, px_abi* pToAbi,const px_char father_payload[])
{
	px_int i,j;
	px_int counter=0;
	px_int lifetime=1;
	px_char var_content[64] = { 0 };
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		px_abi* pabi = PX_Syntax_GetAbiByIndex(pSyntax, i);
		if (PX_Syntax_CheckAbiName(pabi,"scope"))
		{
			const px_char* ptype;
			px_int member_count;
			px_int offset;
			const px_char* pfrom_string;
			member_count = PX_AbiGet_PayloadMemberCount(pabi, "variables");
			for (j = 0; j < member_count; j++)
			{
				PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].offset",  PX_STRINGFORMAT_INT(i));
				offset = PX_AbiGetValue_int(pabi, var_content);
				PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].type",  PX_STRINGFORMAT_INT(i));
				ptype = PX_AbiGetValue_string(pabi, var_content);
				PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].from", PX_STRINGFORMAT_INT(i));
				pfrom_string = PX_AbiGetValue_string(pabi, var_content);

				if(father_payload&& father_payload[0])
					PX_sprintf2(var_content, sizeof(var_content), "%1.variables.[%2].lifetime", PX_STRINGFORMAT_STRING(father_payload), PX_STRINGFORMAT_INT(counter));
				else
					PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].lifetime", PX_STRINGFORMAT_INT(counter));

				if (!PX_AbiSet_int(pToAbi, var_content, lifetime))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_GrabeVariablesAbi out of memory");
					return PX_FALSE;
				}
				if (father_payload && father_payload[0])
					PX_sprintf2(var_content, sizeof(var_content), "%1.variables.[%2].offset", PX_STRINGFORMAT_STRING(father_payload), PX_STRINGFORMAT_INT(counter));
				else
					PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].offset", PX_STRINGFORMAT_INT(counter));

				if (!PX_AbiSet_int(pToAbi, var_content, offset))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_GrabeVariablesAbi out of memory");
					return PX_FALSE;
				}
				if (father_payload && father_payload[0])
					PX_sprintf2(var_content, sizeof(var_content), "%1.variables.[%2].type", PX_STRINGFORMAT_STRING(father_payload), PX_STRINGFORMAT_INT(counter));
				else
					PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].type", PX_STRINGFORMAT_INT(counter));

				if (!PX_AbiSet_string(pToAbi, var_content, ptype))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_GrabeVariablesAbi out of memory");
					return PX_FALSE;
				}
				if (father_payload && father_payload[0])
					PX_sprintf2(var_content, sizeof(var_content), "%1.variables.[%2].from", PX_STRINGFORMAT_STRING(father_payload), PX_STRINGFORMAT_INT(counter));
				else
					PX_sprintf1(var_content, sizeof(var_content), "variables.[%1].from", PX_STRINGFORMAT_INT(counter));

				if (!PX_AbiSet_string(pToAbi, var_content, pfrom_string))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_GrabeVariablesAbi out of memory");
					return PX_FALSE;
				}
				counter++;
			}
			lifetime++;
		}
	}
	return PX_TRUE;
}

static px_bool PX_Syntax_ExecuteNextAst(PX_Syntax* pSyntax, PX_Syntax_ast* pcurrent_ast)
{
	PX_Syntax_ast newast = { 0 };
	px_char content[128] = { 0 };
	//push next
	if (pcurrent_ast->pebnf_index!=-1)//pebnf
	{
		PX_Syntax_pebnf* ppebnf;
		PX_VECTOR_CHECK_RANGE(&pSyntax->pebnf, pcurrent_ast->pebnf_index);
		ppebnf = PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, pcurrent_ast->pebnf_index);
		PX_ASSERTIFX(!ppebnf, "ppebnf should not be null");
		if (!ppebnf->pbnfnode)
		{
			PX_sprintf1(content, sizeof(content), "ast:parser:*%1->node is null,return true\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&ppebnf->mnenonic)));
			PX_Syntax_Message(pSyntax, content);
			if (PX_Syntax_ExecuteAstOpcode(pSyntax, "return true"))
				return PX_TRUE;
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteNextAst out of memory");
			return PX_FALSE;
		}
		else
		{
			newast.pebnf_index = -1;//pebnf node
			newast.pbnfnode = ppebnf->pbnfnode;//try next
			newast.call_abistack_count = pSyntax->reg_abi_stack.size;
			newast.syntaxlexer_state = PX_SyntaxLexer_GetState(&pSyntax->reg_syntaxlexer);
			if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &newast))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteNextAst out of memory");
				return PX_FALSE;
			}
			PX_sprintf1(content, sizeof(content), "ast:parser:enter bnfnode->%1\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&ppebnf->pbnfnode->constant)));
			PX_Syntax_Message(pSyntax, content);
			return PX_TRUE;
		}
	}
	else
	{
		PX_ASSERTIFX(!pcurrent_ast->pbnfnode, "pbnfnode should not be null");
		if (pcurrent_ast->pbnfnode->pnext == PX_NULL)
		{
			PX_Syntax_Message(pSyntax, "ast:parser:next node is null,return true\n");
			if (PX_Syntax_ExecuteAstOpcode(pSyntax, "return true"))
				return PX_TRUE;
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteNextAst out of memory");
			return PX_FALSE;
		}
		else
		{
			PX_Syntax_bnfnode* pnext = pcurrent_ast->pbnfnode->pnext;
			if (PX_strequ(PX_StringGetText(&pnext->constant), "\n"))
			{
				PX_sprintf0(content, sizeof(content), "ast:parser:next node newline\n");
			}
			else
			{
				PX_sprintf1(content, sizeof(content), "ast:parser:next node %1\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&pnext->constant)));
			}
			PX_Syntax_Message(pSyntax, content);
			
			newast.pebnf_index = -1;
			newast.pbnfnode = pnext;//try next
			newast.call_abistack_count = pSyntax->reg_abi_stack.size;
			newast.syntaxlexer_state = PX_SyntaxLexer_GetState(&pSyntax->reg_syntaxlexer);

			if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &newast))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteNextAst out of memory2");
				return PX_FALSE;
			}
			return PX_TRUE;
		}
	}
}

px_void PX_Syntax_AstClearAbiStack(PX_Syntax* pSyntax, PX_Syntax_ast* past)
{
	while (pSyntax->reg_abi_stack.size > past->call_abistack_count)
	{
		PX_Syntax_PopAbi(pSyntax);
	}
}

static px_bool PX_Syntax_ExecuteOthers(PX_Syntax* pSyntax, PX_Syntax_ast* pcurrent_ast)
{
	px_char content_message[128] = { 0 };
	PX_Syntax_ast newast = { 0 };
	PX_Syntax_AstClearAbiStack(pSyntax, pcurrent_ast);

	if (pcurrent_ast->pebnf_index!=-1)
	{
		PX_ASSERTX("pebnf_index should be -1");
		return PX_FALSE;
	}
	else
	{
		PX_Syntax_bnfnode* pbnfnode = pcurrent_ast->pbnfnode;
		PX_ASSERTIFX(!pbnfnode, "pbnfnode should not be null");
		if (pbnfnode->pothers == PX_NULL)
		{
			PX_sprintf0(content_message, sizeof(content_message), "ast:parser:others node is null,return false\n");
			PX_Syntax_Message(pSyntax, content_message);

			if (PX_Syntax_ExecuteAstOpcode(pSyntax, "return false"))
				return PX_TRUE;

			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOthers out of memory");
			return PX_FALSE;
		}
		else
		{
			PX_Syntax_bnfnode* pother = pcurrent_ast->pbnfnode->pothers;
			PX_sprintf1(content_message, sizeof(content_message), "ast:parser:others node %1\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&pother->constant)));
			PX_Syntax_Message(pSyntax, content_message);
			newast.pebnf_index = -1;
			newast.pbnfnode = pother;
			newast.syntaxlexer_state = pcurrent_ast->syntaxlexer_state;
			newast.call_abistack_count = pSyntax->reg_abi_stack.size;
			if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &newast))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ExecuteOthers out of memory1");
				return PX_FALSE;
			}
		}

		return PX_TRUE;
	}
}


px_bool PX_Syntax_CheckAbiName(px_abi* pabi, const px_char name[])
{
	if (pabi)
	{
		const px_char* pname = PX_AbiGet_string(pabi, "name");
		if (!pname)
		{
			return PX_FALSE;
		}
		return PX_strequ(pname, name);
	}
	return PX_FALSE;
}

px_bool PX_Syntax_CheckLastAbiName(PX_Syntax* pSyntax, const px_char name[])
{
	px_abi* pabi = PX_Syntax_GetLastAbi(pSyntax);
	if (pabi)
	{
		return PX_Syntax_CheckAbiName(pabi, name);
	}
	return PX_FALSE;

}

px_bool PX_Syntax_CheckSecondLastAbiName(PX_Syntax* pSyntax, const px_char name[])
{
	px_abi* pabi = PX_Syntax_GetSecondLastAbi(pSyntax);
	if (pabi)
	{
		return PX_Syntax_CheckAbiName(pabi, name);
	}
	return PX_FALSE;

}

px_int PX_Syntax_GetAbiIndexFromForward(PX_Syntax* pSyntax, const px_char name[])
{
	px_int i;
	px_abi* pabi;
	for (i = 0; i < pSyntax->reg_abi_stack.size; i++)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, name))
		{
			return i;
		}
	}
	return -1;

}

px_int PX_Syntax_GetAbiIndexFromBackward(PX_Syntax* pSyntax, const px_char name[])
{
	px_int i;
	px_abi* pabi;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, name))
		{
			return i;
		}
	}
	return -1;
	
}

px_int PX_Syntax_GetSecondAbiIndexFromBackward(PX_Syntax* pSyntax, const px_char name[])
{
	px_int i;
	px_int count = 0;
	px_abi* pabi;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (PX_Syntax_CheckAbiName(pabi, name))
		{
			if (count == 1)
			{
				return i;
			}
			count++;
		}
	}
	return -1;
	
}

px_abi * PX_Syntax_GetAbiFromBackward(PX_Syntax* pSyntax, const px_char name[])
{
	px_int index = PX_Syntax_GetAbiIndexFromBackward(pSyntax, name);
	if (index != -1)
	{
		return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
	}
	return PX_NULL;
}


px_abi* PX_Syntax_GetAbiFromForward(PX_Syntax* pSyntax, const px_char name[])
{
	px_int index = PX_Syntax_GetAbiIndexFromForward(pSyntax, name);
	if (index != -1)
	{
		return PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, index);
	}
	return PX_NULL;
}

px_bool PX_Syntax_IsTerminated(PX_Syntax* pSyntax)
{
	px_int i;
	for (i = 0; i < pSyntax->reg_ast_instr_stack.size; i++)
	{
		px_abi *popcode = PX_VECTORAT(px_abi, &pSyntax->reg_ast_instr_stack, i);
		const px_char* ptype = PX_AbiGetValue_string(popcode, "type");
		if (PX_strequ(ptype, "terminate"))
			return PX_TRUE;
	}
	return PX_FALSE;
}

static px_bool PX_Syntax_ExecuteCurrentNode(PX_Syntax* pSyntax)
{
	PX_Syntax_ast current_ast = { 0 };
	PX_Syntax_bnfnode* pbnfnode = PX_NULL;

	if (pSyntax->reg_ast_stack.size == 0)
	{
		if (PX_Syntax_IsEndOfSource(pSyntax))
		{
			return PX_TRUE;
		}
		//end of execute
		return PX_FALSE;
	}

	if (!PX_VectorLastTo(&pSyntax->reg_ast_stack, &current_ast))
	{
		return PX_FALSE;
	}

	PX_SyntaxLexer_SetState(&pSyntax->reg_syntaxlexer, &current_ast.syntaxlexer_state);

	if (current_ast.pebnf_index != -1)//<-----------------------------pebnf root
	{
		PX_Syntax_pebnf* ppebnf = PX_Syntax_GetPebnfByIndex(pSyntax, current_ast.pebnf_index);
		if (ppebnf->penterfunction)
		{
			px_char message_content[128] = { 0 };
			const px_char* pmnemonic = PX_StringGetText(&ppebnf->mnenonic);
			if (!ppebnf->penterfunction(pSyntax, &current_ast, ppebnf->userptr))
			{
				if (PX_Syntax_IsTerminated(pSyntax))
					return PX_TRUE;
				PX_sprintf1(message_content, sizeof(message_content), "ast:parser:%1 function return false\n", PX_STRINGFORMAT_STRING(pmnemonic));
				PX_Syntax_Message(pSyntax, message_content);
				return (PX_Syntax_ExecuteAstOpcode(pSyntax, "return false"));
			}
			PX_sprintf1(message_content, sizeof(message_content), "ast:parser:%1 function return true\n", PX_STRINGFORMAT_STRING(pmnemonic));
			PX_Syntax_Message(pSyntax, message_content);
			if (PX_Syntax_IsTerminated(pSyntax))
				return PX_TRUE;
		}
		return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
	}
	else//<-----------------------------bnfnode
	{
		pbnfnode = current_ast.pbnfnode;
		PX_ASSERTIFX(!pbnfnode, "Error:Current AST node is null");
		if (pbnfnode->penterfunction)
		{
			px_char message_content[128];
			if (!pbnfnode->penterfunction(pSyntax, &current_ast, pbnfnode->userptr))
			{
				PX_sprintf0(message_content, sizeof(message_content), "ast:parser:return false\n");
				PX_Syntax_Message(pSyntax, message_content);
				if (PX_Syntax_IsTerminated(pSyntax))
					return PX_TRUE;
				PX_VectorPop(&pSyntax->reg_ast_stack);
				return PX_Syntax_ExecuteOthers(pSyntax, &current_ast);
			}
			PX_sprintf0(message_content, sizeof(message_content), "ast:parser:return true\n");
			PX_Syntax_Message(pSyntax, message_content);
			if (PX_Syntax_IsTerminated(pSyntax))
				return PX_TRUE;
		}

		switch (pbnfnode->type)
		{
		case PX_SYNTAX_AST_TYPE_CONSTANT:
		{
			const px_char* pstr = PX_StringGetText(&pbnfnode->constant);
			PX_SYNTAXLEXER_LEXEME_TYPE type;
			const px_char* plexeme;
			const px_char* pcontent1, * pcontent2;
			px_char message_content[128] = { 0 };
			if (pstr[1] == '\0' && (pstr[0] == '\r' || pstr[0] == '\n'))
			{
				type = PX_Syntax_GetNextLexeme3(pSyntax);
				plexeme = PX_Syntax_GetCurrentLexeme(pSyntax);

				if (PX_strequ(pstr, "\n"))
					pcontent1 = "newline";
				else if (PX_strequ(pstr, "\r"))
					pcontent1 = "carriage_return";
				else
					pcontent1 = pstr;

				if (PX_strequ(plexeme, "\n"))
					pcontent2 = "newline";
				else
					pcontent2 = plexeme;

				PX_sprintf2(message_content, sizeof(message_content), "ast:parser:compare constant \"%1\" with \"%2\"\n", PX_STRINGFORMAT_STRING(pcontent1), PX_STRINGFORMAT_STRING(pcontent2));
			}
			else
			{
				type = PX_Syntax_GetNextLexeme(pSyntax);
				plexeme = PX_Syntax_GetCurrentLexeme(pSyntax);
				if (PX_strequ(pstr, "\n"))
					pcontent1 = "newline";
				else if (PX_strequ(pstr, "\r"))
					pcontent1 = "carriage_return";
				else
					pcontent1 = pstr;
				PX_sprintf2(message_content, sizeof(message_content), "ast:parser:compare constant \"%1\" with \"%2\"\n", PX_STRINGFORMAT_STRING(pcontent1), PX_STRINGFORMAT_STRING(plexeme));
			}
			PX_Syntax_Message(pSyntax, message_content);
			PX_VectorPop(&pSyntax->reg_ast_stack);

			if (PX_strequ(plexeme, pstr))
			{
				if (pstr[1] == '\0' && (pstr[0] == '\r' || pstr[0] == '\n'))
				{
					PX_Syntax_Message(pSyntax, "ast:parser:const matched newline\n");
				}
				else
				{
					PX_sprintf1(message_content, sizeof(message_content), "ast:parser:const matched %1\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&current_ast.pbnfnode->constant)));
					PX_Syntax_Message(pSyntax, message_content);
				}

				if (pbnfnode->pleavefunction)
				{
					if (pbnfnode->pleavefunction(pSyntax, &current_ast, pbnfnode->userptr))
					{
						if (PX_Syntax_IsTerminated(pSyntax))
							return PX_TRUE;
						PX_Syntax_Message(pSyntax, "ast:parser:const function return true,ast next\n");
						return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
					}
					else
					{
						if (PX_Syntax_IsTerminated(pSyntax))
							return PX_TRUE;
						PX_Syntax_Message(pSyntax, "ast:parser:const function return false,try others\n");
					}
				}
				else
				{
					PX_Syntax_Message(pSyntax, "ast:parser:const function is null,ast next\n");
					return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
				}
			}
			return PX_Syntax_ExecuteOthers(pSyntax, &current_ast);
		}
		break;
		case PX_SYNTAX_AST_TYPE_FUNCTION:
		{
			PX_VectorPop(&pSyntax->reg_ast_stack);
			if (pbnfnode->pleavefunction)
			{
				if (pbnfnode->pleavefunction(pSyntax, &current_ast, pbnfnode->userptr))
				{
					if (PX_Syntax_IsTerminated(pSyntax))
						return PX_TRUE;
					PX_Syntax_Message(pSyntax, "ast:parser:function return true,ast next\n");
					return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
				}
				else
				{
					if (PX_Syntax_IsTerminated(pSyntax))
						return PX_TRUE;
					PX_Syntax_Message(pSyntax, "ast:parser:function return false,try others\n");
				}
			}
			else
			{
				PX_Syntax_Message(pSyntax, "ast:parser:function is null,ast next\n");
				return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
			}
			return PX_Syntax_ExecuteOthers(pSyntax, &current_ast);
		}
		break;
		case PX_SYNTAX_AST_TYPE_CONTINUOUS:
		{
			px_char ch;
			PX_VectorPop(&pSyntax->reg_ast_stack);
			ch = PX_Syntax_PreviewNextChar(pSyntax);
			if (ch == ' ' || ch == '\t' || ch == '\0' || ch == '\n' || ch == '\r')
			{
				PX_Syntax_Message(pSyntax, "ast:parser:continuous not matched.\n");
				return PX_Syntax_ExecuteOthers(pSyntax, &current_ast);
			}
			PX_Syntax_Message(pSyntax, "ast:parser:continuous matched\n");

			return PX_Syntax_ExecuteNextAst(pSyntax, &current_ast);
		}
		break;
		case PX_SYNTAX_AST_TYPE_LOOP:
			PX_VectorPop(&pSyntax->reg_ast_stack);
		case PX_SYNTAX_AST_TYPE_RECURSION:
		{
			PX_Syntax_ast newast = { 0 };
			const px_char* pstr = PX_StringGetText(&pbnfnode->constant);
			px_int pebnf_index = PX_Syntax_GetPebnfIndexByMnemonic(pSyntax, pstr);
			PX_Syntax_pebnf* ppebnf = PX_Syntax_GetPebnfByIndex(pSyntax, pebnf_index);
			PX_ASSERTIFX(!ppebnf, "Error:unknown pebnf");
			newast.pbnfnode = ppebnf->pbnfnode;
			newast.pebnf_index = -1;
			PX_ASSERTIFX(!newast.pbnfnode, "Error:unknown pebnf->node error");
			newast.syntaxlexer_state = PX_SyntaxLexer_GetState(&pSyntax->reg_syntaxlexer);
			newast.call_abistack_count = pSyntax->reg_abi_stack.size;
			if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &newast))
			{
				return PX_FALSE;
			}
			return PX_TRUE;
		}
		break;
		case PX_SYNTAX_AST_TYPE_LINKER:
		{
			//call linker next
			px_int pebnf_index;
			PX_Syntax_ast newast = { 0 };
			px_char content[128];
			const px_char* pstr = PX_StringGetText(&pbnfnode->constant);
			pebnf_index= PX_Syntax_GetPebnfIndexByMnemonic(pSyntax, pstr);
			if (pebnf_index==-1)
			{
				PX_ASSERTX("Error:linker not found");
				return PX_FALSE;
			}
			PX_sprintf2(content, sizeof(content), "ast:parser:call linker %1 abistack:%2\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&current_ast.pbnfnode->constant)), PX_STRINGFORMAT_INT(pSyntax->reg_abi_stack.size));
			PX_Syntax_Message(pSyntax, content);
			newast.syntaxlexer_state = PX_SyntaxLexer_GetState(&pSyntax->reg_syntaxlexer);
			newast.pbnfnode = PX_NULL;
			newast.call_abistack_count = pSyntax->reg_abi_stack.size;
			newast.pebnf_index = pebnf_index;
			
			if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &newast))
			{
				return PX_FALSE;
			}
			return PX_TRUE;
		}
		break;
		default:
			PX_ASSERT();
			break;
		}
		return PX_FALSE;
	}

}

px_bool PX_Syntax_RemovePebnf(PX_Syntax* pSyntax, const px_char pebnf[])
{
	px_int i;
	for (i = 0; i < pSyntax->pebnf.size; i++)
	{
		PX_Syntax_pebnf* ppebnf = PX_VECTORAT(PX_Syntax_pebnf, &pSyntax->pebnf, i);
		if (PX_strequ(PX_StringGetText(&ppebnf->mnenonic), pebnf))
		{
			PX_StringFree(&ppebnf->mnenonic);
			PX_SyntaxFree_pebnfnode(pSyntax, ppebnf->pbnfnode);
			PX_VectorErase(&pSyntax->pebnf, i);
			return PX_TRUE;
		}
	}
	return PX_FALSE;
}

const px_char* PX_Syntax_AllocUnnamed(PX_Syntax* pSyntax, const px_char* prefix)
{
	if (prefix)
		PX_strset(pSyntax->reg_unname, prefix);
	else
		PX_strset(pSyntax->reg_unname, "_unnamed_");

	PX_strcat(pSyntax->reg_unname, PX_itos(pSyntax->reg_unname_index, 10).data);
	pSyntax->reg_unname_index++;
	return pSyntax->reg_unname;
}


px_abi* PX_Syntax_PushOperand(PX_Syntax* pSyntax, const px_char type[], px_int type_size, PX_SYNTAX_OPERAND_FROM datafrom)
{
	px_abi* pnewabi = PX_Syntax_NewAbi(pSyntax, "operand");
	if (type[0] == '\0')
	{
		PX_DEBUG_BREAK();
	}
	if (!pnewabi)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_PushOperand out of memory1");
		return PX_NULL;
	}
	if (!PX_AbiSet_string(pnewabi, "type", type) || \
		!PX_AbiSet_int(pnewabi, "type_size", type_size) || \
		!PX_AbiSet_int(pnewabi, "from", (px_int)datafrom)
		)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_PushOperand out of memory2");
		PX_Syntax_PopAbi(pSyntax);
		return PX_NULL;
	}
	return pnewabi;
}



static px_bool PX_Syntax_ScopeOperandToFunction(PX_Syntax* pSyntax, px_int scope_index, const px_char pirabi_name[], px_int operand_abi_index, px_int register_index, const px_char function_name[], px_bool* pfunction_return)
{
	px_abi* pscope;
	px_abi* poperand_abi;
	const px_char* poperand_type;
	px_string payload, from_type_string;
	px_bool matched = PX_FALSE;

	*pfunction_return = PX_FALSE;
	if (scope_index < 0 || scope_index >= pSyntax->reg_abi_stack.size)
	{
		PX_ASSERTX("PX_Syntax_ScopeOperandToFunction invalid scope index");
		return PX_FALSE;
	}
	pscope = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, scope_index);
	PX_ASSERTIFX(!PX_Syntax_CheckAbiName(pscope, "scope"), "PX_Syntax_ScopeOperandToFunction scope_index is not a scope abi");
	if (!PX_Syntax_CheckAbiName(pscope, "scope"))
		return PX_FALSE;

	poperand_abi = PX_Syntax_GetAbiByIndex(pSyntax, operand_abi_index);
	PX_ASSERTIFX(!poperand_abi || PX_Syntax_CheckAbiName(poperand_abi, "operand") == PX_FALSE, "Error:abi is not operand");
	poperand_type = PX_AbiGetValue_string(poperand_abi, "type");

	if (!PX_StringInitialize(pSyntax->mp, &payload))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ScopeOperandToFunction out of memory");
		return PX_FALSE;
	}
	if (!PX_StringInitialize(pSyntax->mp, &from_type_string))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ScopeOperandToFunction out of memory");
		PX_StringFree(&payload);
		return PX_FALSE;
	}
	if (!PX_StringSet(&from_type_string, poperand_type))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ScopeOperandToFunction out of memory");
		goto _END;
	}

	while (PX_StringLen(&from_type_string) > 0)
	{
		if (!PX_StringFormat2(&payload, "type_defines.%1.%2", PX_STRINGFORMAT_STRING(PX_StringGetText(&from_type_string)), PX_STRINGFORMAT_STRING(function_name)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ScopeOperandToFunction out of memory");
			goto _END;
		}
		if (!PX_AbiExist_Type(pscope, PX_StringGetText(&payload), PX_ABI_TYPE_PTR))
		{
			PX_StringTrimBackwardUntil(&from_type_string, '.');
			continue;
		}

		matched = PX_TRUE;
		if (PX_strequ(function_name,"register_function"))
		{
			PX_Syntax_RegisterFunction pregisterfunction = (PX_Syntax_RegisterFunction)PX_AbiGetValue_ptr(pscope, PX_StringGetText(&payload));
			*pfunction_return = pregisterfunction(pSyntax, pirabi_name, operand_abi_index, register_index);
		}
		else if (PX_strequ(function_name, "memory_function"))
		{
			PX_Syntax_MemoryFunction pmemoryfunction = (PX_Syntax_MemoryFunction)PX_AbiGetValue_ptr(pscope, PX_StringGetText(&payload));
			*pfunction_return = pmemoryfunction(pSyntax, pirabi_name, operand_abi_index, register_index);
		}
		else
		{
			PX_ASSERTX("Error:unknown function name");
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ScopeOperandToFunction unknown function name");
			*pfunction_return = PX_FALSE;
		}
		goto _END;
	}
_END:
	PX_StringFree(&from_type_string);
	PX_StringFree(&payload);
	return matched;
}

px_bool PX_Syntax_OperandToFunction(PX_Syntax* pSyntax,const px_char pirabi_name[], px_int operand_abi_index, px_int register_index,const px_char function_name[])
{
	px_abi* pabi;
	px_int i;
	for (i = pSyntax->reg_abi_stack.size - 1; i >= 0; i--)
	{
		px_bool function_return = PX_FALSE;
		pabi = PX_VECTORAT(px_abi, &pSyntax->reg_abi_stack, i);
		if (!PX_Syntax_CheckAbiName(pabi, "scope"))
			continue;

		if (PX_Syntax_ScopeOperandToFunction(pSyntax, i, pirabi_name, operand_abi_index, register_index, function_name, &function_return))
		{
			//the matched function has been called,it has reported the error itself when it failed
			if (function_return == PX_TRUE)
				return PX_TRUE;
			return PX_FALSE;
		}
	}
	PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_OperandToFunction function not found");
	return PX_FALSE;
}

px_bool PX_Syntax_OperandToRegister(PX_Syntax* pSyntax, const px_char pirabi_name[], px_int operand_abi_index, px_int register_index)
{
	return PX_Syntax_OperandToFunction(pSyntax, pirabi_name, operand_abi_index, register_index, "register_function");
}
px_bool PX_Syntax_OperandToMemory(PX_Syntax* pSyntax, const px_char pirabi_name[], px_int operand_abi_index, px_int register_index)
{
	return PX_Syntax_OperandToFunction(pSyntax, pirabi_name, operand_abi_index, register_index, "memory_function");
}

px_void PX_Syntax_PopInstrStack(PX_Syntax* pSyntax)
{
	px_abi* pabi = PX_VECTORLAST(px_abi, &pSyntax->reg_ast_instr_stack);
	if (pabi)
	{
		PX_AbiFree(pabi);
		PX_VectorPop(&pSyntax->reg_ast_instr_stack);
	}
}

px_void PX_Syntax_MakeBreakPoint(PX_Syntax* pSyntax, px_int break_execute_id)
{
	pSyntax->break_execute_id = break_execute_id;
}

PX_SYNTAX_AST_RETURN PX_Syntax_ExecuteNext(PX_Syntax* pSyntax)
{
	px_char content[128] = { 0 };
	pSyntax->execute_id++;
	if (pSyntax->reg_ast_instr_stack.size)
	{
		px_abi* pabi = PX_VECTORLAST(px_abi, &pSyntax->reg_ast_instr_stack);
		const px_char* ptype = PX_AbiGet_string(pabi, "type");
		if (ptype)
		{
			if (PX_strequ(ptype, "terminate"))
			{
				PX_Syntax_Message(pSyntax, "ast:parser:compiler terminated.\n");
				return PX_SYNTAX_AST_RETURN_ERROR;
			}
			else if (PX_strequ(ptype, "return true") || PX_strequ(ptype, "return false"))
			{
				//return node
				PX_Syntax_ast current_ast = { 0 };
				px_bool is_return_true = PX_strequ(ptype, "return true");
				PX_Syntax_PopInstrStack(pSyntax);

				if (pSyntax->reg_ast_stack.size == 0)
				{	
					return is_return_true ? PX_SYNTAX_AST_RETURN_END : PX_SYNTAX_AST_RETURN_ERROR;
				}

				PX_VectorPopTo(&pSyntax->reg_ast_stack, &current_ast);
				
				if (is_return_true)
				{
					if (current_ast.pebnf_index != -1)
					{
						PX_Syntax_pebnf* ppebnf;
						PX_ASSERTIFX(current_ast.pebnf_index == -1, "Error:pebnf index should not be -1");
						ppebnf = PX_Syntax_GetPebnfByIndex(pSyntax, current_ast.pebnf_index);
						PX_ASSERTIFX(!ppebnf, "Error:pebnf node should not be null");
						if (ppebnf->pleavefunction)
						{
							if (!ppebnf->pleavefunction(pSyntax, &current_ast, ppebnf->userptr))
							{
								if (PX_Syntax_IsTerminated(pSyntax))
									return PX_SYNTAX_AST_RETURN_ERROR;

								if (!PX_Syntax_ExecuteAstOpcode(pSyntax, "return false"))
								{
									return PX_SYNTAX_AST_RETURN_ERROR;
								}
								return PX_SYNTAX_AST_RETURN_CONTINUE;
							}
						}
						PX_sprintf1(content, sizeof(content), "ast:parser:*%1 return true\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&ppebnf->mnenonic)));
						PX_Syntax_Message(pSyntax, content);
						if (!PX_Syntax_ExecuteAstOpcode(pSyntax, "return true"))
							return PX_SYNTAX_AST_RETURN_ERROR;
					}
					else
					{
						PX_Syntax_bnfnode* pbnfnode = current_ast.pbnfnode;
						PX_ASSERTIFX(pbnfnode == PX_NULL, "Error:pebnf node should not be null");
						if (pbnfnode->pleavefunction)
						{
							if (!pbnfnode->pleavefunction(pSyntax, &current_ast, pbnfnode->userptr))
							{
								if (PX_Syntax_IsTerminated(pSyntax))
									return PX_SYNTAX_AST_RETURN_ERROR;
								PX_sprintf1(content, sizeof(content), "ast:parser:%1 leave function return false,try others\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&pbnfnode->constant)));
								PX_Syntax_Message(pSyntax, content);
								if (!PX_Syntax_ExecuteOthers(pSyntax, &current_ast))
								{
									return PX_SYNTAX_AST_RETURN_ERROR;
								}
								return PX_SYNTAX_AST_RETURN_CONTINUE;
							}
						}
						if (!PX_Syntax_ExecuteNextAst(pSyntax, &current_ast))
							return PX_SYNTAX_AST_RETURN_ERROR;
					}
				}
				else//"return false"
				{
					if (current_ast.pebnf_index!=-1)
					{
						PX_Syntax_pebnf* ppebnf;
						PX_ASSERTIFX(current_ast.pebnf_index == -1, "Error:pebnf index should not be -1");
						ppebnf = PX_Syntax_GetPebnfByIndex(pSyntax, current_ast.pebnf_index);
						PX_sprintf1(content, sizeof(content), "ast:parser:*%1 return false\n", PX_STRINGFORMAT_STRING(PX_StringGetText(&ppebnf->mnenonic)));
						PX_Syntax_Message(pSyntax, content);
						if (!PX_Syntax_ExecuteAstOpcode(pSyntax, "return false"))
							return PX_SYNTAX_AST_RETURN_ERROR;
					}
					else
					{
						if (!PX_Syntax_ExecuteOthers(pSyntax, &current_ast))//<-------try others
						{
							return PX_SYNTAX_AST_RETURN_ERROR;
						}
					}
					
				}
			}
		}
	}
	else
	{
		if (!PX_Syntax_ExecuteCurrentNode(pSyntax))
		{
			return PX_SYNTAX_AST_RETURN_ERROR;
		}
	}
	return PX_SYNTAX_AST_RETURN_CONTINUE;
}

px_int PX_Syntax_GetCurrentSourceIndex(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentSourceIndex(&pSyntax->reg_syntaxlexer); 
}

px_int PX_Syntax_GetCurrentRow(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_GetCurrentLine(&pSyntax->reg_syntaxlexer);
}

px_int PX_Syntax_GetCurrentLine(PX_Syntax* pSyntax)
{
	return PX_Syntax_GetCurrentRow(pSyntax);
}

px_bool PX_Syntax_IsExecuting(PX_Syntax* pSyntax)
{
	return PX_SyntaxLexer_IsEnd(&pSyntax->reg_syntaxlexer) && (pSyntax->reg_ast_stack.size == 0) ? PX_FALSE : PX_TRUE;
}



px_bool PX_Syntax_CallSource(PX_Syntax* pSyntax, const px_char name[], const px_char pebnf[])
{
	if (!PX_SyntaxLexer_CallSourceByName(&pSyntax->reg_syntaxlexer,name))
		return PX_FALSE;

	if (pebnf && pebnf[0])
	{
		PX_Syntax_ast init_ast = { 0 };
		//push ast
		init_ast.syntaxlexer_state = PX_Syntax_GetLexerState(pSyntax);
		init_ast.call_abistack_count = pSyntax->reg_abi_stack.size;
		init_ast.pbnfnode = PX_Syntax_GetPebnfNode(pSyntax, pebnf);
		init_ast.pebnf_index = PX_Syntax_GetPebnfIndexByMnemonic(pSyntax, pebnf);


		PX_ASSERTIFX(!init_ast.pbnfnode, "Error:pebnf node not found");
		PX_ASSERTIFX(init_ast.pebnf_index==-1, "Error:pebnf not found");

		if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &init_ast))
		{
			return PX_FALSE;
		}
	}
	return PX_TRUE;
}
px_bool PX_Syntax_CallSourceIndex(PX_Syntax* pSyntax, px_int index, const px_char pebnf[])
{ 
	if (!PX_SyntaxLexer_CallSourceByIndex(&pSyntax->reg_syntaxlexer, index))
		return PX_FALSE;

	if (pebnf && pebnf[0])
	{
		PX_Syntax_ast init_ast = { 0 };
		//push ast
		init_ast.syntaxlexer_state = PX_Syntax_GetLexerState(pSyntax);
		init_ast.call_abistack_count = pSyntax->reg_abi_stack.size;
		init_ast.pebnf_index = PX_Syntax_GetPebnfIndexByMnemonic(pSyntax, pebnf);

		PX_ASSERTIFX(init_ast.pebnf_index==-1, "Error:pebnf not found");

		if (!PX_VectorPushback(&pSyntax->reg_ast_stack, &init_ast))
		{
			PX_TERMINATE("Out of memory");
			return PX_FALSE;
		}
	}
	return PX_TRUE;
}

px_bool PX_Syntax_Execute(PX_Syntax* pSyntax, const px_char filename[], const px_char pebnf[])
{
	PX_Syntax_ClearState(pSyntax);
	if (!PX_Syntax_CallSource(pSyntax, filename, pebnf))
		return PX_FALSE;
	if (!PX_Syntax_EnterScope(pSyntax))//scope 1--->global scope
	{
		PX_Syntax_Message(pSyntax, "Error:sources_enter scope failed.");
		return PX_FALSE;
	}

	while (PX_TRUE)
	{
		PX_SYNTAX_AST_RETURN ast_return;
		ast_return = PX_Syntax_ExecuteNext(pSyntax);
		if (ast_return == PX_SYNTAX_AST_RETURN_ERROR)
		{
			return PX_FALSE;
		}
		else if (ast_return == PX_SYNTAX_AST_RETURN_END)
		{
			PX_ASSERTIFX(pSyntax->reg_ast_stack.size !=0, "Error:ast stack size not match");
			PX_Syntax_Terminate(pSyntax, "ast:ok:compile completed");
			return PX_TRUE;
		}
		else if (ast_return == PX_SYNTAX_AST_RETURN_CONTINUE)
		{
			continue;
		}
		else
		{
			PX_ASSERT();
			return PX_FALSE;
		}
	}
	return PX_FALSE;
}


px_void PX_Syntax_Terminate(PX_Syntax* pSyntax, const px_char message[])
{
	px_char build_content[1024] = { 0 };
	//numeric too long
	PX_sprintf3(build_content,sizeof(build_content), "ast:terminate:%1:%2 %3 \n", \
		PX_STRINGFORMAT_STRING(PX_Syntax_GetCurrentLexerSourceName(pSyntax)), \
		PX_STRINGFORMAT_INT(PX_Syntax_GetCurrentLexerLine(pSyntax)), \
		PX_STRINGFORMAT_STRING(message));
	PX_Syntax_Message(pSyntax, build_content);
	//ternimate
	PX_Syntax_ExecuteAstOpcode(pSyntax, "terminate");
}

px_void PX_Syntax_AstReturn(PX_Syntax* pSyntax,PX_Syntax_ast *past,const px_char message[])
{
	past->pbnfnode = &pSyntax->bnfnode_return;
	if(!PX_StringSet(&past->pbnfnode->constant, message))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_AstReturn out of memory");
	}
}

px_void PX_Syntax_Message(PX_Syntax* pSyntax, const px_char message[])
{
	if (!PX_StringAppend(&pSyntax->message, message))
	{
		PX_ASSERT();
		return;
	}
}

px_string* PX_Syntax_GetMessage(PX_Syntax* pSyntax)
{
	return &pSyntax->message;
}

px_void PX_Syntax_MessageClear(PX_Syntax* pSyntax)
{
	PX_StringClear(&pSyntax->message);
}

PX_SYNTAX_FUNCTION(PX_Syntax_TokenRender)
{
	const px_char* pcurrent = PX_Syntax_GetCurrentLexeme(pSyntax);
	px_int len = PX_strlen(pcurrent);
	px_color clr;
	px_int begin_index, begin,end_index, end;
	begin_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	if (pcurrent[0]==')'&&pcurrent[1]=='\0')
		pcurrent ="(";
	if (pcurrent[0] == ']' && pcurrent[1] == '\0')
		pcurrent= "[";
	if (pcurrent[0] == '}' && pcurrent[1] == '\0')
		pcurrent = "{";

	if (begin_index==end_index)
	{
		clr._argb.ucolor = PX_crc32(pcurrent, len);
		clr._argb.a = 255;
		clr._argb.r = clr._argb.r < 128 ? clr._argb.r + 128 : clr._argb.r;
		clr._argb.g = clr._argb.g < 128 ? clr._argb.g + 128 : clr._argb.g;
		clr._argb.b = clr._argb.b < 128 ? clr._argb.b + 128 : clr._argb.b;
		if (PX_Syntax_NewStaticMapToken(pSyntax, begin_index, begin, end_index, end, clr, pcurrent) ==PX_NULL)
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_TokenRender out of memory");
			return PX_FALSE;
		}
	}
	return PX_TRUE;
	
}

PX_SYNTAX_FUNCTION(PX_Syntax_TokenRenderScope)
{
	px_char scope[16] = "scope";
	px_color clr;
	px_int begin_index, begin, end_index, end;
	PX_strcat(scope, PX_itos(PX_Syntax_GetLifetime(pSyntax), 10).data);
	begin_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	if (begin_index == end_index)
	{
		clr._argb.ucolor = PX_crc32(scope, PX_strlen(scope));
		clr._argb.a = 255;
		clr._argb.r = clr._argb.r < 128 ? clr._argb.r + 128 : clr._argb.r;
		clr._argb.g = clr._argb.g < 128 ? clr._argb.g + 128 : clr._argb.g;
		clr._argb.b = clr._argb.b < 128 ? clr._argb.b + 128 : clr._argb.b;

		if (PX_NULL==PX_Syntax_NewStaticMapToken(pSyntax, begin_index, begin, end_index, end, clr, scope))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_TokenRender out of memory");
			return PX_FALSE;
		}
	}
	return PX_TRUE;
}



px_bool PX_Syntax_IR_optimize_pass1(px_string* ir)
{
	px_bool opt;

	if (!ir || !ir->buffer)
	{
		return PX_FALSE;
	}

	do
	{
		opt = PX_FALSE;

		/* Handle copy-back before the forwarding rules overlap it. */
		opt |= PX_StringTrimer_Solve(ir, "mov r0,r1\nmov r1,r0\n", "mov r0,r1\n");
		opt |= PX_StringTrimer_Solve(ir, "mov r1,r0\nmov r0,r1\n", "mov r1,r0\n");

		opt |= PX_StringTrimer_Solve(ir, "mov r0,%1\nmov r1,r0\n", "mov r1,%1\n");
		opt |= PX_StringTrimer_Solve(ir, "mov r1,%1\nmov r0,r1\n", "mov r0,%1\n");

		/* The IR compiler uses r0/r1 as expression temporaries. */
		opt |= PX_StringTrimer_Solve(ir, "mov r0,0\nadd r0,r1\n", "mov r0,r1\n");
		opt |= PX_StringTrimer_Solve(ir, "mov r1,0\nadd r1,r0\n", "mov r1,r0\n");

		opt |= PX_StringTrimer_Solve(ir, "push r%1\nstore%2 r%3,%4\npop r%1\n", "store%2 r%3,%4\n");

		opt = opt | PX_StringTrimer_Solve(ir, "push %1\npop %1\n", "");
		opt = opt | PX_StringTrimer_Solve(ir, "jmp %1\n%1:\n", "%1:\n");
		opt = opt | PX_StringTrimer_Solve(ir, "jmp %1\njmp %2\n", "jmp %1\n");
		opt = opt | PX_StringTrimer_Solve(ir, "ret\nret\n", "ret\n");
		opt = opt | PX_StringTrimer_Solve(ir, "ret\njmp %1\n", "ret\n");
		opt = opt | PX_StringTrimer_Solve(ir, "ret\nadd sp,%1\nret\n", "ret\n");
	} while (opt);

	return PX_TRUE;
}
