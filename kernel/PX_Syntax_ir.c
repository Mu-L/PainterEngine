#include "PX_Syntax_ir.h"


PX_SYNTAX_FUNCTION(PX_Syntax_IR_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:unknown ir error");
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_ir_skip_newline_spacer)
{
	px_int counter = 0;
	while (PX_TRUE)
	{
		px_char ch = PX_Syntax_PreviewNextChar(pSyntax);
		if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
		{
			PX_Syntax_GetNextChar(pSyntax);
			counter++;
			continue;
		}
		else
		{
			break;
		}
	}
	return counter != 0;
}

static px_int PX_Syntax_IR_GetRP(PX_Syntax* pSyntax)//resource pointer
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		return 0;
	}
	return PX_AbiGetValue_int(pscope, "rp");
}

static px_int PX_Syntax_IR_GetMP(PX_Syntax* pSyntax)//module base pointer
{
	return pSyntax->reg_module_base_addr;
}

static px_int PX_Syntax_IR_GetGP(PX_Syntax* pSyntax)//global pointer
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		return 0;
	}
	return PX_AbiGetValue_int(pscope, "gp");
}

static px_int PX_Syntax_IR_GetTP(PX_Syntax* pSyntax)//text pointer
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		return 0;
	}
	return PX_AbiGetValue_int(pscope, "tp");
}


static px_int PX_Syntax_IR_GetScanMode(PX_Syntax* pSyntax)
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope) return -1;
	return PX_AbiGetValue_int(pscope, "scan_mode");
}

static px_bool PX_Syntax_IR_EmitText(PX_Syntax* pSyntax, const px_byte* payload, px_int size)
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	px_int scan_mode;
	PX_Syntax_bin_map_to_ir mapEntry;
	px_dword bin_size;
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:EmitText no scope");
		return PX_FALSE;
	}
	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
	{
		px_int text_length = PX_AbiGetValue_int(pscope, "text_length");
		text_length += size;
		return PX_AbiSet_int(pscope, "text_length", text_length);
	}
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_RDATA)
	{
		return PX_TRUE;
	}
	
	bin_size = PX_AbiGet_buffer_size(pscope, "text");
	mapEntry.ip = bin_size;
	mapEntry.source_index = pSyntax->reg_expr_source_index;
	mapEntry.source_line = pSyntax->reg_expr_begin_line;
	if (!PX_Syntax_AppendBuffer(pSyntax, "scope", "bin_map_to_ir", (px_byte*)&mapEntry, sizeof(mapEntry)))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:EmitText failed to append bin_map_to_ir");
		return PX_FALSE;
	}
	
	return PX_Syntax_AppendBuffer(pSyntax, "scope", "text", payload, size);
}

static px_bool PX_Syntax_IR_EmitRdata(PX_Syntax* pSyntax, const px_byte* payload, px_int size)
{
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	px_int scan_mode;
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:EmitRdata no scope");
		return PX_FALSE;
	}
	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
	{
		return PX_TRUE;
	}
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_RDATA)
	{
		px_int rdata_length = PX_AbiGetValue_int(pscope, "rdata_length");
		rdata_length += size;
		return PX_AbiSet_int(pscope, "rdata_length", rdata_length);
	}
	return PX_Syntax_AppendBuffer(pSyntax, "scope", "rdata", payload, size);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_mp_const)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_int ivalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_unsigned_int"), "IR_ir_mp_const must follow an const_unsigned_int abi");
	ivalue = PX_atoi(PX_AbiGetValue_string(plast, "value"));
	ivalue += PX_Syntax_IR_GetMP(pSyntax);
	PX_AbiSet_string(plast, "value", PX_itos(ivalue, 10).data);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_gp_const)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_int ivalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_ir_gp_const must follow an const_int abi");
	ivalue = PX_atoi(PX_AbiGetValue_string(plast, "value"));
	ivalue += PX_Syntax_IR_GetGP(pSyntax);
	PX_AbiSet_string(plast, "value", PX_itos(ivalue, 10).data);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_tp_const)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_int ivalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_ir_tp_const must follow an const_int abi");
	ivalue = PX_atoi(PX_AbiGetValue_string(plast, "value"));
	ivalue += PX_Syntax_IR_GetTP(pSyntax);
	PX_AbiSet_string(plast, "value", PX_itos(ivalue, 10).data);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_rp_const)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_int ivalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_ir_rp_const must follow an const_int abi");
	ivalue = PX_atoi(PX_AbiGetValue_string(plast, "value"));
	ivalue += PX_Syntax_IR_GetRP(pSyntax);
	PX_AbiSet_string(plast, "value", PX_itos(ivalue, 10).data);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_mp)
{
	px_abi* new_mp = PX_Syntax_NewAbi(pSyntax, "const_int");
	if (!new_mp)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_mp Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(new_mp, "value", PX_itos(PX_Syntax_IR_GetMP(pSyntax), 10).data))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_mp Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_gp)
{
	px_abi* new_gp = PX_Syntax_NewAbi(pSyntax, "const_int");
	if (!new_gp)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_gp Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(new_gp, "value", PX_itos(PX_Syntax_IR_GetGP(pSyntax), 10).data))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_gp Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_tp)
{
	px_abi* new_tp = PX_Syntax_NewAbi(pSyntax, "const_int");
	if (!new_tp)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_tp Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(new_tp, "value", PX_itos(PX_Syntax_IR_GetTP(pSyntax), 10).data))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_tp Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_rp)
{
	px_abi* new_rp = PX_Syntax_NewAbi(pSyntax, "const_int");
	if (!new_rp)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_rp Memory Error");
		return PX_FALSE;
	}
	if (!PX_AbiSet_string(new_rp, "value", PX_itos(PX_Syntax_IR_GetRP(pSyntax), 10).data))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_rp Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_IR_rx)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_abi* new_rx;
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_rx must follow an identifier abi");
	pvalue = PX_AbiGet_string(plast, "value");
	
	if (PX_strequ2(pvalue, "ip"))
	{
		reg_index = 4;
	}
	else if (PX_strequ2(pvalue, "sp"))
	{
		reg_index = 5;
	}
	else if (PX_strequ2(pvalue, "bp"))
	{
		reg_index = 6;
	}
	else
	{
		if (pvalue[0] != 'r' && pvalue[0] != 'R')
		{
			return PX_FALSE;
		}
		else if (PX_charIsNumeric(pvalue[1]) && pvalue[2] == '\0')
		{
			if (pvalue[1] < '0' || pvalue[1]>'7')
			{
				PX_Syntax_Terminate(pSyntax, "ast:error:IR_rx target must be r0-r7");
				return PX_FALSE;
			}
			reg_index = pvalue[1] - '0';
		}
		else
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:Unknown RX target");
			return PX_FALSE;
		}
	}
	

	PX_Syntax_PopAbi(pSyntax);
	new_rx = PX_Syntax_NewAbi(pSyntax, "ir_rx");
	if (!new_rx)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_rx Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(new_rx, "index", reg_index))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_rx Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_fx)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_abi* new_fx;
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_fx must follow an identifier abi");
	pvalue = PX_AbiGet_string(plast, "value");
	if (pvalue[0] != 'f' && pvalue[0] != 'F')
	{
		return PX_FALSE;
	}

	if (PX_charIsNumeric(pvalue[1]) && pvalue[2] == '\0')
	{
		if (pvalue[1] < '0' || pvalue[1] > '3')
		{
			PX_Syntax_Terminate(pSyntax, "ast:error:IR_fx target must be f0-f3");
			return PX_FALSE;
		}
		reg_index = pvalue[1] - '0';
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_fx target must be f0-f3");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	new_fx = PX_Syntax_NewAbi(pSyntax, "ir_fx");
	if (!new_fx)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fx Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(new_fx, "index", reg_index))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fx Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_local_addr)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_int ivalue;
	px_abi* new_addr;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_local_addr must follow an const_int abi");
	pvalue = PX_AbiGet_string(plast, "value");
	ivalue = PX_atoi(pvalue);
	if (ivalue<0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_local_addr target must be non-negative");
		return PX_FALSE;
	}
	
	PX_Syntax_PopAbi(pSyntax);
	new_addr = PX_Syntax_NewAbi(pSyntax, "ir_addr");
	if (!new_addr)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_local_addr Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_dword(new_addr, "local_offset", ivalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_local_addr Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_const_addr)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_int ivalue;
	px_abi* new_addr;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_ir_const_addr must follow an const_int abi");
	pvalue = PX_AbiGet_string(plast, "value");
	ivalue = PX_atoi(pvalue);
	if (ivalue<0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_ir_const_addr target must be non-negative");
		return PX_FALSE;
	}				
	PX_Syntax_PopAbi(pSyntax);
	new_addr = PX_Syntax_NewAbi(pSyntax, "ir_addr");
	if (!new_addr)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_const_addr Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_dword(new_addr, "absolute_offset", ivalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_const_addr Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;

}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ir_label_addr)
{
	px_char payload[128]="labels.";
	px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_dword label_offset;
	PX_SYNTAX_IR_SCAN_MODE scan_mode;
	px_abi* new_addr;
	const px_char* pvalue;
	PX_ASSERTIFX(!plast||!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_ir_label_addr must follow an identifier abi");
	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	pvalue = PX_AbiGet_string(plast, "value");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL|| scan_mode == PX_SYNTAX_IR_SCAN_MODE_RDATA)
	{
		PX_Syntax_PopAbi(pSyntax);
		return PX_TRUE;
	}
	PX_strcat_s(payload, sizeof(payload), pvalue);
	if (!PX_AbiExist_Type(pscope,payload,PX_ABI_TYPE_DWORD))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_ir_label_addr label not found");
		return PX_FALSE;
	}
	label_offset = PX_AbiGetValue_dword(pscope, payload);
	PX_Syntax_PopAbi(pSyntax);
	new_addr = PX_Syntax_NewAbi(pSyntax, "ir_addr");
	if (!new_addr)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_label_addr Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_dword(new_addr, "absolute_offset", label_offset))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ir_label_addr Memory Error2");
		return PX_FALSE;
	}

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_stack_addr)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_int ivalue;
	px_abi* new_addr;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_stack_addr must follow an const_int abi");
	pvalue = PX_AbiGet_string(plast, "value");
	ivalue = PX_atoi(pvalue);
	if (ivalue<0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_stack_addr target must be non-negative");
		return PX_FALSE;
	}

	PX_Syntax_PopAbi(pSyntax);
	new_addr = PX_Syntax_NewAbi(pSyntax, "ir_addr");
	if (!new_addr)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_stack_addr Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_dword(new_addr, "stack_offset", ivalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_stack_addr Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_param_addr)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	const px_char* pvalue;
	px_int ivalue;
	px_abi* new_addr;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_param_addr must follow an const_int abi");
	pvalue = PX_AbiGet_string(plast, "value");
	ivalue = PX_atoi(pvalue);
	if (ivalue<0)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_param_addr target must be non-negative");
		return PX_FALSE;
	}

	PX_Syntax_PopAbi(pSyntax);
	new_addr = PX_Syntax_NewAbi(pSyntax, "ir_addr");
	if (!new_addr)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_param_addr Memory Error1");
		return PX_FALSE;
	}
	if (!PX_AbiSet_dword(new_addr, "param_offset", ivalue))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_param_addr Memory Error2");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_Syntax_IR_loadxx(PX_Syntax* pSyntax, px_bool signed_instr, px_int x)
{
	//loadu8[r0...r3], [n][bp + n]
	px_byte payload[8] = { 0 };
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_int reg_index;
	px_dword offset;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_addr"), "IR_loadxx must follow an addr abi");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_loadxx must follow an rx abi");
	reg_index = PX_AbiGetValue_int(psecondlast, "index");
	//opcode
	if (signed_instr)
	{
		switch (x)
		{
		case 8:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI8;
			break;
		case 16:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI16;
			break;
		case 32:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI32;
			break;
		default:
			PX_ASSERTX("IR_loadux x error");
			return PX_FALSE;
		}
	}
	else
	{
		switch (x)
		{
		case 8:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU8;
			break;
		case 16:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU16;
			break;
		case 32:
			payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU32;
			break;
		default:
			PX_ASSERTX("IR_loadux x error");
			return PX_FALSE;
		}
	}


	//target register
	payload[1] = reg_index & 0x0f;

	//0 is global 1 is local(bp-n) 2 is stack(sp+n) 3 is param(bp+n)

	if (PX_AbiExist(plast, "absolute_offset"))
	{
		offset = PX_AbiGetValue_dword(plast, "absolute_offset");
	}
	else if (PX_AbiExist(plast, "local_offset"))
	{
		offset = PX_AbiGetValue_dword(plast, "local_offset");
		payload[1] |= 0x10;
	}
	else if(PX_AbiExist(plast, "stack_offset"))
	{
		offset = PX_AbiGetValue_dword(plast, "stack_offset");
		payload[1] |= 0x20;
	}
	else if(PX_AbiExist(plast, "param_offset"))
	{
		offset = PX_AbiGetValue_dword(plast, "param_offset");
		payload[1] |= 0x30;
	}
	else
	{
		PX_ASSERTX("IR_loadxx address abi must have one of (module base +)absolute_offset/local_offset/stack_offset/param_offset");
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_loadxx address abi must have one of (module base +)absolute_offset/local_offset/stack_offset/param_offset");
		return PX_FALSE;
	}
	*(px_dword*)(payload + 2) = offset;

	if (!PX_Syntax_IR_EmitText(pSyntax,payload, 6))//6bytes
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_loaduxx Memory Error1");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);

	return PX_TRUE;
}



