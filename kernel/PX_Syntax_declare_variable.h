#ifndef PX_SYNTAX_DECLARE_TYPE_H
#define PX_SYNTAX_DECLARE_TYPE_H
#include "PX_Syntax.h"
#include "PX_Syntax_base_type.h"
px_bool PX_Syntax_load_declare_variable(PX_Syntax* pSyntax);
px_bool PX_Syntax_new_declare_variable_token(struct _PX_Syntax* pSyntax, const px_char* final_from);
#endif
