#ifndef PX_SYNTAX_BASE_OPERATE_H
#define PX_SYNTAX_BASE_OPERATE_H
#include "PX_Syntax.h"
#include "PX_Syntax_base_type.h"

px_bool PX_Syntax_NewBeginEndOperate(PX_Syntax* pSyntax, px_int opcode_index, PX_Syntax_Operate_Function opfun);
px_bool PX_Syntax_NewUnaryPrefixOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], PX_Syntax_Operate_Function opfun);
px_bool PX_Syntax_NewUnarySuffixOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], PX_Syntax_Operate_Function opfun);
px_bool PX_Syntax_NewBinaryOperate(PX_Syntax* pSyntax, px_int opcode_index, const px_char type1[], const px_char type2[], PX_Syntax_Operate_Function opfun);

//ix,ux,fx--->copy value to register
//struct,union----push value to stack and copy address to register
//array ----copy address to register
//pointer----copy value to register
//reference --- 
px_bool PX_Syntax_GetStructMemberAbi(PX_Syntax* pSyntax, px_abi* pir_abi, const px_char struct_type[], const px_char member_name[]);

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_positive_negative_ixfx);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_opcode_ixfx_assign_ixfx);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_opcode_pointer_assign);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_add_sub_mul_div);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_add_sub_ix);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_array_add_ix);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_mod);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_shl_shr);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_bitand_bitor_bitxor);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ixfx_cmp);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_logical_and_or);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_logical_not);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_bitwise_not);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_inc);
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_ix_dec);

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_struct_member);//.
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_struct_offset);//->

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_address_of);//&
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_dereference);//*

PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_pointer_offset);//pointer[]
PX_SYNTAX_OPERATE_FUNCTION(PX_Syntax_operate_array_dereference_offset);//array[]


PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_auto);
PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_ix_to_fx);
PX_SYNTAX_TYPE_CONVERT_FUNCTION(PX_Syntax_convert_fx_to_ix);
#endif