px_bool PX_Syntax_IR_storex(PX_Syntax* pSyntax,px_int x)
{
	//loadu8[r0...r3], [n][bp + n]
	px_byte payload[8] = { 0 };
	px_abi* plast =PX_Syntax_GetLastAbi(pSyntax); 
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_int reg_index;
	px_dword offset;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_storexx must follow an addr abi");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_addr"), "IR_storexx must follow an rx abi");
	reg_index = PX_AbiGetValue_int(plast, "index");
	//opcode
	switch (x)
	{
	case 8:
		payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE8;
		break;
	case 16:
		payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE16;
		break;
	case 32:
		payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE32;
		break;
	default:
		PX_ASSERTX("IR_storex error");
		return PX_FALSE;
	}


	//target register
	payload[1] = reg_index & 0x0f;

	//0 is global 1 is local(bp-n) 2 is stack(sp+n) 3 is param(bp+n)
	if (PX_AbiExist(psecondlast, "absolute_offset"))
	{
		offset = PX_AbiGetValue_dword(psecondlast, "absolute_offset");
	}
	else if (PX_AbiExist(psecondlast, "local_offset"))
	{
		offset = PX_AbiGetValue_dword(psecondlast, "local_offset");
		payload[1] |= 0x10;
	}
	else if (PX_AbiExist(psecondlast, "stack_offset"))
	{
		offset = PX_AbiGetValue_dword(psecondlast, "stack_offset");
		payload[1] |= 0x20;
	}
	else if (PX_AbiExist(psecondlast, "param_offset"))
	{
		offset = PX_AbiGetValue_dword(psecondlast, "param_offset");
		payload[1] |= 0x30;
	}
	else
	{
		PX_ASSERTX("IR_storexx address abi must have one of (module base +)absolute_offset/local_offset/stack_offset/param_offset");
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_storexx address abi must have one of (module base +)absolute_offset/local_offset/stack_offset/param_offset");
		return PX_FALSE;
	}
	*(px_dword*)(payload + 2) = offset;

	if (!PX_Syntax_IR_EmitText(pSyntax,payload, 6))//6bytes
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_storexx Memory Error1");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_IR_rdata)
{
	//label_name(identifier) db  string(eg:"base64:")
	px_char rdata_label_key[128] = "labels.";
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	const px_char* plabel_value;
	const px_char* plast_value;
	px_byte* payload = PX_NULL;
	px_int payload_length;
	px_int scan_mode;
	px_abi *pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ir_db: no scope abi found");
		return PX_FALSE;
	}
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_string")&& !PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_db must follow a const_string or identifier abi");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "identifier"), "IR_db must follow an identifier abi");
	plabel_value = PX_AbiGetValue_string(psecondlast, "value");
	plast_value = PX_AbiGetValue_string(plast, "value");
	if (PX_strlen(plabel_value) + PX_strlen(rdata_label_key) >= 128)
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_db label name too long");
		return PX_FALSE;
	}

	if (!PX_Syntax_CheckLastAbiName(pSyntax, "const_string"))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_db must follow a const_string abi");
		return PX_FALSE;
	}
	if (!PX_strequ3(plast_value,"base64:",7)||PX_Base64Check(plabel_value+7,!PX_strlen(plabel_value)-7))
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:IR_db invalid base64 string");
		return PX_FALSE;
	}
	payload_length = PX_Base64GetDecodeBytesLength(PX_strlen(plast_value) - 7);

	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	PX_strcat_s(rdata_label_key, 128, plabel_value);

	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
	{
		PX_Syntax_PopAbi(pSyntax);
		PX_Syntax_PopAbi(pSyntax);
		return PX_TRUE;
	}

	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_RDATA)
	{
		px_int module_base_addr = PX_Syntax_IR_GetMP(pSyntax);
		px_int text_length = PX_AbiGetValue_int(pscope, "text_length");
		px_int rdata_length = PX_AbiGetValue_int(pscope, "rdata_length");
		px_dword rdata_offset = (px_dword)( module_base_addr + text_length + rdata_length);
		if (!PX_AbiSet_dword(pscope, rdata_label_key, rdata_offset))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_db Memory Error4");
			return PX_FALSE;
		}
		rdata_length += payload_length;
		PX_AbiSet_int(pscope, "rdata_length", rdata_length);
		PX_Syntax_PopAbi(pSyntax);
		PX_Syntax_PopAbi(pSyntax);
		return PX_TRUE;
	}

	payload = PX_Malloc(px_byte, pSyntax->mp, payload_length);
	if (!payload)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_db Memory Error1");
		return PX_FALSE;
	}
	PX_Base64Decode(plast_value + 7,PX_strlen(plast_value) - 7 ,payload);

	if (!PX_Syntax_AppendBuffer(pSyntax,"scope","rdata", payload, payload_length))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_db Memory Error3");
		PX_Free(pSyntax->mp, payload);
		return PX_FALSE;
	}

	PX_Free(pSyntax->mp, payload);
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu8)
{
	return PX_Syntax_IR_loadxx(pSyntax, PX_FALSE, 8);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu16)
{
	return PX_Syntax_IR_loadxx( pSyntax, PX_FALSE, 16);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu32)
{
	return PX_Syntax_IR_loadxx( pSyntax, PX_FALSE, 32);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi8)
{
	return PX_Syntax_IR_loadxx( pSyntax, PX_TRUE, 8);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi16)
{
	return PX_Syntax_IR_loadxx( pSyntax, PX_TRUE, 16);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi32)
{
	return PX_Syntax_IR_loadxx( pSyntax, PX_TRUE, 32);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store8)
{
	return PX_Syntax_IR_storex( pSyntax, 8);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store16)
{
	return PX_Syntax_IR_storex( pSyntax, 16);
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store32)
{
	return PX_Syntax_IR_storex( pSyntax, 32);
}

// ============================================================
//  loadxxr (2 bytes): the register supplies the address and receives the value
//   assembly: load*r reg
//   byte0: opcode
//   byte1: register index (low 4 bits)
// ============================================================
static px_bool PX_Syntax_IR_loadxxr(PX_Syntax* pSyntax, px_bool signed_instr, px_int x)
{
	px_byte payload[2] = { 0 };
	px_abi* preg = PX_Syntax_GetLastAbi(pSyntax);
	px_int reg_index;

	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_loadxxr must follow an rx abi");
	reg_index = PX_AbiGetValue_int(preg, "index");

	if (signed_instr)
	{
		switch (x)
		{
		case 8:  payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI8R;  break;
		case 16: payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI16R; break;
		case 32: payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADI32R; break;
		default: PX_ASSERTX("IR_loadxxr x error"); return PX_FALSE;
		}
	}
	else
	{
		switch (x)
		{
		case 8:  payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU8R;  break;
		case 16: payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU16R; break;
		case 32: payload[0] = PX_SYNTAX_MACHINE_OPCODE_LOADU32R; break;
		default: PX_ASSERTX("IR_loadxxr x error"); return PX_FALSE;
		}
	}

	payload[1] = (px_byte)(reg_index & 0x0f);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_loadxxr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu8r)  { return PX_Syntax_IR_loadxxr(pSyntax, PX_FALSE, 8);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu16r) { return PX_Syntax_IR_loadxxr(pSyntax, PX_FALSE, 16); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadu32r) { return PX_Syntax_IR_loadxxr(pSyntax, PX_FALSE, 32); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi8r)  { return PX_Syntax_IR_loadxxr(pSyntax, PX_TRUE,  8);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi16r) { return PX_Syntax_IR_loadxxr(pSyntax, PX_TRUE,  16); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_loadi32r) { return PX_Syntax_IR_loadxxr(pSyntax, PX_TRUE,  32); }

// ============================================================
//  storexr (2 bytes): register-to-register store
//   assembly: store*r addr_reg, val_reg   (first=addr, second=val)
//   byte0: opcode
//   byte1: (0~3) val_reg | (4~7) addr_reg
// ============================================================
static px_bool PX_Syntax_IR_storexr(PX_Syntax* pSyntax, px_int x)
{
	px_byte payload[2] = { 0 };
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);       // val_reg
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax); // addr_reg
	px_int val_index, addr_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"),       "IR_storexr last must be ir_rx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_storexr second-last must be ir_rx");
	addr_index = PX_AbiGetValue_int(psecondlast, "index");
	val_index  = PX_AbiGetValue_int(plast, "index");

	switch (x)
	{
	case 8:  payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE8R;  break;
	case 16: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE16R; break;
	case 32: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE32R; break;
	default: PX_ASSERTX("IR_storexr x error"); return PX_FALSE;
	}

	payload[1] = (px_byte)((val_index & 0x0f) | ((addr_index & 0x0f) << 4));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_storexr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store8r)  { return PX_Syntax_IR_storexr(pSyntax, 8);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store16r) { return PX_Syntax_IR_storexr(pSyntax, 16); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store32r) { return PX_Syntax_IR_storexr(pSyntax, 32); }

// ============================================================
//  storexrc (6 bytes): register-address constant-value store
//   assembly: store*rc addr_reg, const   (first=addr_reg, second=const)
//   byte0: opcode
//   byte1: addr_reg index (full byte; only low 4 bits meaningful)
//   bytes 2..5: 32-bit constant value
// ============================================================
static px_bool PX_Syntax_IR_storexrc(PX_Syntax* pSyntax, px_int x)
{
	px_byte payload[6] = { 0 };
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);       // const_int
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax); // addr_reg
	px_int  addr_index;
	px_dword const_value;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"),  "IR_storexrc last must be const_int");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"),"IR_storexrc second-last must be ir_rx");
	addr_index  = PX_AbiGetValue_int(psecondlast, "index");
	const_value = (px_dword)PX_atoi(PX_AbiGet_string(plast, "value"));

	switch (x)
	{
	case 8:  payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE8RC;  break;
	case 16: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE16RC; break;
	case 32: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE32RC; break;
	default: PX_ASSERTX("IR_storexrc x error"); return PX_FALSE;
	}

	payload[1] = (px_byte)(addr_index & 0xff);
	*(px_dword*)(payload + 2) = const_value;

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_storexrc Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store8rc)  { return PX_Syntax_IR_storexrc(pSyntax, 8);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store16rc) { return PX_Syntax_IR_storexrc(pSyntax, 16); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store32rc) { return PX_Syntax_IR_storexrc(pSyntax, 32); }

// ============================================================
//  storexc (10 bytes): immediate-address constant-value store
//   assembly: store*c [addr_expr], const   (first=ir_addr, second=const)
//   byte0: opcode
//   byte1: addr mode (0=global, 1=local bp-, 2=stack sp+)
//   bytes 2..5: 32-bit dst address offset
//   bytes 6..9: 32-bit constant value
// ============================================================
static px_bool PX_Syntax_IR_storexc(PX_Syntax* pSyntax, px_int x)
{
	px_byte payload[10] = { 0 };
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);       // const_int
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax); // ir_addr
	px_dword offset;
	px_byte  addr_mode;
	px_dword const_value;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"),     "IR_storexc last must be const_int");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_addr"), "IR_storexc second-last must be ir_addr");

	if (PX_AbiExist(psecondlast, "absolute_offset"))
	{
		offset    = PX_AbiGetValue_dword(psecondlast, "absolute_offset");
		addr_mode = 0x00;
	}
	else if (PX_AbiExist(psecondlast, "local_offset"))
	{
		offset    = PX_AbiGetValue_dword(psecondlast, "local_offset");
		addr_mode = 0x01;
	}
	else if (PX_AbiExist(psecondlast, "stack_offset"))
	{
		offset    = PX_AbiGetValue_dword(psecondlast, "stack_offset");
		addr_mode = 0x02;
	}
	else if (PX_AbiExist(psecondlast, "param_offset"))
	{
		offset    = PX_AbiGetValue_dword(psecondlast, "param_offset");
		addr_mode = 0x03;
	}
	else
	{
		PX_ASSERTX("IR_storexc address abi must have one of (module base +)absolute_offset/local_offset/stack_offset/param_offset");
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_storexc bad address abi");
		return PX_FALSE;
	}
	const_value = (px_dword)PX_atoi(PX_AbiGet_string(plast, "value"));

	switch (x)
	{
	case 8:  payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE8C;  break;
	case 16: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE16C; break;
	case 32: payload[0] = PX_SYNTAX_MACHINE_OPCODE_STORE32C; break;
	default: PX_ASSERTX("IR_storexc x error"); return PX_FALSE;
	}

	payload[1] = addr_mode;
	*(px_dword*)(payload + 2) = offset;
	*(px_dword*)(payload + 6) = const_value;

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 10))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_storexc Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_store8c)  { return PX_Syntax_IR_storexc(pSyntax, 8);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store16c) { return PX_Syntax_IR_storexc(pSyntax, 16); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_store32c) { return PX_Syntax_IR_storexc(pSyntax, 32); }


PX_SYNTAX_FUNCTION(PX_Syntax_IR_push)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_push must follow an rx abi");
	px_int reg_index = PX_AbiGetValue_int(plast, "index");
	px_byte payload[2];
	//opcode
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_PUSH;
	//target register
	payload[1] = ((reg_index & 0x0f) << 4) | 0x01; //type 1 is register
	if (!PX_Syntax_IR_EmitText(pSyntax,payload, 2))//2bytes
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_push Memory Error1");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}



