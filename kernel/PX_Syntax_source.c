#include "PX_Syntax_source.h"


PX_SYNTAX_FUNCTION(PX_Syntax_source_error)
{
	PX_Syntax_Terminate(pSyntax, "ast:error:Unknow content");
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_bootloader)
{
	if (!PX_Syntax_load_comment(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_comment failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_include(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_include failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_int(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_int failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_const_string(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_string failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_const_float(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_float failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_keyword(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_keyword failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_bcontainer(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_bcontainer failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_identifier(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_identifier failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_numeric(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_numeric failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_tuple(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_tuple failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_load_const_string_list(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_string_list failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_int_list(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_int_list failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_float_list(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_float_list failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_numeric_list(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_numeric_list failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_tuple_list(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_tuple_list failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_const_set(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_const_set failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_bcontainer(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_bcontainer failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_define(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_define failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_declare_prefix(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_declare_prefix failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_declare_token_prefix(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_declare_token_prefix failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_declare_token_suffix(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_declare_token_suffix failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_define_struct(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_define_struct failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_return(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_return failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_function(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_function failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_if(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_if failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_while(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_while failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_do_while(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_do_while failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_for(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_for failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_switch(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_switch failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_break_continue(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_break_continue failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_declare_variable(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_declare_variable failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_eof(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_eof failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_expr(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_expr failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_goto(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_goto failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_scope_block(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_scope failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_block(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_block failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_base_type(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_base_type failed.");
		return PX_FALSE;
	}

	if (!PX_Syntax_load_typedef(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_load_typedef failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_source_begin)
{
	if (!PX_Syntax_EnterBaseScope(pSyntax))
	{
		PX_ASSERTX("Error:sources_enter scope failed.");
		PX_Syntax_Terminate(pSyntax, "runtime:error:sources_enter scope failed.");
		return PX_FALSE;
	}
	if (!PX_Syntax_init_base_type(pSyntax))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:source init PX_Syntax_init_base_type failed.");
		return PX_FALSE;
	}
	return PX_TRUE;
}

PX_SYNTAX_FUNCTION(PX_Syntax_source_leave)
{

	if (PX_Syntax_LeaveScope(pSyntax))
	{
		return PX_TRUE;
	}
	return PX_FALSE;

}

PX_SYNTAX_FUNCTION(PX_Syntax_done)
{
	px_abi* pscopeabi = PX_Syntax_GetLastAbi(pSyntax);
	px_string op_ir;
	PX_Syntax_Message(pSyntax, "ast:compiler:done\n");
	if (!PX_StringInitialize(pSyntax->mp, &op_ir))
	{
		PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done Memory Error");
		return PX_FALSE;
	}
	if (PX_AbiExist_Type(pscopeabi, "ir", PX_ABI_TYPE_STRING))
	{
		const px_char* pir = PX_AbiGetValue_string(pscopeabi, "ir");
		if (!PX_StringSet(&op_ir, pir))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done Memory Error");
			PX_StringFree(&op_ir);
			return PX_FALSE;
		}
		if (!PX_Syntax_IR_optimize_pass1(&op_ir))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done IR optimize failed");
			return PX_FALSE;
		}
		if (!PX_AbiSet_string(pscopeabi, "ir", PX_StringGetText(&op_ir)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done Memory Error");
			PX_StringFree(&op_ir);
			return PX_FALSE;
		}
	}
	if (PX_AbiExist_Type(pscopeabi, "ir_library", PX_ABI_TYPE_STRING))
	{
		const px_char* pir = PX_AbiGetValue_string(pscopeabi, "ir_library");
		if (!PX_StringSet(&op_ir, pir))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done Memory Error");
			PX_StringFree(&op_ir);
			return PX_FALSE;
		}
		if (!PX_Syntax_IR_optimize_pass1(&op_ir))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done IR optimize failed");
			return PX_FALSE;
		}
		if (!PX_AbiSet_string(pscopeabi, "ir_library", PX_StringGetText(&op_ir)))
		{
			PX_Syntax_Terminate(pSyntax, "runtime:error:PX_Syntax_done Memory Error");
			PX_StringFree(&op_ir);
			return PX_FALSE;
		}
	}
	PX_StringFree(&op_ir);
	return PX_TRUE;
}


px_bool PX_Syntax_load_sources(PX_Syntax* pSyntax)
{
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "source", PX_Syntax_source_begin, PX_Syntax_source_leave, 0))
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "source= blocks", 0, 0, 0))
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "source= eof", 0, 0, 0))
	{
		return PX_FALSE;
	}
	if (!PX_Syntax_Parse_PEBNF(pSyntax, "source = *", 0, PX_Syntax_source_error, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "_", PX_Syntax_bootloader, PX_Syntax_done, 0))
	{
		return PX_FALSE;
	}

	if (!PX_Syntax_Parse_PEBNF(pSyntax, "_ = source", 0, 0, 0))
	{
		return PX_FALSE;
	}
    return PX_TRUE;
}
