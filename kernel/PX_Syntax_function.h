
#ifndef PX_SYNTAX_FUNCTION_H
#define PX_SYNTAX_FUNCTION_H
#include "PX_Syntax.h"

// function_block --- function_return_type(abi type)
//				  |
//				   --- function_name(identifier abi)
//				  |
//                 --- param_count(int)
// 				  |
//				   --- [](parameters abi variable)

// call_function   --- value(string)
//                |
//                 --- check_param_count(int)
//				  |
//				   --- function_block(abi function_block)

px_bool PX_Syntax_load_function(PX_Syntax* pSyntax);
#endif // !PX_SYNTAX_FUNCTION_H