PX_SYNTAX_FUNCTION(PX_Syntax_IR_pop)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_pop must follow an rx abi");
	px_int reg_index = PX_AbiGetValue_int(plast, "index");
	px_byte payload[2];
	//opcode
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_POP;
	//target register
	payload[1] = ((reg_index & 0x0f) << 4) | 0x01; //type 1 is register
	if (!PX_Syntax_IR_EmitText(pSyntax,payload, 2))//2bytes
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_pop Memory Error1");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_popn)
{
	px_byte payload[6];
	//opcode
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_POPN;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	if (!PX_Syntax_IR_EmitText(pSyntax,payload, 1))//1bytes
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_popn Memory Error1");
		return PX_FALSE;
	}
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_ir_init)
{
	px_abi* pscope;
	if (pSyntax->reg_abi_stack.size!=0)
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_EnterScope(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error1");
		return PX_FALSE;
	}
	pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!PX_AbiSet_int(pscope, "tp", 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error2");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pscope, "gp", 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error3");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pscope, "rp", 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error4");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pscope, "scan_mode", PX_SYNTAX_IR_SCAN_MODE_LABEL))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error5");
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(pscope, "text_length", 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error6");
		return PX_FALSE;
	}

	if (!PX_AbiSet_int(pscope, "rdata_length", 0))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_init Memory Error7");
		return PX_FALSE;
	}
	return PX_TRUE;
}



PX_SYNTAX_FUNCTION(PX_Syntax_ir_comment_loc)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_abi* pscope;
	px_dword bin_size = 0;
	px_int scan_mode;
	PX_Syntax_bin_map_to_source map_entry;

	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "ir_loc: last abi must be const_int");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "const_int"), "ir_loc: second last abi must be const_int");

	pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ir_loc: no scope abi found");
		return PX_FALSE;
	}

	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	if (scan_mode != PX_SYNTAX_IR_SCAN_MODE_TEXT)
	{
		PX_Syntax_PopAbi(pSyntax);
		PX_Syntax_PopAbi(pSyntax);
		return PX_TRUE;
	}

	// get current ip from bin data size
	if (!PX_AbiGet_data(pscope, "text", &bin_size))
	{
		bin_size = 0;
	}

	map_entry.ip = (px_dword)bin_size;
	map_entry.source_index = (px_dword)PX_atoi(PX_AbiGetValue_string(psecondlast, "value"));
	map_entry.source_line = (px_dword)PX_atoi(PX_AbiGetValue_string(plast, "value"));

	// append map entry to scope "bin_map_to_source" data
	if (!PX_AbiExist_Type(pscope, "bin_map_to_source", PX_ABI_TYPE_DATA))
	{
		if (!PX_AbiSet_data(pscope, "bin_map_to_source", (px_byte*)&map_entry, sizeof(map_entry)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:ir_loc Memory Error1");
			return PX_FALSE;
		}
	}
	else
	{
		px_dword map_size = 0;
		PX_Syntax_bin_map_to_source* pdata = PX_AbiGet_data(pscope, "bin_map_to_source", &map_size);
		if (map_size)
		{
			PX_Syntax_bin_map_to_source* plast_entry = &pdata[map_size / sizeof(PX_Syntax_bin_map_to_source) - 1];
			if (plast_entry->source_index != map_entry.source_index || plast_entry->source_line != map_entry.source_line)
			{
				if (!PX_AbiAppend_data(pscope, "bin_map_to_source", (px_byte*)&map_entry, sizeof(map_entry)))
				{
					PX_Syntax_Terminate(pSyntax, "runtime:error:ir_loc Memory Error2");
					return PX_FALSE;
				}
			}
		}
	}

	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_ir_comment)
{
	px_int begin_source_index, begin;
	px_int end_source_index, end;
	PX_SYNTAXLEXER_LEXEME_TYPE type = PX_Syntax_GetNextLexeme(pSyntax);
	if (type == PX_SYNTAXLEXER_LEXEME_TYPE_DELIMITER)
	{
		if (PX_strequ(PX_Syntax_GetCurrentLexeme(pSyntax), ";"))
		{
			begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
			begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
			//skip the line
			while (PX_TRUE)
			{
				px_char ch = PX_Syntax_GetNextChar(pSyntax);
				if (ch == '\0')
				{
					end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
					end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
					if (begin_source_index== end_source_index)
					{
						if (!PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end - 1, PX_COLOR(255, 255, 192, 64), "comment context"))
						{
							PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
							return PX_FALSE;
						}
					}
					
					return PX_TRUE;
				}
				if (ch == '\n')
				{
					end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
					end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
					if (!PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 255, 192, 64), "comment context"))
					{
						PX_Syntax_Terminate(pSyntax, "runtime:error:out of memory");
						return PX_FALSE;
					}
					return PX_TRUE;
				}
			}
		}
		return PX_FALSE;
	}
	return PX_FALSE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_ir_import)
{
	//last abi is const_string
	px_int begin_source_index, end_source_index,begin,end;
	px_int scan_mode;
	px_abi* pscope_abi = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	PX_ASSERTIFX(!pscope_abi, "import: no scope abi found");
	PX_ASSERTIFX(!plast, "import: no abi found");
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_string"), "import: must follow an const_string abi");
	begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);

	scan_mode = PX_AbiGetValue_int(pscope_abi, "scan_mode");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
	{
		if (PX_AbiExist_Type(pscope_abi, "imports", PX_ABI_TYPE_STRING))
		{
			if (!PX_AbiAppend_string(pscope_abi, "imports", ","))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:import Memory Error");
				return PX_FALSE;
			}
		}

		if (!PX_AbiAppend_string(pscope_abi, "imports", PX_Syntax_GetCurrentLexeme(pSyntax)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:import Memory Error");
			return PX_FALSE;
		}
	}

	PX_Syntax_PopAbi(pSyntax);
	if(begin_source_index==end_source_index)
		PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 255, 66, 96), "label");

	return PX_TRUE;
}

static px_bool PX_Syntax_ir_label_exec(struct _PX_Syntax* pSyntax, struct _PX_Syntax_ast* past, px_void* userptr, px_bool export)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pscope;
	px_char labels_payload[128] = "labels.";
	const px_char* label_name;
	px_dword current_offset;
	px_int scan_mode;
	px_int begin_source_index, begin;
	px_int end_source_index, end;
	px_int module_base_addr = PX_Syntax_IR_GetMP(pSyntax);
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "ir_label: must follow an identifier abi");
	label_name = PX_AbiGet_string(plast, "value");
	if (!label_name || label_name[0] == '\0')
	{
		PX_Syntax_Terminate(pSyntax, "ast:error:ir_label: empty label name");
		return PX_FALSE;
	}

	begin_source_index = PX_AbiGetValue_int(plast, "source_index");
	begin = PX_AbiGetValue_int(plast, "begin");

	end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);

	pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	if (!pscope)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:ir_label: no scope abi found");
		return PX_FALSE;
	}

	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");

	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
	{
		current_offset = (px_dword)(module_base_addr + PX_AbiGetValue_int(pscope, "text_length"));
		PX_strcat(labels_payload, label_name);
		if (!PX_AbiSet_dword(pscope, labels_payload, current_offset))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:ir_label Memory Error");
			return PX_FALSE;
		}
	}

	if (export)
	{
		px_string export_labels_payload;
		current_offset = (px_dword)(module_base_addr + PX_AbiGetValue_int(pscope, "text_length"));
		if (!PX_StringInitializeFormat1(pSyntax->mp,&export_labels_payload,"exports.%1",PX_STRINGFORMAT_STRING(label_name)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_ir_label_exec out of memory");
			return PX_FALSE;
		}

		if (!PX_AbiSet_dword(pscope, PX_StringGetText(&export_labels_payload), current_offset))
		{
			PX_StringFree(&export_labels_payload);
			PX_Syntax_Terminate(pSyntax, "runtime:error:ir_label Memory Error");
			return PX_FALSE;
		}
		PX_StringFree(&export_labels_payload);	
	}

	PX_Syntax_PopAbi(pSyntax);

	PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 255, 66, 96), "label");

	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_ir_label)
{
	return PX_Syntax_ir_label_exec(pSyntax, past, userptr, PX_FALSE);
}

PX_SYNTAX_FUNCTION(PX_Syntax_export_ir_label)
{
	return PX_Syntax_ir_label_exec(pSyntax, past, userptr, PX_TRUE);
}

px_int PX_Syntax_Disassemble_loadxx(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6)
	{
		return 0;
	}
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_LOADU8:
		PX_strcat(out, "loadu8 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADU16:
		PX_strcat(out, "loadu16 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADU32:
		PX_strcat(out, "loadu32 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI8:
		PX_strcat(out, "loadi8 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI16:
		PX_strcat(out, "loadi16 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI32:
		PX_strcat(out, "loadi32 ");
		break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_loadxx: opcode error");
		return 0;
	}
	//target register
	do
	{
		px_dword target_reg = payload[1] & 0x0f;
		px_dword addr_mode = payload[1] & 0xf0;
		px_dword offset = *(px_dword*)(payload + 2);
		px_char temp[32] = { 0 };

		PX_strcat(out, "r");
		PX_strcatchar(out, (px_char)('0' + target_reg));
		PX_strcatchar(out, ',');
		if (addr_mode == 0x10)
		{
			PX_sprintf1(temp, sizeof(temp), "bp-%1", PX_STRINGFORMAT_INT(offset));
		}
		else if (addr_mode == 0x20)
		{
			PX_sprintf1(temp, sizeof(temp), "sp+%1", PX_STRINGFORMAT_INT(offset));
		}
		else if (addr_mode == 0x30)
		{
			PX_sprintf1(temp, sizeof(temp), "bp+%1", PX_STRINGFORMAT_INT(offset));
		}
		else
		{
			PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(offset));
		}
		PX_strcat(out, temp);
	} while (0);
	return 6;
}

px_int PX_Syntax_Disassemble_storexx(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6)
	{
		return 0;
	}
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_STORE8:
		PX_strcat(out, "store8 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE16:
		PX_strcat(out, "store16 ");
		break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE32:
		PX_strcat(out, "store32 ");
		break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_storexx: opcode error");
		return 0;
	}
	//target register
	do
	{
		px_dword source_reg = payload[1] & 0x0f;
		px_dword addr_mode = payload[1] & 0xf0;
		px_dword offset = *(px_dword*)(payload + 2);
		px_char temp[32] = { 0 };

		if (addr_mode == 0x10)
		{
			PX_sprintf1(temp, sizeof(temp), "bp-%1", PX_STRINGFORMAT_INT(offset));
		}
		else if (addr_mode == 0x20)
		{
			PX_sprintf1(temp, sizeof(temp), "sp+%1", PX_STRINGFORMAT_INT(offset));
		}
		else if (addr_mode == 0x30)
		{
			PX_sprintf1(temp, sizeof(temp), "bp+%1", PX_STRINGFORMAT_INT(offset));
		}
		else
		{
			PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(offset));
		}
		PX_strcat(out, temp);
		PX_strcat(out, ",r");
		PX_strcatchar(out, (px_char)('0' + source_reg));
	} while (0);
	return 6;
}

px_int PX_Syntax_Disassemble_loadxxr(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2)
		return 0;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_LOADU8R:  PX_strcat(out, "loadu8 ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADU16R: PX_strcat(out, "loadu16 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADU32R: PX_strcat(out, "loadu32 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI8R:  PX_strcat(out, "loadi8 ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI16R: PX_strcat(out, "loadi16 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_LOADI32R: PX_strcat(out, "loadi32 "); break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_loadxxr: opcode error");
		return 0;
	}
	reg = payload[1] & 0x0f;
	PX_strcat(out, "r");
	PX_strcatchar(out, (px_char)('0' + reg));
	return 2;
}

px_int PX_Syntax_Disassemble_storexr(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int val_reg, addr_reg;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2)
		return 0;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_STORE8R:  PX_strcat(out, "store8 ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE16R: PX_strcat(out, "store16 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE32R: PX_strcat(out, "store32 "); break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_storexr: opcode error");
		return 0;
	}
	val_reg  = payload[1] & 0x0f;
	addr_reg = (payload[1] >> 4) & 0x0f;
	PX_strcat(out, "r");
	PX_strcatchar(out, (px_char)('0' + addr_reg));
	PX_strcat(out, ",r");
	PX_strcatchar(out, (px_char)('0' + val_reg));
	return 2;
}

px_int PX_Syntax_Disassemble_storexrc(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int  addr_reg;
	px_dword cval;
	px_char  temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6)
		return 0;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_STORE8RC:  PX_strcat(out, "store8 ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE16RC: PX_strcat(out, "store16 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE32RC: PX_strcat(out, "store32 "); break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_storexrc: opcode error");
		return 0;
	}
	addr_reg = payload[1] & 0x0f;
	cval     = *(px_dword*)(payload + 2);
	PX_strcat(out, "r");
	PX_strcatchar(out, (px_char)('0' + addr_reg));
	PX_strcat(out, ",");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(cval));
	PX_strcat(out, temp);
	return 6;
}

px_int PX_Syntax_Disassemble_storexc(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_byte  addr_mode;
	px_dword offset;
	px_dword cval;
	px_char  temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 10)
		return 0;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_STORE8C:  PX_strcat(out, "store8 ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE16C: PX_strcat(out, "store16 "); break;
	case PX_SYNTAX_MACHINE_OPCODE_STORE32C: PX_strcat(out, "store32 "); break;
	default:
		PX_ASSERTX("PX_Syntax_Disassemble_storexc: opcode error");
		return 0;
	}
	addr_mode = payload[1];
	offset    = *(px_dword*)(payload + 2);
	cval      = *(px_dword*)(payload + 6);
	if (addr_mode == 0x01)
	{
		PX_sprintf1(temp, sizeof(temp), "bp-%1", PX_STRINGFORMAT_INT(offset));
	}
	else if (addr_mode == 0x02)
	{
		PX_sprintf1(temp, sizeof(temp), "sp+%1", PX_STRINGFORMAT_INT(offset));
	}
	else if (addr_mode == 0x03)
	{
		PX_sprintf1(temp, sizeof(temp), "bp+%1", PX_STRINGFORMAT_INT(offset));
	}
	else
	{
		PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(offset));
	}
	PX_strcat(out, temp);
	PX_strcat(out, ",");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(cval));
	PX_strcat(out, temp);
	return 10;
}

