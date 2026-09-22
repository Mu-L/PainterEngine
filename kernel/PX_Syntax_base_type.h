#ifndef PX_SYNTAX_BASE_TYPE_H
#define PX_SYNTAX_BASE_TYPE_H
#include "PX_Syntax_base_opcode.h"
#include "PX_Syntax_base_operate.h"

const px_char* PX_Syntax_GetPointerBaseType(const px_char type_name[]);
const px_char* PX_Syntax_GetArrayBaseType(const px_char type_name[]);
px_int PX_Syntax_GetArrayDimension(const px_char type_name[]);
px_int PX_Syntax_GetTypePointerLevel(const px_char type_name[]);
px_int PX_Syntax_GetArraySize(PX_Syntax* pSyntax, const px_char array_type_name[]);
px_int PX_Syntax_array_parse_count(const px_char payload[]);
px_bool PX_Syntax_SetTypePointerLevel(px_string* ptype, px_int pointerlevel);

px_bool PX_Syntax_load_base_type(PX_Syntax* pSyntax);
px_bool PX_Syntax_init_base_type(PX_Syntax* pSyntax);


#endif