px_int PX_Syntax_Disassemble_push(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int optype;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2)
	{
		return 0;
	}
	optype = payload[1] & 0x0f;
	switch (optype)
	{
	case 0:
		break;
	case 1:
	{
		px_int reg_index = (payload[1] & 0xf0) >> 4;
		if (reg_index <= 3)
		{
			PX_strset(out, "push r");
			PX_strcatchar(out, (px_char)('0' + reg_index));
		}
		else if (reg_index == 4)
		{
			PX_strset(out, "push ip");
		}
		else if (reg_index == 5)
		{
			PX_strset(out, "push sp");
		}
		else if (reg_index == 6)
		{
			PX_strset(out, "push bp");
		}
		else if (reg_index == 7)
		{
			PX_strset(out, "push flag");
		}
		else
		{
			PX_strset(out, "unknow");
		}
	}
	break;
	default:
		return 0;
		break;
	}
	return 2;

}

px_int PX_Syntax_Disassemble_pop(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int optype;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2)
	{
		return 0;
	}
	optype = payload[1] & 0x0f;
	switch (optype)
	{
	case 0:
		break;
	case 1:
	{
		px_int reg_index = (payload[1] & 0xf0) >> 4;
		if (reg_index == 4)
		{
			PX_strset(out, "pop ip");
		}
		else if (reg_index == 5)
		{
			PX_strset(out, "pop sp");
		}
		else if (reg_index == 6)
		{
			PX_strset(out, "pop bp");
		}
		else if (reg_index == 7)
		{
			PX_strset(out, "pop flag");
		}
		else
		{
			if (reg_index <= 3)
			{
				PX_strset(out, "pop r");
				PX_strcatchar(out, (px_char)('0' + reg_index));
			}
			else
			{
				PX_strcat(out, "unknow");
				return 0;
			}
		}
	}
	break;
	default:
		return 0;
		break;
	}
	return 2;
}

px_int PX_Syntax_Disassemble_popn(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 1)
	{
		return 0;
	}
	PX_strset(out, "popn");
	return 1;
}

// ============================================================
//  nop
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_nop)
{
	px_byte payload[1];
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_NOP;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 1))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_nop Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_int PX_Syntax_Disassemble_nop(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 1)
		return 0;
	PX_strset(out, "nop");
	return 1;
}


// ============================================================
//  movr[r0...r7,f0...f3], [r0...r7,f0...f3]  (3 bytes)
//  byte0: opcode
//  byte1: (dest_type 0~3) | (dest_index 4~7)
//  byte2: (src_type  0~3) | (src_index  4~7)
//  type: 0=common register  1=float register
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_movr)
{
	px_abi* plast        = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast  = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[3];
	px_int dest_type, dest_index, src_type, src_index;

	// last is src, second-last is dest
	if (PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"))
	{
		src_type  = 0;
		src_index = PX_AbiGetValue_int(plast, "index");
	}
	else if (PX_Syntax_CheckLastAbiName(pSyntax, "ir_fx"))
	{
		src_type  = 1;
		src_index = PX_AbiGetValue_int(plast, "index");
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_movr: src must be ir_rx or ir_fx");
		return PX_FALSE;
	}

	if (PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"))
	{
		dest_type  = 0;
		dest_index = PX_AbiGetValue_int(psecondlast, "index");
	}
	else if (PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_fx"))
	{
		dest_type  = 1;
		dest_index = PX_AbiGetValue_int(psecondlast, "index");
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_movr: dest must be ir_rx or ir_fx");
		return PX_FALSE;
	}

	payload[0] = PX_SYNTAX_MACHINE_OPCODE_MOVR;
	payload[1] = (px_byte)((dest_type & 0x0f) | ((dest_index & 0x0f) << 4));
	payload[2] = (px_byte)((src_type  & 0x0f) | ((src_index  & 0x0f) << 4));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 3))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_movr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  movc  (7 bytes)
//  byte0: opcode
//  byte1: target register index
//  byte2: value/address mode (0=const_int/(module base +)absolute/gp+offset, 1=bp-offset, 2=sp+offset, 3=bp+offset)
//  bytes3-6: 32-bit address/offset
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_movc)
{
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[7] = { 0 };
	px_int reg_index;
	px_byte addr_mode;
	px_dword offset;

	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_movc: second-last must be ir_rx");
	reg_index = PX_AbiGetValue_int(psecondlast, "index");

	/* A direct ir_const is retained for signed integer immediates. */
	if (PX_Syntax_CheckLastAbiName(pSyntax, "const_int"))
	{
		addr_mode = 0;
		offset = (px_dword)PX_atoi(PX_AbiGet_string(plast, "value"));
	}
	else if (PX_Syntax_CheckLastAbiName(pSyntax, "ir_addr"))
	{
		if (PX_AbiExist(plast, "absolute_offset"))
		{
			addr_mode = 0;
			offset = PX_AbiGetValue_dword(plast, "absolute_offset");
		}
		else if (PX_AbiExist(plast, "local_offset"))
		{
			addr_mode = 1;
			offset = PX_AbiGetValue_dword(plast, "local_offset");
		}
		else if (PX_AbiExist(plast, "stack_offset"))
		{
			addr_mode = 2;
			offset = PX_AbiGetValue_dword(plast, "stack_offset");
		}
		else if (PX_AbiExist(plast, "param_offset"))
		{
			addr_mode = 3;
			offset = PX_AbiGetValue_dword(plast, "param_offset");
		}
		else
		{
			PX_ASSERTX("IR_movc address abi must have one of absolute_offset/local_offset/stack_offset/param_offset");
			PX_Syntax_Terminate(pSyntax, "runtime:error:IR_movc bad address abi");
			return PX_FALSE;
		}
	}
	else
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_movc: last must be const_int or ir_addr");
		return PX_FALSE;
	}

	payload[0] = PX_SYNTAX_MACHINE_OPCODE_MOVC;
	payload[1] = (px_byte)(reg_index & 0x07);
	payload[2] = addr_mode;
	PX_memcpy(payload + 3, &offset, sizeof(offset));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 7))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_movc Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  movf  (6 bytes)
//  byte0: opcode
//  byte1: (reg_index 0~2) | reserved(3~7)
//  bytes2-5: 32-bit float const (IEEE 754)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_movf)
{
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[6];
	px_int  reg_index;
	px_byte reg_field;
	px_float32 fval;
	const px_char* pvalue;

	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_float"), "IR_movf: last must be const_float");

	if (PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"))
	{
		reg_index = PX_AbiGetValue_int(psecondlast, "index");
		reg_field = (px_byte)(reg_index & 0x07); /* bit3=0: rx */
	}
	else if (PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_fx"))
	{
		reg_index = PX_AbiGetValue_int(psecondlast, "index");
		reg_field = (px_byte)((reg_index & 0x07) | 0x08); /* bit3=1: fx */
	}
	else
	{
		PX_ASSERTIFX(PX_TRUE, "IR_movf: second-last must be ir_rx or ir_fx");
		return PX_FALSE;
	}

	pvalue = PX_AbiGet_string(plast, "value");
	fval   = (px_float32)PX_atof(pvalue);

	payload[0] = PX_SYNTAX_MACHINE_OPCODE_MOVF;
	payload[1] = reg_field;
	*(px_float32*)(payload + 2) = fval;

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_movf Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  movn  (1 byte) -- r0=src, r1=dst, r2=size (implicit)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_movn)
{
	px_byte payload[1];
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_MOVN;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 1))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_movn Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_int PX_Syntax_Disassemble_movr(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_type, dest_index, src_type, src_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 3)
		return 0;
	dest_type  = payload[1] & 0x0f;
	dest_index = (payload[1] >> 4) & 0x0f;
	src_type   = payload[2] & 0x0f;
	src_index  = (payload[2] >> 4) & 0x0f;
	PX_strset(out, "mov ");
	// dest
	if (dest_type == 1)
	{
		PX_strcat(out, "f");
		PX_strcatchar(out, (px_char)('0' + dest_index));
	}
	else
	{
		if (dest_index <= 3) { PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index)); }
		else if (dest_index == 4) PX_strcat(out, "ip");
		else if (dest_index == 5) PX_strcat(out, "sp");
		else if (dest_index == 6) PX_strcat(out, "bp");
		else if (dest_index == 7) PX_strcat(out, "flag");
	}
	PX_strcat(out, ",");
	// src
	if (src_type == 1)
	{
		PX_strcat(out, "f");
		PX_strcatchar(out, (px_char)('0' + src_index));
	}
	else
	{
		if (src_index <= 3) { PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + src_index)); }
		else if (src_index == 4) PX_strcat(out, "ip");
		else if (src_index == 5) PX_strcat(out, "sp");
		else if (src_index == 6) PX_strcat(out, "bp");
		else if (src_index == 7) PX_strcat(out, "flag");
	}
	return 3;
}

px_int PX_Syntax_Disassemble_movc(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	px_byte addr_mode;
	px_dword cval;
	px_char temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 7)
		return 0;
	reg_index = payload[1] & 0x07;
	addr_mode = payload[2];
	PX_memcpy(&cval, payload + 3, sizeof(cval));
	PX_strset(out, "mov r");
	PX_strcatchar(out, (px_char)('0' + reg_index));
	PX_strcat(out, ",");
	if (addr_mode == 1)
		PX_strcat(out, "bp-");
	else if (addr_mode == 2)
		PX_strcat(out, "sp+");
	else if (addr_mode == 3)
		PX_strcat(out, "bp+");
	else if (addr_mode != 0)
		return 0;
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(cval));
	PX_strcat(out, temp);
	return 7;
}

px_int PX_Syntax_Disassemble_movf(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	px_int is_fx;
	px_float fval;
	px_char temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6)
		return 0;
	reg_index = payload[1] & 0x07;
	is_fx     = (payload[1] >> 3) & 0x01;
	fval = *(px_float*)(payload + 2);
	if (is_fx)
		PX_strset(out, "movf f");
	else
		PX_strset(out, "movf r");
	PX_strcatchar(out, (px_char)('0' + reg_index));
	PX_strcat(out, ",");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_FLOAT(fval));
	PX_strcat(out, temp);
	return 6;
}

// ============================================================
//  f2i / f2u / i2f / u2f  (2 bytes)
//  byte0: opcode
//  byte1: (0~3) 4bit rx index | (4~7) 4bit fx index
//  f2i: fx->rx   byte1: rx_index(0~3) | fx_index(4~7)
//  f2u: fx->rx   byte1: rx_index(0~3) | fx_index(4~7)
//  i2f: rx->fx   byte1: fx_index(0~3) | rx_index(4~7)
//  u2f: rx->fx   byte1: fx_index(0~3) | rx_index(4~7)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_f2i)
{
	// f2i rx, fx  -- last=ir_fx, second-last=ir_rx
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int rx_index, fx_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_fx"),       "IR_f2i: last must be ir_fx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_f2i: second-last must be ir_rx");
	rx_index = PX_AbiGetValue_int(psecondlast, "index");
	fx_index = PX_AbiGetValue_int(plast,       "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_F2I;
	payload[1] = (px_byte)((rx_index & 0x0f) | ((fx_index & 0x0f) << 4));
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_f2i Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_f2u)
{
	// f2u rx, fx  -- last=ir_fx, second-last=ir_rx
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int rx_index, fx_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_fx"),       "IR_f2u: last must be ir_fx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_f2u: second-last must be ir_rx");
	rx_index = PX_AbiGetValue_int(psecondlast, "index");
	fx_index = PX_AbiGetValue_int(plast,       "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_F2U;
	payload[1] = (px_byte)((rx_index & 0x0f) | ((fx_index & 0x0f) << 4));
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_f2u Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_i2f)
{
	// i2f fx, rx  -- last=ir_rx, second-last=ir_fx
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int rx_index, fx_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"),       "IR_i2f: last must be ir_rx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_fx"), "IR_i2f: second-last must be ir_fx");
	fx_index = PX_AbiGetValue_int(psecondlast, "index");
	rx_index = PX_AbiGetValue_int(plast,       "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_I2F;
	payload[1] = (px_byte)((fx_index & 0x0f) | ((rx_index & 0x0f) << 4));
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_i2f Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_u2f)
{
	// u2f fx, rx  -- last=ir_rx, second-last=ir_fx
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int rx_index, fx_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"),       "IR_u2f: last must be ir_rx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_fx"), "IR_u2f: second-last must be ir_fx");
	fx_index = PX_AbiGetValue_int(psecondlast, "index");
	rx_index = PX_AbiGetValue_int(plast,       "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_U2F;
	payload[1] = (px_byte)((fx_index & 0x0f) | ((rx_index & 0x0f) << 4));
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_u2f Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

px_int PX_Syntax_Disassemble_f2i(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int rx_index, fx_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	rx_index = payload[1] & 0x0f;
	fx_index = (payload[1] >> 4) & 0x0f;
	PX_strset(out, "f2i r");
	PX_strcatchar(out, (px_char)('0' + rx_index));
	PX_strcat(out, ",f");
	PX_strcatchar(out, (px_char)('0' + fx_index));
	return 2;
}

px_int PX_Syntax_Disassemble_f2u(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int rx_index, fx_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	rx_index = payload[1] & 0x0f;
	fx_index = (payload[1] >> 4) & 0x0f;
	PX_strset(out, "f2u r");
	PX_strcatchar(out, (px_char)('0' + rx_index));
	PX_strcat(out, ",f");
	PX_strcatchar(out, (px_char)('0' + fx_index));
	return 2;
}

px_int PX_Syntax_Disassemble_i2f(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int fx_index, rx_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	fx_index = payload[1] & 0x0f;
	rx_index = (payload[1] >> 4) & 0x0f;
	PX_strset(out, "i2f f");
	PX_strcatchar(out, (px_char)('0' + fx_index));
	PX_strcat(out, ",r");
	PX_strcatchar(out, (px_char)('0' + rx_index));
	return 2;
}

px_int PX_Syntax_Disassemble_u2f(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int fx_index, rx_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	fx_index = payload[1] & 0x0f;
	rx_index = (payload[1] >> 4) & 0x0f;
	PX_strset(out, "u2f f");
	PX_strcatchar(out, (px_char)('0' + fx_index));
	PX_strcat(out, ",r");
	PX_strcatchar(out, (px_char)('0' + rx_index));
	return 2;
}

// ============================================================
//  Integer ALU: neg (2 bytes), add/sub/mul/div/idiv/mod (2 bytes)
//  neg:  opcode | (0~3)4bit reg_index
//  binary: opcode | (dest 0~3)|(src 4~7)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_neg)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_neg must follow an ir_rx abi");
	reg_index  = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_NEG;
	payload[1] = (px_byte)(reg_index & 0x0f);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_neg Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// Register ALU instructions use two operands: dst = dst op src.
// byte0: opcode, byte1: dst (low nibble) | src (high nibble)
static px_bool PX_Syntax_IR_alu2(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* psrc = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pdst = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int  dest_index, src_index;

	if (!psrc || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst, "ir_rx") ||
		!PX_Syntax_CheckAbiName(psrc, "ir_rx") )
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2: operands must be ir_rx");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst, "index");
	src_index = PX_AbiGetValue_int(psrc, "index");

	payload[0] = opcode;
	payload[1] = (px_byte)((dest_index & 0x0f) | ((src_index & 0x0f) << 4));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2 Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_add)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ADD);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_sub)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SUB);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_mul)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_MUL);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_div)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_DIV);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_idiv) { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_IDIV); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_mod)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_MOD);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_imod) { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_IMOD); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_and)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_AND);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_or)   { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_OR);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_xor)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_XOR);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_shl)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SHL);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_shr)  { return PX_Syntax_IR_alu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SHR);  }

// Immediate ALU instructions use dst as the implicit left operand.
// byte0: opcode, byte1: dst, bytes2..5: constant
static px_bool PX_Syntax_IR_alu2ci(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* psrc = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pdst = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[6];
	px_int  dest_index;
	px_int  src2_value;

	if (!psrc || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2ci: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst, "ir_rx") ||
		!PX_Syntax_CheckAbiName(psrc, "const_int"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2ci: operands must be ir_rx + const_int");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst, "index");
	src2_value = (px_int)PX_atoi(PX_AbiGet_string(psrc, "value"));

	payload[0] = opcode;
	payload[1] = (px_byte)(dest_index & 0x0f);
	PX_memcpy(payload + 2, &src2_value, sizeof(px_int));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_alu2ci Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_addc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ADDC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_subc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SUBC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_mulc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_MULC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_divc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_DIVC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_idivc){ return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_IDIVC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_modc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_MODC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_imodc){ return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_IMODC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_andc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ANDC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_orc)  { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ORC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_xorc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_XORC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_shlc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SHLC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_shrc) { return PX_Syntax_IR_alu2ci(pSyntax, PX_SYNTAX_MACHINE_OPCODE_SHRC); }

px_int PX_Syntax_Disassemble_neg(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	reg_index = payload[1] & 0x0f;
	PX_strset(out, "neg r");
	PX_strcatchar(out, (px_char)('0' + reg_index));
	return 2;
}

px_int PX_Syntax_Disassemble_alu2(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, src_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	dest_index = payload[1] & 0x0f;
	src_index = (payload[1] >> 4) & 0x0f;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_ADD:  PX_strset(out, "add ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SUB:  PX_strset(out, "sub ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_MUL:  PX_strset(out, "mul ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_DIV:  PX_strset(out, "div ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_IDIV: PX_strset(out, "idiv "); break;
	case PX_SYNTAX_MACHINE_OPCODE_MOD:  PX_strset(out, "mod ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_IMOD: PX_strset(out, "imod "); break;
	case PX_SYNTAX_MACHINE_OPCODE_AND:  PX_strset(out, "and ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_OR:   PX_strset(out, "or ");   break;
	case PX_SYNTAX_MACHINE_OPCODE_XOR:  PX_strset(out, "xor ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SHL:  PX_strset(out, "shl ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SHR:  PX_strset(out, "shr ");  break;
	default: PX_strset(out, "unknow"); return 0;
	}
	PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",r"); PX_strcatchar(out, (px_char)('0' + src_index));
	return 2;
}

px_int PX_Syntax_Disassemble_alu2ci(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, const_value;
	px_char num[16];
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6) return 0;
	dest_index = payload[1] & 0x0f;
	PX_memcpy(&const_value, payload + 2, sizeof(px_int));
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_ADDC:  PX_strset(out, "add ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SUBC:  PX_strset(out, "sub ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_MULC:  PX_strset(out, "mul ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_DIVC:  PX_strset(out, "div ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_IDIVC: PX_strset(out, "idiv "); break;
	case PX_SYNTAX_MACHINE_OPCODE_MODC:  PX_strset(out, "mod ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_IMODC: PX_strset(out, "imod "); break;
	case PX_SYNTAX_MACHINE_OPCODE_ANDC:  PX_strset(out, "and ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_ORC:   PX_strset(out, "or ");   break;
	case PX_SYNTAX_MACHINE_OPCODE_XORC:  PX_strset(out, "xor ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SHLC:  PX_strset(out, "shl ");  break;
	case PX_SYNTAX_MACHINE_OPCODE_SHRC:  PX_strset(out, "shr ");  break;
	default: PX_strset(out, "unknow"); return 0;
	}
	PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",");
	PX_itoa(const_value, num, sizeof(num), 10);
	PX_strcat(out, num);
	return 6;
}

// ============================================================
//  FPU: fneg (2 bytes), fadd/fsub/fmul/fdiv (2 bytes)
//  fcmp / fcomi (2 bytes):  opcode | (0~3)f0 index | (4~7)f1 index
//  fneg: opcode | (0~3) fx_index
//  fadd/fsub/fmul/fdiv: opcode | (dest 0~3)|(src 4~7)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_fneg)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int fx_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_fx"), "IR_fneg must follow an ir_fx abi");
	fx_index   = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_FNEG;
	payload[1] = (px_byte)(fx_index & 0x0f);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fneg Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

static px_bool PX_Syntax_IR_fpu2(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* psrc = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pdst = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int  dest_index, src_index;

	if (!psrc || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst, "ir_fx") ||
		!PX_Syntax_CheckAbiName(psrc, "ir_fx"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2: operands must be ir_fx");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst, "index");
	src_index = PX_AbiGetValue_int(psrc, "index");

	payload[0] = opcode;
	payload[1] = (px_byte)((dest_index & 0x0f) | ((src_index & 0x0f) << 4));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2 Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_fadd) { return PX_Syntax_IR_fpu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FADD); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fsub) { return PX_Syntax_IR_fpu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FSUB); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fmul) { return PX_Syntax_IR_fpu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FMUL); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fdiv) { return PX_Syntax_IR_fpu2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FDIV); }

static px_bool PX_Syntax_IR_fpu2cf(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* psrc = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pdst = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[6];
	px_int  dest_index;
	px_float src2_value;

	if (!psrc || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2cf: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst, "ir_fx") ||
		!PX_Syntax_CheckAbiName(psrc, "const_float"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2cf: operands must be ir_fx + const_float");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst, "index");
	src2_value = (px_float)PX_atof(PX_AbiGet_string(psrc, "value"));

	payload[0] = opcode;
	payload[1] = (px_byte)(dest_index & 0x0f);
	PX_memcpy(payload + 2, &src2_value, sizeof(px_float));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_fpu2cf Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_faddc) { return PX_Syntax_IR_fpu2cf(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FADDC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fsubc) { return PX_Syntax_IR_fpu2cf(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FSUBC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fmulc) { return PX_Syntax_IR_fpu2cf(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FMULC); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fdivc) { return PX_Syntax_IR_fpu2cf(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FDIVC); }


// fcmp / fcomi: 2 bytes -- opcode | (0~3) f0_index | (4~7) f1_index
px_int PX_Syntax_Disassemble_fneg(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int fx_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	fx_index = payload[1] & 0x0f;
	PX_strset(out, "fneg f");
	PX_strcatchar(out, (px_char)('0' + fx_index));
	return 2;
}

px_int PX_Syntax_Disassemble_fpu2(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, src_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	dest_index = payload[1] & 0x0f;
	src_index = (payload[1] >> 4) & 0x0f;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_FADD: PX_strset(out, "fadd "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FSUB: PX_strset(out, "fsub "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FMUL: PX_strset(out, "fmul "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FDIV: PX_strset(out, "fdiv "); break;
	default: PX_strset(out, "unknow"); return 0;
	}
	PX_strcat(out, "f"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",f"); PX_strcatchar(out, (px_char)('0' + src_index));
	return 2;
}

px_int PX_Syntax_Disassemble_fpu2cf(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index;
	px_float const_value;
	px_char num[16];
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6) return 0;
	dest_index = payload[1] & 0x0f;
	PX_memcpy(&const_value, payload + 2, sizeof(px_float));
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_FADDC: PX_strset(out, "fadd "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FSUBC: PX_strset(out, "fsub "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FMULC: PX_strset(out, "fmul "); break;
	case PX_SYNTAX_MACHINE_OPCODE_FDIVC: PX_strset(out, "fdiv "); break;
	default: PX_strset(out, "unknow"); return 0;
	}
	PX_strcat(out, "f"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",");
	PX_sprintf1(num, sizeof(num), "%1", PX_STRINGFORMAT_FLOAT(const_value));
	PX_strcat(out, num);
	return 6;
}

// ============================================================
//  Logical: not (2 bytes), and/or/xor/shl/shr (2 bytes)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_not)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_not must follow an ir_rx abi");
	reg_index  = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_NOT;
	payload[1] = (px_byte)(reg_index & 0x0f);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_not Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  inv (2 bytes): bitwise not rx=~rx
//  opcode | (0~3)4bit reg_index
// ============================================================
PX_SYNTAX_FUNCTION(PX_Syntax_IR_inv)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_inv must follow an ir_rx abi");
	reg_index  = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_INV;
	payload[1] = (px_byte)(reg_index & 0x0f);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_inv Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  andl/orl (2 bytes): logical and/or  r_dst=(r_dst&&/||r_src)?1:0
//  opcode | (0~3) dest/src1 index | (4~7) src2 index
// ============================================================
static px_bool PX_Syntax_IR_logical2(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int  dest_index, src2_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"),       "IR_logical2: last must be ir_rx");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_logical2: second-last must be ir_rx");
	dest_index = PX_AbiGetValue_int(psecondlast, "index");
	src2_index = PX_AbiGetValue_int(plast,       "index");
	payload[0] = opcode;
	payload[1] = (px_byte)((dest_index & 0x0f) | ((src2_index & 0x0f) << 4));
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_logical2 Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}
PX_SYNTAX_FUNCTION(PX_Syntax_IR_andl) { return PX_Syntax_IR_logical2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ANDL); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_orl)  { return PX_Syntax_IR_logical2(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ORL);  }

px_int PX_Syntax_Disassemble_not(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	reg_index = payload[1] & 0x0f;
	PX_strset(out, "not r");
	PX_strcatchar(out, (px_char)('0' + reg_index));
	return 2;
}

px_int PX_Syntax_Disassemble_inv(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	reg_index = payload[1] & 0x0f;
	PX_strset(out, "inv r");
	PX_strcatchar(out, (px_char)('0' + reg_index));
	return 2;
}

px_int PX_Syntax_Disassemble_logical2(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, src2_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	dest_index = payload[1] & 0x0f;
	src2_index = (payload[1] >> 4) & 0x0f;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_ANDL: PX_strset(out, "andl "); break;
	case PX_SYNTAX_MACHINE_OPCODE_ORL:  PX_strset(out, "orl ");  break;
	default: PX_strset(out, "unknow"); return 0;
	}
	PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",r"); PX_strcatchar(out, (px_char)('0' + src2_index));
	return 2;
}

// ============================================================
//  set-compare, integer (2 bytes): rd = (rd OP rb)?1:0
//  gt/ge/lt/le/eq/neq (signed) ugt/uge/ult/ule (unsigned)
//  opcode | (0~3) dest index | (4~7) source index
// ============================================================

static px_bool PX_Syntax_IR_setcc(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* psrc = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pdst = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[2];
	px_int  dest_index, src_index;

	if (!psrc || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_setcc: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst, "ir_rx") ||
		!PX_Syntax_CheckAbiName(psrc, "ir_rx"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_setcc: operands must be ir_rx");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst, "index");
	src_index = PX_AbiGetValue_int(psrc, "index");

	payload[0] = opcode;
	payload[1] = (px_byte)((dest_index & 0x0f) | ((src_index & 0x0f) << 4));

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_setcc Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  set-compare, float (3 bytes): rd = (fa OP fb)?1:0
//  fgt/fge/flt/fle/feq/fneq -- dest is an integer register,
//  both sources are float registers
// ============================================================

static px_bool PX_Syntax_IR_setcc_f(PX_Syntax* pSyntax, px_byte opcode)
{
	// Stack (from top): src2(fx), src1(fx), dest(rx)
	px_abi* psrc2 = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psrc1 = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_int  count = PX_Syntax_GetAbiCount(pSyntax);
	px_abi* pdst  = PX_Syntax_GetAbiByIndex(pSyntax, count - 3);
	px_byte payload[3];
	px_int  dest_index, src1_index, src2_index;

	if (!psrc2 || !psrc1 || !pdst)
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_setcc_f: not enough abi on stack");
		return PX_FALSE;
	}
	if (!PX_Syntax_CheckAbiName(pdst,  "ir_rx") ||
	    !PX_Syntax_CheckAbiName(psrc1, "ir_fx") ||
	    !PX_Syntax_CheckAbiName(psrc2, "ir_fx"))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:IR_setcc_f: operands must be ir_rx,ir_fx,ir_fx");
		return PX_FALSE;
	}
	dest_index = PX_AbiGetValue_int(pdst,  "index");
	src1_index = PX_AbiGetValue_int(psrc1, "index");
	src2_index = PX_AbiGetValue_int(psrc2, "index");

	payload[0] = opcode;
	payload[1] = (px_byte)((dest_index & 0x0f) | ((src1_index & 0x0f) << 4));
	payload[2] = (px_byte)(src2_index & 0x0f);

	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 3))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_setcc_f Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_gt)   { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_GT);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_ge)   { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_GE);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_lt)   { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_LT);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_le)   { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_LE);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_eq)   { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_EQ);   }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_neq)  { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_NEQ);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_ugt)  { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_UGT);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_uge)  { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_UGE);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_ult)  { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ULT);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_ule)  { return PX_Syntax_IR_setcc(pSyntax, PX_SYNTAX_MACHINE_OPCODE_ULE);  }

PX_SYNTAX_FUNCTION(PX_Syntax_IR_fgt)  { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FGT);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fge)  { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FGE);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_flt)  { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FLT);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fle)  { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FLE);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_feq)  { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FEQ);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_fneq) { return PX_Syntax_IR_setcc_f(pSyntax, PX_SYNTAX_MACHINE_OPCODE_FNEQ); }

static const px_char* PX_Syntax_IR_setcc_mnemonic(px_byte opcode)
{
	switch (opcode)
	{
	case PX_SYNTAX_MACHINE_OPCODE_GT:   return "gt ";
	case PX_SYNTAX_MACHINE_OPCODE_GE:   return "ge ";
	case PX_SYNTAX_MACHINE_OPCODE_LT:   return "lt ";
	case PX_SYNTAX_MACHINE_OPCODE_LE:   return "le ";
	case PX_SYNTAX_MACHINE_OPCODE_EQ:   return "eq ";
	case PX_SYNTAX_MACHINE_OPCODE_NEQ:  return "neq ";
	case PX_SYNTAX_MACHINE_OPCODE_UGT:  return "ugt ";
	case PX_SYNTAX_MACHINE_OPCODE_UGE:  return "uge ";
	case PX_SYNTAX_MACHINE_OPCODE_ULT:  return "ult ";
	case PX_SYNTAX_MACHINE_OPCODE_ULE:  return "ule ";
	case PX_SYNTAX_MACHINE_OPCODE_FGT:  return "fgt ";
	case PX_SYNTAX_MACHINE_OPCODE_FGE:  return "fge ";
	case PX_SYNTAX_MACHINE_OPCODE_FLT:  return "flt ";
	case PX_SYNTAX_MACHINE_OPCODE_FLE:  return "fle ";
	case PX_SYNTAX_MACHINE_OPCODE_FEQ:  return "feq ";
	case PX_SYNTAX_MACHINE_OPCODE_FNEQ: return "fneq ";
	default: return PX_NULL;
	}
}

px_int PX_Syntax_Disassemble_setcc(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, src_index;
	const px_char* mnemonic;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	mnemonic = PX_Syntax_IR_setcc_mnemonic(payload[0]);
	if (!mnemonic) { PX_strset(out, "unknow"); return 0; }
	dest_index = payload[1] & 0x0f;
	src_index = (payload[1] >> 4) & 0x0f;
	PX_strset(out, mnemonic);
	PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",r"); PX_strcatchar(out, (px_char)('0' + src_index));
	return 2;
}

px_int PX_Syntax_Disassemble_setcc_f(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int dest_index, src1_index, src2_index;
	const px_char* mnemonic;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 3) return 0;
	mnemonic = PX_Syntax_IR_setcc_mnemonic(payload[0]);
	if (!mnemonic) { PX_strset(out, "unknow"); return 0; }
	dest_index = payload[1] & 0x0f;
	src1_index = (payload[1] >> 4) & 0x0f;
	src2_index = payload[2] & 0x0f;
	PX_strset(out, mnemonic);
	PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + dest_index));
	PX_strcat(out, ",f"); PX_strcatchar(out, (px_char)('0' + src1_index));
	PX_strcat(out, ",f"); PX_strcatchar(out, (px_char)('0' + src2_index));
	return 3;
}

// ============================================================
//  jmp (5 bytes): opcode + 32-bit label address
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_jmp_addr)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[5];
	px_dword addr;
	const px_char* pvalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_jmp_addr must follow a const_int abi");
	pvalue     = PX_AbiGet_string(plast, "value");
	addr       = (px_dword)PX_atoi(pvalue);
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_JMP;
	*(px_dword*)(payload + 1) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 5))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_jmp_addr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  jmp  label-name variant (5 bytes)
//  scan 1: emit opcode + 0 (placeholder)
//  scan 2: emit opcode + resolved label address
// ============================================================

// Resolve a label name to its text address. Returns PX_FALSE and terminates on
// an unresolved label during the TEXT scan; yields 0 during the LABEL scan.
static px_bool PX_Syntax_IR_ResolveLabel(PX_Syntax* pSyntax, const px_char label_name[], px_dword* paddress)
{
	px_abi* pscope;
	px_char label_key[128] = "labels.";
	px_int scan_mode;
	*paddress = 0;
	pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	PX_ASSERTIFX(!pscope, "IR_ResolveLabel: no scope abi found");
	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_TEXT)
	{
		px_dword* paddr;
		PX_strcat(label_key, label_name);
		paddr = PX_AbiGet_dword(pscope, label_key);
		if (!paddr)
		{
			px_char errmsg[256];
			PX_sprintf1(errmsg, sizeof(errmsg), "ast:error:IR_ResolveLabel label '%1' not found", PX_STRINGFORMAT_STRING(label_name));
			PX_Syntax_Terminate(pSyntax, errmsg);
			return PX_FALSE;
		}
		*paddress = *paddr;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_jmp_label)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[5];
	px_dword addr = 0;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_jmp_label must follow an identifier abi");
	if (!PX_Syntax_IR_ResolveLabel(pSyntax, PX_AbiGet_string(plast, "value"), &addr)) return PX_FALSE;
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_JMP;
	*(px_dword*)(payload + 1) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 5))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_jmp_label Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

// ============================================================
//  jz / jnz  rx,<label|addr>  (6 bytes)
//  opcode | (0~3) test register index | 32-bit label address
// ============================================================

static px_bool PX_Syntax_IR_jzx(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[6];
	px_dword addr;
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"),      "IR_jzx: last must be const_int");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"),    "IR_jzx: second-last must be ir_rx");
	reg_index  = PX_AbiGetValue_int(psecondlast, "index");
	addr       = (px_dword)PX_atoi(PX_AbiGet_string(plast, "value"));
	payload[0] = opcode;
	payload[1] = (px_byte)(reg_index & 0x0f);
	*(px_dword*)(payload + 2) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_jzx Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

static px_bool PX_Syntax_IR_jzx_label(PX_Syntax* pSyntax, px_byte opcode)
{
	px_abi* plast       = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* psecondlast = PX_Syntax_GetSecondLastAbi(pSyntax);
	px_byte payload[6];
	px_dword addr = 0;
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"),  "IR_jzx_label: last must be identifier");
	PX_ASSERTIFX(!PX_Syntax_CheckSecondLastAbiName(pSyntax, "ir_rx"), "IR_jzx_label: second-last must be ir_rx");
	reg_index = PX_AbiGetValue_int(psecondlast, "index");
	if (!PX_Syntax_IR_ResolveLabel(pSyntax, PX_AbiGet_string(plast, "value"), &addr)) return PX_FALSE;
	payload[0] = opcode;
	payload[1] = (px_byte)(reg_index & 0x0f);
	*(px_dword*)(payload + 2) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 6))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_jzx_label Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_jz_addr)   { return PX_Syntax_IR_jzx(pSyntax, PX_SYNTAX_MACHINE_OPCODE_JZ);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_jnz_addr)  { return PX_Syntax_IR_jzx(pSyntax, PX_SYNTAX_MACHINE_OPCODE_JNZ); }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_jz_label)  { return PX_Syntax_IR_jzx_label(pSyntax, PX_SYNTAX_MACHINE_OPCODE_JZ);  }
PX_SYNTAX_FUNCTION(PX_Syntax_IR_jnz_label) { return PX_Syntax_IR_jzx_label(pSyntax, PX_SYNTAX_MACHINE_OPCODE_JNZ); }

px_int PX_Syntax_Disassemble_jmp(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_dword addr;
	px_char temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 5) return 0;
	if (payload[0] != PX_SYNTAX_MACHINE_OPCODE_JMP) { PX_strset(out, "unknow"); return 0; }
	addr = *(px_dword*)(payload + 1);
	PX_strset(out, "jmp ");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(addr));
	PX_strcat(out, temp);
	return 5;
}

px_int PX_Syntax_Disassemble_jzx(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_dword addr;
	px_int reg_index;
	px_char temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 6) return 0;
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_JZ:  PX_strset(out, "jz r");  break;
	case PX_SYNTAX_MACHINE_OPCODE_JNZ: PX_strset(out, "jnz r"); break;
	default: PX_strset(out, "unknow"); return 0;
	}
	reg_index = payload[1] & 0x0f;
	addr = *(px_dword*)(payload + 2);
	PX_strcatchar(out, (px_char)('0' + reg_index));
	PX_strcat(out, ",");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(addr));
	PX_strcat(out, temp);
	return 6;
}

// ============================================================
//  call (5 bytes)
//  byte0: opcode
//  bytes1-4: 32-bit address
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_call_addr)
{
	// call <const_int>  -- direct call to address
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[5];
	px_dword addr;
	const px_char* pvalue;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "const_int"), "IR_call_label must follow a const_int abi");
	pvalue     = PX_AbiGet_string(plast, "value");
	addr       = (px_dword)PX_atoi(pvalue);
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_CALL;
	*(px_dword*)(payload + 1) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 5))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_call_label Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_callr)
{
	// callr <ir_rx>  -- indirect call via register (2 bytes)
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_callr must follow an ir_rx abi");
	reg_index  = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_CALLR;
	payload[1] = (px_byte)(reg_index & 0xff);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_callr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}


PX_SYNTAX_FUNCTION(PX_Syntax_IR_jmpr)
{
	// jmpr <ir_rx>  -- indirect jump via register (2 bytes)
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_byte payload[2];
	px_int reg_index;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "ir_rx"), "IR_jmpr must follow an ir_rx abi");
	reg_index  = PX_AbiGetValue_int(plast, "index");
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_JMPR;
	payload[1] = (px_byte)(reg_index & 0xff);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 2))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_jmpr Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_call_label)
{
	px_abi* plast = PX_Syntax_GetLastAbi(pSyntax);
	px_abi* pscope;
	px_byte payload[5];
	px_dword addr = 0;
	px_string label_key;
	const px_char* label_name;
	px_int scan_mode;
	PX_ASSERTIFX(!PX_Syntax_CheckLastAbiName(pSyntax, "identifier"), "IR_call_label_name must follow an identifier abi");
	label_name = PX_AbiGet_string(plast, "value");
	pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
	PX_ASSERTIFX(!pscope, "IR_call_label: no scope abi found");
	scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
	if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_TEXT)
	{
		px_dword* paddr;
		if (!PX_StringInitializeFormat1(pSyntax->mp,&label_key, "labels.%1",PX_STRINGFORMAT_STRING(label_name)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_call_label out of memory.");
			return PX_FALSE;
		}
		paddr = PX_AbiGet_dword(pscope, PX_StringGetText(&label_key));
		if (!paddr)
		{
			px_string errmsg;
			if(!PX_StringInitializeFormat1(pSyntax->mp,&errmsg,  "ast:error:IR_call_label label '%1' not found", PX_STRINGFORMAT_STRING(label_name)))
			{
				PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_call_label out of memory.");
				PX_StringFree(&label_key);
				return PX_FALSE;
			}
			PX_Syntax_Terminate(pSyntax, PX_StringGetText(&errmsg));
			PX_StringFree(&errmsg);
			PX_StringFree(&label_key);
			return PX_FALSE;
		}
		PX_StringFree(&label_key);
		addr = *paddr;
	}
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_CALL;
	*(px_dword*)(payload + 1) = addr;
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 5))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_call_label_name Memory Error");
		return PX_FALSE;
	}
	PX_Syntax_PopAbi(pSyntax);
	return PX_TRUE;
}

px_int PX_Syntax_Disassemble_call(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_dword value;
	px_char temp[32] = { 0 };
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 5) return 0;
	value = *(px_dword*)(payload + 1);
	PX_strset(out, "call ");
	PX_sprintf1(temp, sizeof(temp), "%1", PX_STRINGFORMAT_INT(value));
	PX_strcat(out, temp);
	return 5;
}

px_int PX_Syntax_Disassemble_callr(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	reg_index = payload[1] & 0xff;
	PX_strset(out, "callr ");
	if (reg_index <= 3) { PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + reg_index)); }
	else if (reg_index == 4) PX_strcat(out, "ip");
	else if (reg_index == 5) PX_strcat(out, "sp");
	else if (reg_index == 6) PX_strcat(out, "bp");
	else if (reg_index == 7) PX_strcat(out, "flag");
	else PX_strcat(out, "unknow");
	return 2;
}

px_int PX_Syntax_Disassemble_jmpr(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	px_int reg_index;
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 2) return 0;
	reg_index = payload[1] & 0xff;
	PX_strset(out, "jmpr ");
	if (reg_index <= 3) { PX_strcat(out, "r"); PX_strcatchar(out, (px_char)('0' + reg_index)); }
	else if (reg_index == 4) PX_strcat(out, "ip");
	else if (reg_index == 5) PX_strcat(out, "sp");
	else if (reg_index == 6) PX_strcat(out, "bp");
	else if (reg_index == 7) PX_strcat(out, "flag");
	else PX_strcat(out, "unknow");
	return 2;
}

// ============================================================
//  ret (1 byte)
// ============================================================

PX_SYNTAX_FUNCTION(PX_Syntax_IR_ret)
{
	px_byte payload[1];
	payload[0] = PX_SYNTAX_MACHINE_OPCODE_RET;
	PX_SYNTAX_CALL_FUNCTION(PX_Syntax_TokenRender);
	if (!PX_Syntax_IR_EmitText(pSyntax, payload, 1))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_ret Memory Error");
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_int PX_Syntax_Disassemble_ret(const px_byte* payload, px_int reserved_size, px_char out[32])
{
	PX_memset(out, 0, sizeof(px_char) * 32);
	if (reserved_size < 1) return 0;
	PX_strset(out, "ret");
	return 1;
}


// ============================================================
//  ee disassemble helpers
// ============================================================
static const px_char* PX_Syntax_IR_ee_type_name(px_byte type, px_dword value, px_char buf[32])
{
	switch (type)
	{
	case 0: PX_sprintf1(buf, 32, "%1", PX_STRINGFORMAT_INT(value)); break;
	case 1: { px_float fv = *(px_float*)&value; PX_sprintf1(buf, 32, "%1f", PX_STRINGFORMAT_FLOAT(fv)); } break;
	case 2: PX_sprintf1(buf, 32, "r%1", PX_STRINGFORMAT_INT(value)); break;
	case 3: PX_sprintf1(buf, 32, "f%1", PX_STRINGFORMAT_INT(value)); break;
	case 4: PX_sprintf1(buf, 32, "%1", PX_STRINGFORMAT_INT(value)); break;
	case 5: PX_sprintf1(buf, 32, "bp-%1", PX_STRINGFORMAT_INT(value)); break;
	case 6: PX_sprintf1(buf, 32, "sp+%1", PX_STRINGFORMAT_INT(value)); break;
	case 7: PX_sprintf1(buf, 32, "bp+%1", PX_STRINGFORMAT_INT(value)); break;
	default: PX_strset(buf, "?"); break;
	}
	return buf;
}


px_int PX_Syntax_Disassemble(const px_byte* payload, px_int reserved_size, px_char out_asm[32])
{
	if (reserved_size < 1)
	{
		return 0;
	}
	switch (payload[0])
	{
	case PX_SYNTAX_MACHINE_OPCODE_NOP:
		return PX_Syntax_Disassemble_nop(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_MOVR:
		return PX_Syntax_Disassemble_movr(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_MOVC:
		return PX_Syntax_Disassemble_movc(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_MOVF:
		return PX_Syntax_Disassemble_movf(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_MOVN:
		PX_memset(out_asm, 0, sizeof(px_char) * 32);
		PX_strset(out_asm, "movn");
		return 1;
	case PX_SYNTAX_MACHINE_OPCODE_LOADU8:
	case PX_SYNTAX_MACHINE_OPCODE_LOADU16:
	case PX_SYNTAX_MACHINE_OPCODE_LOADU32:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI8:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI16:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI32:
		return PX_Syntax_Disassemble_loadxx(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_STORE8:
	case PX_SYNTAX_MACHINE_OPCODE_STORE16:
	case PX_SYNTAX_MACHINE_OPCODE_STORE32:
		return PX_Syntax_Disassemble_storexx(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_LOADU8R:
	case PX_SYNTAX_MACHINE_OPCODE_LOADU16R:
	case PX_SYNTAX_MACHINE_OPCODE_LOADU32R:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI8R:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI16R:
	case PX_SYNTAX_MACHINE_OPCODE_LOADI32R:
		return PX_Syntax_Disassemble_loadxxr(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_STORE8R:
	case PX_SYNTAX_MACHINE_OPCODE_STORE16R:
	case PX_SYNTAX_MACHINE_OPCODE_STORE32R:
		return PX_Syntax_Disassemble_storexr(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_STORE8RC:
	case PX_SYNTAX_MACHINE_OPCODE_STORE16RC:
	case PX_SYNTAX_MACHINE_OPCODE_STORE32RC:
		return PX_Syntax_Disassemble_storexrc(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_STORE8C:
	case PX_SYNTAX_MACHINE_OPCODE_STORE16C:
	case PX_SYNTAX_MACHINE_OPCODE_STORE32C:
		return PX_Syntax_Disassemble_storexc(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_F2I:
		return PX_Syntax_Disassemble_f2i(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_F2U:
		return PX_Syntax_Disassemble_f2u(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_I2F:
		return PX_Syntax_Disassemble_i2f(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_U2F:
		return PX_Syntax_Disassemble_u2f(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_PUSH:
		return PX_Syntax_Disassemble_push(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_POP:
		return PX_Syntax_Disassemble_pop(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_POPN:
		return PX_Syntax_Disassemble_popn(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_NEG:
		return PX_Syntax_Disassemble_neg(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_ADD:
	case PX_SYNTAX_MACHINE_OPCODE_SUB:
	case PX_SYNTAX_MACHINE_OPCODE_MUL:
	case PX_SYNTAX_MACHINE_OPCODE_DIV:
	case PX_SYNTAX_MACHINE_OPCODE_IDIV:
	case PX_SYNTAX_MACHINE_OPCODE_MOD:
	case PX_SYNTAX_MACHINE_OPCODE_IMOD:
	case PX_SYNTAX_MACHINE_OPCODE_AND:
	case PX_SYNTAX_MACHINE_OPCODE_OR:
	case PX_SYNTAX_MACHINE_OPCODE_XOR:
	case PX_SYNTAX_MACHINE_OPCODE_SHL:
	case PX_SYNTAX_MACHINE_OPCODE_SHR:
		return PX_Syntax_Disassemble_alu2(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_ADDC:
	case PX_SYNTAX_MACHINE_OPCODE_SUBC:
	case PX_SYNTAX_MACHINE_OPCODE_MULC:
	case PX_SYNTAX_MACHINE_OPCODE_DIVC:
	case PX_SYNTAX_MACHINE_OPCODE_IDIVC:
	case PX_SYNTAX_MACHINE_OPCODE_MODC:
	case PX_SYNTAX_MACHINE_OPCODE_IMODC:
	case PX_SYNTAX_MACHINE_OPCODE_ANDC:
	case PX_SYNTAX_MACHINE_OPCODE_ORC:
	case PX_SYNTAX_MACHINE_OPCODE_XORC:
	case PX_SYNTAX_MACHINE_OPCODE_SHLC:
	case PX_SYNTAX_MACHINE_OPCODE_SHRC:
		return PX_Syntax_Disassemble_alu2ci(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_FNEG:
		return PX_Syntax_Disassemble_fneg(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_FADD:
	case PX_SYNTAX_MACHINE_OPCODE_FSUB:
	case PX_SYNTAX_MACHINE_OPCODE_FMUL:
	case PX_SYNTAX_MACHINE_OPCODE_FDIV:
		return PX_Syntax_Disassemble_fpu2(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_FADDC:
	case PX_SYNTAX_MACHINE_OPCODE_FSUBC:
	case PX_SYNTAX_MACHINE_OPCODE_FMULC:
	case PX_SYNTAX_MACHINE_OPCODE_FDIVC:
		return PX_Syntax_Disassemble_fpu2cf(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_NOT:
		return PX_Syntax_Disassemble_not(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_INV:
		return PX_Syntax_Disassemble_inv(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_ANDL:
	case PX_SYNTAX_MACHINE_OPCODE_ORL:
		return PX_Syntax_Disassemble_logical2(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_GT:
	case PX_SYNTAX_MACHINE_OPCODE_GE:
	case PX_SYNTAX_MACHINE_OPCODE_LT:
	case PX_SYNTAX_MACHINE_OPCODE_LE:
	case PX_SYNTAX_MACHINE_OPCODE_EQ:
	case PX_SYNTAX_MACHINE_OPCODE_NEQ:
	case PX_SYNTAX_MACHINE_OPCODE_UGT:
	case PX_SYNTAX_MACHINE_OPCODE_UGE:
	case PX_SYNTAX_MACHINE_OPCODE_ULT:
	case PX_SYNTAX_MACHINE_OPCODE_ULE:
		return PX_Syntax_Disassemble_setcc(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_FGT:
	case PX_SYNTAX_MACHINE_OPCODE_FGE:
	case PX_SYNTAX_MACHINE_OPCODE_FLT:
	case PX_SYNTAX_MACHINE_OPCODE_FLE:
	case PX_SYNTAX_MACHINE_OPCODE_FEQ:
	case PX_SYNTAX_MACHINE_OPCODE_FNEQ:
		return PX_Syntax_Disassemble_setcc_f(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_JMP:
		return PX_Syntax_Disassemble_jmp(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_JZ:
	case PX_SYNTAX_MACHINE_OPCODE_JNZ:
		return PX_Syntax_Disassemble_jzx(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_CALL:
		return PX_Syntax_Disassemble_call(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_CALLR:
		return PX_Syntax_Disassemble_callr(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_JMPR:
		return PX_Syntax_Disassemble_jmpr(payload, reserved_size, out_asm);
	case PX_SYNTAX_MACHINE_OPCODE_RET:
		return PX_Syntax_Disassemble_ret(payload, reserved_size, out_asm);
	default:
		break;
	}
	return PX_FALSE;
}

//#include "platform/modules/px_file.h"
PX_SYNTAX_FUNCTION(PX_Syntax_IR_next_scan)
{
	px_int scan_mode;

	if (PX_Syntax_IsEndOfSource(pSyntax))
	{
		px_int module_base_addr = PX_Syntax_IR_GetMP(pSyntax);
		px_abi* pscope = PX_Syntax_GetAbiFromForward(pSyntax, "scope");
		PX_ASSERTIFX(!pscope, "IR_next_scan: no scope abi found");
		scan_mode = PX_AbiGetValue_int(pscope, "scan_mode");
		if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_LABEL)
		{
			PX_AbiSet_int(pscope, "scan_mode", PX_SYNTAX_IR_SCAN_MODE_RDATA);
		}
		else if (scan_mode == PX_SYNTAX_IR_SCAN_MODE_RDATA)
		{
			PX_AbiSet_int(pscope, "scan_mode", PX_SYNTAX_IR_SCAN_MODE_TEXT);
			PX_AbiSet_int(pscope, "rp", module_base_addr+ PX_AbiGetValue_int(pscope, "text_length"));
			PX_AbiSet_int(pscope, "gp", module_base_addr+PX_AbiGetValue_int(pscope, "text_length") + PX_AbiGetValue_int(pscope, "rdata_length"));
		}
		else
		{
			return PX_FALSE;
		}
		PX_SyntaxLexer_Reset(&pSyntax->reg_syntaxlexer);
		PX_Syntax_ClearSourceDescription(pSyntax);
		PX_StringClear(&pSyntax->message);
		if (!PX_Syntax_CallSourceIndex(pSyntax, 0, "_"))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_IR_next_scan CallSourceIndex failed");
		}
		return PX_TRUE;
	}
	return PX_FALSE;
}


// ============================================================
//  ee_cmp (12 bytes):
//  [opcode][type_src1][type_src2][pad(1B)][src1 4B][src2 4B]
//  abi stack (top to bottom): src2, src1
// ============================================================
// ============================================================
//  ee_fcmp (12 bytes):
//  [opcode][type_src1][type_src2][pad(1B)][src1 4B][src2 4B]
//  abi stack (top to bottom): src2, src1
// ============================================================
PX_SYNTAX_FUNCTION(PX_Syntax_IR_display_mp)
{
	px_int mp = PX_Syntax_IR_GetMP(pSyntax);

	px_int begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	px_int begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	px_int end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	px_int end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);

	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexemeLine(pSyntax);
	pSyntax->reg_expr_source_index = begin_source_index;

	//color
	if (PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 127, 166, 89), 0))
	{
		px_char payload[32] = "modulebase pointer:";
		PX_strcat(payload, PX_itos(mp, 10).data);
		PX_Syntax_SetLastMapInfo(pSyntax, begin_source_index, payload);

	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_display_gp)
{
	px_int gp = PX_Syntax_IR_GetGP(pSyntax);

	px_int begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	px_int begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	px_int end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	px_int end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);

	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexemeLine(pSyntax);
	pSyntax->reg_expr_source_index = begin_source_index;

	//color
	if (PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 127, 166, 89), 0))
	{
		px_char payload[32] = "global pointer:";
		PX_strcat(payload, PX_itos(gp,10).data);
		PX_Syntax_SetLastMapInfo(pSyntax, begin_source_index, payload);

	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_display_rp)
{
	px_int rp = PX_Syntax_IR_GetRP(pSyntax);
	px_int begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	px_int begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	px_int end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	px_int end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexemeLine(pSyntax);
	pSyntax->reg_expr_source_index = begin_source_index;
	//color
	if (PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 127, 166, 89), 0))
	{
		px_char payload[32] = "resources pointer:";
		PX_strcat(payload, PX_itos(rp, 10).data);
		PX_Syntax_SetLastMapInfo(pSyntax, begin_source_index, payload);
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_display_tp)
{
	px_int tp = PX_Syntax_IR_GetTP(pSyntax);
	px_int begin_source_index = PX_Syntax_GetCurrentLexemeBeginSourceIndex(pSyntax);
	px_int begin = PX_Syntax_GetCurrentLexemeBegin(pSyntax);
	px_int end_source_index = PX_Syntax_GetCurrentLexemeEndSourceIndex(pSyntax);
	px_int end = PX_Syntax_GetCurrentLexemeEnd(pSyntax);
	pSyntax->reg_expr_begin_line = PX_Syntax_GetCurrentLexemeLine(pSyntax);
	pSyntax->reg_expr_source_index = begin_source_index;
	//color
	if (PX_Syntax_NewStaticMapToken(pSyntax, begin_source_index, begin, end_source_index, end, PX_COLOR(255, 127, 166, 89), 0))
	{
		px_char payload[32] = "thread pointer:";
		PX_strcat(payload, PX_itos(tp, 10).data);
		PX_Syntax_SetLastMapInfo(pSyntax, begin_source_index, payload);
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_IR_no_rx_fx)
{
	PX_Syntax_AstReturn(pSyntax, past, "identifier should not be rx or fx.");
	return PX_FALSE;
}

px_bool PX_Syntax_load_ir(PX_Syntax* pSyntax)
{
	PX_Syntax_load_const_int(pSyntax);
	PX_Syntax_load_const_int_list(pSyntax);
	PX_Syntax_load_const_float(pSyntax);
	PX_Syntax_load_const_string(pSyntax);
	PX_Syntax_load_const_string_list(pSyntax);
	PX_Syntax_load_keyword(pSyntax);
	PX_Syntax_load_identifier(pSyntax);
	PX_Syntax_load_eof(pSyntax);

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_init = *", 0, PX_Syntax_ir_init, 0))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_skip_newline_spacer = *", 0, PX_Syntax_ir_skip_newline_spacer, PX_NULL))
		return PX_FALSE;
	
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_loc = ';' '@loc'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_loc = ';' '@loc' const_int const_int", 0, PX_Syntax_ir_comment_loc, PX_NULL))//;@loc source_index source offset
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "comment = ir_loc", 0, PX_NULL, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "comment = *", 0, PX_Syntax_ir_comment, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label = 'import' const_string", 0, PX_Syntax_ir_import, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label = 'export' identifier ':'", 0, PX_Syntax_export_ir_label, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label = identifier ':'", 0, PX_Syntax_ir_label, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'mp'", 0, PX_Syntax_IR_display_mp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'gp'", 0, PX_Syntax_IR_display_gp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'tp'", 0, PX_Syntax_IR_display_tp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'rp'", 0, PX_Syntax_IR_display_rp, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'mp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_mp_const, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'gp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_gp_const, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'tp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_tp_const, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'rp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_rp_const, PX_NULL))
		return PX_FALSE;
	
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'mp' *", 0, PX_Syntax_IR_ir_mp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'gp' *", 0, PX_Syntax_IR_ir_gp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'tp' *", 0, PX_Syntax_IR_ir_tp, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const = 'rp' *", 0, PX_Syntax_IR_ir_rp, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const =  const_int", 0, 0, PX_NULL))
		return PX_FALSE;

	
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_rx = identifier", 0,PX_Syntax_IR_rx, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_fx = identifier", 0, PX_Syntax_IR_fx, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label_addr = ir_rx", 0, PX_Syntax_IR_no_rx_fx, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label_addr = ir_fx", 0, PX_Syntax_IR_no_rx_fx, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_label_addr = identifier", 0, PX_Syntax_IR_ir_label_addr, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const_addr = 'mp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_mp_const, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const_addr = 'gp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_gp_const, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const_addr = 'tp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_tp_const, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const_addr = 'rp' '+' const_unsigned_int", 0, PX_Syntax_IR_ir_rp_const, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_const_addr = ir_label_addr", 0, PX_Syntax_IR_ir_rp_const, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_addr = 'bp' '-' const_unsigned_int", 0, PX_Syntax_IR_local_addr, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_addr = 'bp' '+' const_unsigned_int", 0, PX_Syntax_IR_param_addr, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir_addr = 'sp' '+' const_unsigned_int", 0, PX_Syntax_IR_stack_addr, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir_addr = ir_const_addr", 0, PX_Syntax_IR_ir_const_addr, PX_NULL))
		return PX_FALSE;


	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = ir_skip_newline_spacer", 0, 0, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = comment", 0, 0, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = ir_label", 0, 0, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'rdata' identifier ':' const_string", 0, PX_Syntax_IR_rdata, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu8'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu16'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu32'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load8'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load16'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load32'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi8'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi16'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi32'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store8'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store16'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store32'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'push'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'pop'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	//PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'popn'", 0, PX_Syntax_IR_opcode_render_color, PX_NULL);
	//PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'nop'", 0, PX_Syntax_IR_opcode_render_color, PX_NULL);
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	//PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'movn'", 0, PX_Syntax_IR_opcode_render_color, PX_NULL);
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'f2i'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'f2u'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'i2f'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'u2f'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	//PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ff2f'", 0, PX_Syntax_IR_opcode_render_color, PX_NULL);
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'neg'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'add'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'sub'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mul'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'div'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'idiv'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mod'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'imod'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fneg'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fadd'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fsub'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fmul'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fdiv'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'not'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'inv'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'andl'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'orl'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'and'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'or'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'xor'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shl'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shr'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'gt'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ge'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'lt'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'le'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'eq'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'neq'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ugt'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'uge'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ult'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ule'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fgt'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fge'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'flt'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fle'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'feq'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fneq'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jmp'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jz'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jnz'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'call'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'callr'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jmpr'", 0, PX_Syntax_TokenRender, PX_NULL))
		return PX_FALSE;
	//PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ret'", 0, PX_Syntax_IR_opcode_render_color, PX_NULL);

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'push' ir_rx ", 0, PX_Syntax_IR_push, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'pop' ir_rx ", 0, PX_Syntax_IR_pop, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'popn' ", 0, PX_Syntax_IR_popn,  PX_NULL))
		return PX_FALSE;


	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'nop' ", 0, PX_Syntax_IR_nop, PX_NULL))
		return PX_FALSE;

	/* movr: dest and src can each be ir_rx or ir_fx -- match both variants */

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_movc, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_movr, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_rx ',' ir_fx '\n'", 0, PX_Syntax_IR_movr, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_movc, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_fx ',' ir_rx '\n'", 0, PX_Syntax_IR_movr, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mov' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_movr, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'movf' ir_rx ',' const_float '\n'", 0, PX_Syntax_IR_movf, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'movf' ir_fx ',' const_float '\n'", 0, PX_Syntax_IR_movf, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'movn' '\n'", 0, PX_Syntax_IR_movn, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'f2i' ir_rx ',' ir_fx '\n'", 0, PX_Syntax_IR_f2i, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'f2u' ir_rx ',' ir_fx '\n'", 0, PX_Syntax_IR_f2u, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'i2f' ir_fx ',' ir_rx '\n'", 0, PX_Syntax_IR_i2f, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'u2f' ir_fx ',' ir_rx '\n'", 0, PX_Syntax_IR_u2f, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu8' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu8, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu16' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu16, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu32' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu32, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load8' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu8, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load16' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu16, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load32' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadu32, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi8' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadi8, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi16' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadi16, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi32' ir_rx ',' ir_addr '\n'", 0, PX_Syntax_IR_loadi32, PX_NULL))
		return PX_FALSE;

	// Register load uses the same register as address and destination.
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu8'  ir_rx '\n'", 0, PX_Syntax_IR_loadu8r,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu16' ir_rx '\n'", 0, PX_Syntax_IR_loadu16r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadu32' ir_rx '\n'", 0, PX_Syntax_IR_loadu32r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load8'   ir_rx '\n'", 0, PX_Syntax_IR_loadu8r,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load16'  ir_rx '\n'", 0, PX_Syntax_IR_loadu16r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'load32'  ir_rx '\n'", 0, PX_Syntax_IR_loadu32r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi8'  ir_rx '\n'", 0, PX_Syntax_IR_loadi8r,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi16' ir_rx '\n'", 0, PX_Syntax_IR_loadi16r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'loadi32' ir_rx '\n'", 0, PX_Syntax_IR_loadi32r, PX_NULL))
		return PX_FALSE;

	// Register store: store* addr_reg, val_reg
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store8'  ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_store8r,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store16' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_store16r, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store32' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_store32r, PX_NULL))
		return PX_FALSE;

	// storexrc share store mnemonic: store* addr_reg, const
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store8'  ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_store8rc,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store16' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_store16rc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store32' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_store32rc, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store8' ir_addr ',' ir_rx '\n'", 0, PX_Syntax_IR_store8, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store16' ir_addr ',' ir_rx '\n'", 0, PX_Syntax_IR_store16, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store32' ir_addr ',' ir_rx '\n'", 0, PX_Syntax_IR_store32, PX_NULL))
		return PX_FALSE;

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store8'  ir_addr ',' ir_const '\n'", 0, PX_Syntax_IR_store8c, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store16' ir_addr ',' ir_const '\n'", 0, PX_Syntax_IR_store16c, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'store32' ir_addr ',' ir_const '\n'", 0, PX_Syntax_IR_store32c, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'neg' ir_rx '\n'", 0, PX_Syntax_IR_neg, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'add' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_add, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'add' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_addc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'sub' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_sub, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'sub' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_subc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mul' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_mul, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mul' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_mulc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'div' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_div, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'div' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_divc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'idiv' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_idiv, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'idiv' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_idivc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mod' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_mod, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'mod' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_modc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'imod' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_imod, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'imod' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_imodc, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fneg' ir_fx '\n'", 0, PX_Syntax_IR_fneg, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fadd' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fadd, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fadd' ir_fx ',' const_float '\n'", 0, PX_Syntax_IR_faddc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fsub' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fsub, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fsub' ir_fx ',' const_float '\n'", 0, PX_Syntax_IR_fsubc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fmul' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fmul, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fmul' ir_fx ',' const_float '\n'", 0, PX_Syntax_IR_fmulc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fdiv' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fdiv, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fdiv' ir_fx ',' const_float '\n'", 0, PX_Syntax_IR_fdivc, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'not' ir_rx '\n'", 0, PX_Syntax_IR_not, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'inv' ir_rx '\n'", 0, PX_Syntax_IR_inv, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'andl' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_andl, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'orl' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_orl, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'and' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_and, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'and' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_andc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'or' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_or, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'or' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_orc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'xor' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_xor, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'xor' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_xorc, PX_NULL))
		return PX_FALSE;
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shl' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_shl, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shl' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_shlc, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shr' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_shr, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'shr' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_shrc, PX_NULL))
		return PX_FALSE;


	// Integer comparisons use the destination as the implicit left operand.
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'gt' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_gt, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ge' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_ge, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'lt' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_lt, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'le' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_le, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'eq' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_eq, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'neq' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_neq, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ugt' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_ugt, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'uge' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_uge, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ult' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_ult, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ule' ir_rx ',' ir_rx '\n'", 0, PX_Syntax_IR_ule, PX_NULL))
		return PX_FALSE;

	// float set-compare: destination is an integer register, both sources are float
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fgt' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fgt, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fge' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fge, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'flt' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_flt, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fle' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fle, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'feq' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_feq, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'fneq' ir_rx ',' ir_fx ',' ir_fx '\n'", 0, PX_Syntax_IR_fneq, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jmp' identifier '\n'", 0, PX_Syntax_IR_jmp_label, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jz'  ir_rx ',' identifier '\n'", 0, PX_Syntax_IR_jz_label,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jnz' ir_rx ',' identifier '\n'", 0, PX_Syntax_IR_jnz_label, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jmp' ir_const '\n'", 0, PX_Syntax_IR_jmp_addr, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jz'  ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_jz_addr,  PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jnz' ir_rx ',' ir_const '\n'", 0, PX_Syntax_IR_jnz_addr, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'call' identifier '\n'", 0, PX_Syntax_IR_call_label, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'call' ir_const '\n'", 0, PX_Syntax_IR_call_addr,  PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'callr' ir_rx '\n'",     0, PX_Syntax_IR_callr,   PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'jmp' ir_rx '\n'", 0, PX_Syntax_IR_jmpr, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "ir = 'ret' '\n'", 0, PX_Syntax_IR_ret, PX_NULL))
		return PX_FALSE;

	if(!PX_Syntax_Parse_PEBNF(pSyntax, "next_scan = *", 0, PX_Syntax_IR_next_scan, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "irs = ir ---", 0, 0, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "irs = next_scan ---", 0, 0, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "irs = eof", 0, 0, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "irs = * ", 0, PX_Syntax_IR_error, PX_NULL))
		return PX_FALSE;
	if(!PX_Syntax_Parse_PEBNF(pSyntax, "_ = ir_init irs", 0, 0, PX_NULL))
		return PX_FALSE;
	return PX_TRUE;
}
