#ifndef PX_SYNTAX_H
#define PX_SYNTAX_H
#include "../core/PX_Core.h"
struct _PX_Syntax;
struct _PX_Syntax_ast;
struct _PX_Syntax_pbnfnode;
struct _PX_Syntax_pebnf;


typedef px_bool (*PX_Syntax_Function)(struct _PX_Syntax* pSyntax, struct _PX_Syntax_ast* past,px_void *userptr);
#define PX_SYNTAX_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax,struct _PX_Syntax_ast* past,px_void *userptr)
#define PX_SYNTAX_CALL_FUNCTION(name) name(pSyntax,past,userptr)

typedef px_bool (*PX_Syntax_Operate_Function)(struct _PX_Syntax* pSyntax, px_int operand1_abi_index, px_int operand2_abi_index,px_int opcode_abi_index);
#define PX_SYNTAX_OPERATE_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax, px_int operand1_abi_index, px_int operand2_abi_index,px_int opcode_abi_index)

typedef enum
{
	PX_SYNTAX_AST_TYPE_CONSTANT=0,
	PX_SYNTAX_AST_TYPE_LINKER,
	PX_SYNTAX_AST_TYPE_FUNCTION,
	PX_SYNTAX_AST_TYPE_RECURSION,
	PX_SYNTAX_AST_TYPE_LOOP,
	PX_SYNTAX_AST_TYPE_CONTINUOUS,
}PX_SYNTAX_AST_TYPE;

typedef enum
{
	PX_SYNTAX_OPERAND_FROM_MODULEBASE = 0,//offset = mp+offset
	PX_SYNTAX_OPERAND_FROM_RDATA, //offset = rp+offset
	PX_SYNTAX_OPERAND_FROM_PARAM, // address = bp+offset, the value is passed by parameter, and can be used in the same expression
	PX_SYNTAX_OPERAND_FROM_GLOBAL,  // address = gp+offset
	PX_SYNTAX_OPERAND_FROM_LOCAL, // address = bp-offset
	PX_SYNTAX_OPERAND_FROM_STACK, // offset not used,meaning the value is on stack top
	PX_SYNTAX_OPERAND_FROM_REFERENCE, //address = sp, the value is reference, and can be used in the same expression
	PX_SYNTAX_OPERAND_FROM_TEMP,  //offset = bp-address,but the value is temporary,can be used in the same expression, but will be released after the expression is evaluated
	PX_SYNTAX_OPERAND_FROM_CONST, //offset = 0, value = constant value, the value is not stored in memory, but can be used in the same expression
	PX_SYNTAX_OPERAND_FROM_MEMBER, //offset = bp-address, the value is a member of a struct, and can be used in the same expression
}PX_SYNTAX_OPERAND_FROM;

typedef enum
{
	PX_SYNTAX_AST_RETURN_CONTINUE,
	PX_SYNTAX_AST_RETURN_ERROR,
	PX_SYNTAX_AST_RETURN_END,
}PX_SYNTAX_AST_RETURN;



typedef struct _PX_Syntax_bnfnode
{
	PX_SYNTAX_AST_TYPE type;
	px_string constant;
	PX_Syntax_Function penterfunction, pleavefunction;
	px_void* userptr;
	struct _PX_Syntax_bnfnode* pothers;
	struct _PX_Syntax_bnfnode* pnext;
}PX_Syntax_bnfnode;

typedef struct
{
	px_string mnenonic;
	PX_Syntax_Function penterfunction,pleavefunction;
	px_void* userptr;
	PX_Syntax_bnfnode * pbnfnode;
}PX_Syntax_pebnf;


typedef struct _PX_Syntax_ast
{
	px_int pebnf_index;
	PX_Syntax_bnfnode* pbnfnode;
	px_int call_abistack_count;
	PX_SYNTAXLEXER_STATE syntaxlexer_state;
}PX_Syntax_ast;

typedef struct
{
	px_string name;
	px_vector members;//px_string
}PX_Syntax_maptype;

typedef enum
{
	PX_SYNTAX_OPCODE_TYPE_UNARY_PREFIX,
	PX_SYNTAX_OPCODE_TYPE_UNARY_SUFFIX,
	PX_SYNTAX_OPCODE_TYPE_BINARY,
	PX_SYNTAX_OPCODE_TYPE_TERNARY,
	PX_SYNTAX_OPCODE_TYPE_BEGIN,
	PX_SYNTAX_OPCODE_TYPE_END,
}PX_SYNTAX_OPCODE_TYPE;

typedef struct
{
	PX_SYNTAX_OPCODE_TYPE type;
	px_char opcode[7];
	px_char pair;
	px_int  precedence;
	px_bool assignment;
	px_int  offset_end_redirect_opcode_index;
}PX_Syntax_opcode;

typedef px_bool(*PX_Syntax_RegisterFunction)(struct _PX_Syntax* pSyntax, const px_char pabi_name[], px_int operand_index, px_int register_index);
#define PX_SYNTAX_REGISTER_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax, const px_char pabi_name[],px_int operand_index, px_int register_index)

typedef px_bool(*PX_Syntax_MemoryFunction)(struct _PX_Syntax* pSyntax, const px_char pabi_name[], px_int operand_index, px_int register_index);
#define PX_SYNTAX_MEMORY_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax, const px_char pabi_name[],px_int operand_index,px_int register_index)

typedef px_bool(*PX_Syntax_TypeConvertFunction)(struct _PX_Syntax* pSyntax, const px_char pabi_name[], px_int register_index);
#define PX_SYNTAX_TYPE_CONVERT_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax, const px_char pabi_name[],px_int register_index)

typedef px_int(*PX_Syntax_TypeSizeFunction)(struct _PX_Syntax* pSyntax, const px_char type_name[],px_abi *ptype_abi);
#define PX_SYNTAX_TYPE_SIZE_FUNCTION(name) px_int name(struct _PX_Syntax* pSyntax, const px_char type_name[],px_abi *ptype_abi)

typedef px_bool(*PX_Syntax_TypeDecorateFunction)(struct _PX_Syntax* pSyntax, px_abi* ptype_declare_abi);
#define PX_SYNTAX_TYPE_DECORATE_FUNCTION(name) px_bool name(struct _PX_Syntax* pSyntax,px_abi *ptype_declare_abi)

typedef enum
{
	PX_SYNTAX_MACHINE_OPCODE_TRAP=0,
    PX_SYNTAX_MACHINE_OPCODE_NOP,
	PX_SYNTAX_MACHINE_OPCODE_MOVR,
	PX_SYNTAX_MACHINE_OPCODE_MOVC,
	PX_SYNTAX_MACHINE_OPCODE_MOVF,
	PX_SYNTAX_MACHINE_OPCODE_MOVN,

    PX_SYNTAX_MACHINE_OPCODE_LOADU8,
	PX_SYNTAX_MACHINE_OPCODE_LOADU16,
	PX_SYNTAX_MACHINE_OPCODE_LOADU32,
	PX_SYNTAX_MACHINE_OPCODE_LOADI8,
	PX_SYNTAX_MACHINE_OPCODE_LOADI16,
	PX_SYNTAX_MACHINE_OPCODE_LOADI32,
	PX_SYNTAX_MACHINE_OPCODE_STORE8,
	PX_SYNTAX_MACHINE_OPCODE_STORE16,
	PX_SYNTAX_MACHINE_OPCODE_STORE32,
	PX_SYNTAX_MACHINE_OPCODE_LOADU8R,
	PX_SYNTAX_MACHINE_OPCODE_LOADU16R,
	PX_SYNTAX_MACHINE_OPCODE_LOADU32R,
	PX_SYNTAX_MACHINE_OPCODE_LOADI8R,
	PX_SYNTAX_MACHINE_OPCODE_LOADI16R,
	PX_SYNTAX_MACHINE_OPCODE_LOADI32R,

	PX_SYNTAX_MACHINE_OPCODE_STORE8R,
	PX_SYNTAX_MACHINE_OPCODE_STORE16R,
	PX_SYNTAX_MACHINE_OPCODE_STORE32R,
	PX_SYNTAX_MACHINE_OPCODE_STORE8C,
	PX_SYNTAX_MACHINE_OPCODE_STORE16C,
	PX_SYNTAX_MACHINE_OPCODE_STORE32C,
	PX_SYNTAX_MACHINE_OPCODE_STORE8RC,
	PX_SYNTAX_MACHINE_OPCODE_STORE16RC,
	PX_SYNTAX_MACHINE_OPCODE_STORE32RC,

	PX_SYNTAX_MACHINE_OPCODE_F2I,
	 PX_SYNTAX_MACHINE_OPCODE_F2U,
	PX_SYNTAX_MACHINE_OPCODE_I2F,
	PX_SYNTAX_MACHINE_OPCODE_U2F,

	PX_SYNTAX_MACHINE_OPCODE_PUSH,
	PX_SYNTAX_MACHINE_OPCODE_PUSHAD,
	PX_SYNTAX_MACHINE_OPCODE_POP,
	PX_SYNTAX_MACHINE_OPCODE_POPAD,
	PX_SYNTAX_MACHINE_OPCODE_POPN,

	PX_SYNTAX_MACHINE_OPCODE_INV,
	PX_SYNTAX_MACHINE_OPCODE_ANDL,
	PX_SYNTAX_MACHINE_OPCODE_ORL,

	PX_SYNTAX_MACHINE_OPCODE_NEG,
	PX_SYNTAX_MACHINE_OPCODE_ADD,
	PX_SYNTAX_MACHINE_OPCODE_ADDC,
	PX_SYNTAX_MACHINE_OPCODE_SUB,
	PX_SYNTAX_MACHINE_OPCODE_SUBC,
	PX_SYNTAX_MACHINE_OPCODE_MUL,
	PX_SYNTAX_MACHINE_OPCODE_MULC,
	PX_SYNTAX_MACHINE_OPCODE_DIV,
	PX_SYNTAX_MACHINE_OPCODE_DIVC,
	PX_SYNTAX_MACHINE_OPCODE_IDIV,
	PX_SYNTAX_MACHINE_OPCODE_IDIVC,
	PX_SYNTAX_MACHINE_OPCODE_MOD,
	PX_SYNTAX_MACHINE_OPCODE_MODC,
	PX_SYNTAX_MACHINE_OPCODE_IMOD,
	PX_SYNTAX_MACHINE_OPCODE_IMODC,
	PX_SYNTAX_MACHINE_OPCODE_FNEG,
	PX_SYNTAX_MACHINE_OPCODE_FADD,
	PX_SYNTAX_MACHINE_OPCODE_FADDC,
	PX_SYNTAX_MACHINE_OPCODE_FSUB,
	PX_SYNTAX_MACHINE_OPCODE_FSUBC,
	PX_SYNTAX_MACHINE_OPCODE_FMUL,
	PX_SYNTAX_MACHINE_OPCODE_FMULC,
	PX_SYNTAX_MACHINE_OPCODE_FDIV,
	PX_SYNTAX_MACHINE_OPCODE_FDIVC,

	PX_SYNTAX_MACHINE_OPCODE_AND,
	PX_SYNTAX_MACHINE_OPCODE_ANDC,
	PX_SYNTAX_MACHINE_OPCODE_OR,
	PX_SYNTAX_MACHINE_OPCODE_ORC,
	PX_SYNTAX_MACHINE_OPCODE_XOR,
	PX_SYNTAX_MACHINE_OPCODE_XORC,
	PX_SYNTAX_MACHINE_OPCODE_NOT,

	PX_SYNTAX_MACHINE_OPCODE_SHL,
	PX_SYNTAX_MACHINE_OPCODE_SHLC,
	PX_SYNTAX_MACHINE_OPCODE_SHR,
	PX_SYNTAX_MACHINE_OPCODE_SHRC,

	PX_SYNTAX_MACHINE_OPCODE_GT,
	PX_SYNTAX_MACHINE_OPCODE_GE,
	PX_SYNTAX_MACHINE_OPCODE_LT,
	PX_SYNTAX_MACHINE_OPCODE_LE,
	PX_SYNTAX_MACHINE_OPCODE_EQ,
	PX_SYNTAX_MACHINE_OPCODE_NEQ,
	PX_SYNTAX_MACHINE_OPCODE_UGT,
	PX_SYNTAX_MACHINE_OPCODE_UGE,
	PX_SYNTAX_MACHINE_OPCODE_ULT,
	PX_SYNTAX_MACHINE_OPCODE_ULE,
	PX_SYNTAX_MACHINE_OPCODE_FGT,
	PX_SYNTAX_MACHINE_OPCODE_FGE,
	PX_SYNTAX_MACHINE_OPCODE_FLT,
	PX_SYNTAX_MACHINE_OPCODE_FLE,
	PX_SYNTAX_MACHINE_OPCODE_FEQ,
	PX_SYNTAX_MACHINE_OPCODE_FNEQ,

	PX_SYNTAX_MACHINE_OPCODE_JMP,
	PX_SYNTAX_MACHINE_OPCODE_JZ,
	PX_SYNTAX_MACHINE_OPCODE_JNZ,
	PX_SYNTAX_MACHINE_OPCODE_CALL,

	PX_SYNTAX_MACHINE_OPCODE_CALLR,
	PX_SYNTAX_MACHINE_OPCODE_JMPR,

	PX_SYNTAX_MACHINE_OPCODE_RET,
}PX_SYNTAX_MACHINE_OPCODE;


typedef struct _PX_Syntax
{
	px_memorypool* mp;
	//px_int reg_lifetime;
	px_string message;
	px_vector pebnf;
	PX_Syntax_bnfnode bnfnode_return;
	px_vector reg_ast_stack;
	px_vector reg_abi_stack;
	px_vector reg_ast_instr_stack;
	px_vector reg_expr_opcode_stack;//px_string
	px_syntaxlexer reg_syntaxlexer;
	px_int    reg_expr_source_index;
	px_int    reg_expr_begin_line;
	px_int    reg_write_expr_source_index;
	px_int    reg_write_expr_begin_line;
	px_vector reg_maptype_stack;//PX_Syntax_MapType
	px_int    reg_lastsource_index;
	px_int	  reg_unname_index;
	px_char   reg_unname[32];
	px_dword  reg_module_base_addr;
	px_int    execute_id, break_execute_id;
}PX_Syntax;

//Initialize/free/clear
px_bool PX_Syntax_Initialize(px_memorypool* mp, PX_Syntax* pSyntax);
px_void PX_Syntax_Free(PX_Syntax* pSyntax);
px_void PX_Syntax_ClearState(PX_Syntax* pSyntax);
px_void PX_Syntax_ClearSources(PX_Syntax* pSyntax);
px_void PX_Syntax_ClearSourceDescription(PX_Syntax* pSyntax);

//lifetime
px_int PX_Syntax_GetLifetime(PX_Syntax* pSyntax);

//random color
px_color PX_Syntax_GetRandomColor(px_uint32 seed);

//execute
px_void PX_Syntax_MakeBreakPoint(PX_Syntax* pSyntax, px_int break_execute_id);
PX_SYNTAX_AST_RETURN PX_Syntax_ExecuteNext(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentSourceIndex(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentRow(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLine(PX_Syntax* pSyntax);
px_bool PX_Syntax_IsExecuting(PX_Syntax* pSyntax);

//source
px_bool PX_Syntax_AddSource(PX_Syntax* pSyntax, const px_char filename[], const px_char source[]);
PX_SyntaxLexer_Source* PX_Syntax_GetSourceByIndex(PX_Syntax* pSyntax, px_int index);
px_int PX_Syntax_GetSourceCount(PX_Syntax* pSyntax);
px_bool PX_Syntax_IsValidToken(const px_char token[]);
px_bool PX_Syntax_IsEndOfSource(PX_Syntax* pSyntax);
//pebnf
PX_Syntax_pebnf* PX_Syntax_GetPebnfByIndex(PX_Syntax* pSyntax, px_int index);
PX_Syntax_pebnf* PX_Syntax_GetPebnf(PX_Syntax* pSyntax, const px_char mnemonic[]);
PX_Syntax_bnfnode* PX_Syntax_GetPebnfNode(PX_Syntax* pSyntax, const px_char mnemonic[]);
PX_Syntax_bnfnode* PX_Syntax_GetOtherNode(PX_Syntax_bnfnode* pbnfnode, PX_SYNTAX_AST_TYPE ast_type, const px_char mnemonic[]);
px_bool PX_Syntax_Parse_PEBNF(PX_Syntax* pSyntax, const px_char PEBNF[], PX_Syntax_Function penterfunction, PX_Syntax_Function pleavefunction, px_void* userptr);

//ast opcode
px_bool PX_Syntax_ExecuteAstOpcode(PX_Syntax* pSyntax, const px_char type[]);


//opcode
px_int PX_Syntax_NewOpcodeDefine(PX_Syntax* pSyntax, const px_char opcode[], PX_SYNTAX_OPCODE_TYPE type, px_dword precedence);
px_int PX_Syntax_GetOpcodeDefineIndex(PX_Syntax* pSyntax, const px_char opcode[], PX_SYNTAX_OPCODE_TYPE type);
PX_Syntax_opcode* PX_Syntax_GetOpcodeDefine(PX_Syntax* pSyntax, px_int index);
px_bool PX_Syntax_ExecuteOpcode(PX_Syntax* pSyntax, px_int opcode_index);

//abi
px_abi* PX_Syntax_NewAbi(PX_Syntax* pSyntax, const px_char name[]);

px_abi* PX_Syntax_GetAbiByIndex(PX_Syntax* pSyntax, px_int index);
px_abi* PX_Syntax_GetAbiFromForward(PX_Syntax* pSyntax, const px_char name[]);
px_abi* PX_Syntax_GetAbiFromBackward(PX_Syntax* pSyntax, const px_char name[]);


px_void PX_Syntax_PopOperate3(PX_Syntax* pSyntax, px_int abi1_index, px_int abi2_index, px_int abi3_index);
px_void PX_Syntax_PopOperate2(PX_Syntax* pSyntax, px_int abi1_index, px_int abi2_index);
px_void PX_Syntax_PopLastAbiName(PX_Syntax* pSyntax, const px_char name[]);

px_void PX_Syntax_PopAbi(PX_Syntax* pSyntax);
px_void PX_Syntax_PopLastSecondAbi(PX_Syntax* pSyntax);
px_void PX_Syntax_PopAbiIndex(PX_Syntax* pSyntax, px_int index);
px_bool PX_Syntax_RenameLastAbi(PX_Syntax* pSyntax, const px_char name[]);
px_bool PX_Syntax_MergeLastAbi(PX_Syntax* pSyntax, const px_char new_name[]);
px_bool PX_Syntax_MergeLast2AbiToSecondLast(PX_Syntax* pSyntax);
px_bool PX_Syntax_MergeLast2AbiWithNameToSecondLast(PX_Syntax* pSyntax, const px_char name[]);
px_bool PX_Syntax_MergeAbiIndexToAbiIndexWithName(PX_Syntax* pSyntax, px_int src_index, px_int dst_index, const px_char name[]);

px_abi* PX_Syntax_GetLastAbi(PX_Syntax* pSyntax);
px_abi* PX_Syntax_GetSecondLastAbi(PX_Syntax* pSyntax);
px_abi* PX_Syntax_GetThirdLastAbi(PX_Syntax* pSyntax);
px_bool PX_Syntax_CheckAbiName(px_abi* pabi, const px_char name[]);
px_bool PX_Syntax_CheckLastAbiName(PX_Syntax* pSyntax, const px_char name[]);
px_bool PX_Syntax_CheckSecondLastAbiName(PX_Syntax* pSyntax, const px_char name[]);
px_int  PX_Syntax_GetAbiIndexFromForward(PX_Syntax* pSyntax, const px_char name[]);
px_int  PX_Syntax_GetAbiIndexFromBackward(PX_Syntax* pSyntax, const px_char name[]);
px_int  PX_Syntax_GetSecondAbiIndexFromBackward(PX_Syntax* pSyntax, const px_char name[]);
px_int PX_Syntax_GetAbiCount(PX_Syntax* pSyntax);

//operand
px_abi* PX_Syntax_PushOperand(PX_Syntax* pSyntax, const px_char type[], px_int type_size, PX_SYNTAX_OPERAND_FROM datafrom);
px_bool PX_Syntax_OperandToRegister(PX_Syntax* pSyntax,const px_char pirabi_name[], px_int operand_abi_index, px_int register_index);
px_bool PX_Syntax_OperandToMemory(PX_Syntax* pSyntax, const px_char pirabi_name[], px_int operand_abi_index, px_int register_index);

//type system
px_bool PX_Syntax_NewType(PX_Syntax* pSyntax, const px_char type[], const px_char mnemonic[], const  px_int size, PX_Syntax_TypeSizeFunction sizefunction);
px_bool PX_Syntax_NewTypedef(PX_Syntax* pSyntax, const px_char type[], const px_char mnemonic[]);
px_bool PX_Syntax_NewTypeRegisterMemoryMap(PX_Syntax* pSyntax, const px_char type[], PX_Syntax_RegisterFunction memory_to_register, PX_Syntax_MemoryFunction register_to_memory);
px_bool PX_Syntax_NewTypeConvert(PX_Syntax* pSyntax, const px_char from_type[], const px_char to_type[],PX_Syntax_TypeConvertFunction convert_function);
px_bool PX_Syntax_NewTypeDecorate(PX_Syntax* pSyntax, const px_char type[], const px_char decorate[], PX_Syntax_TypeDecorateFunction decorate_function);

px_void PX_Syntax_TypeDecorate(PX_Syntax* pSyntax, px_abi* ptype_declare_abi, const px_char decorate[]);
px_bool PX_Syntax_ConvertType(PX_Syntax* pSyntax, const px_char pirabi_name[], px_int register_index, const px_char from_type[], const px_char to_type[]);
px_bool PX_Syntax_TypeMatch(const px_char source_type[], const px_char target_type[]);
px_bool PX_Syntax_TypeMatch2(const px_char source_type[], const px_char target_type[], const px_char target_type2[]);
px_bool PX_Syntax_TypeMatch3(const px_char source_type[], const px_char target_type[], const px_char target_type2[], const px_char target_type3[]);


const px_char* PX_Syntax_GetTypeByMnemonic(PX_Syntax* pSyntax, const px_char mnemonic[]);
px_bool PX_Syntax_GetTypeAbi(PX_Syntax* pSyntax, const px_char type[], px_abi* ptype_define_abi);
px_int  PX_Syntax_GetTypeSize(PX_Syntax* pSyntax, const px_char type[]);
px_int	PX_Syntax_GetTypeScopeAbiIndex(PX_Syntax* pSyntax, const px_char type[]);
px_abi* PX_Syntax_GetTypeScopeAbi(PX_Syntax* pSyntax, const px_char type[]);


px_bool PX_Syntax_GetScopeMatchTypeAbi(PX_Syntax* pSyntax, px_int scope_index, const px_char type[], px_abi* ptype_define_abi);
px_bool PX_Syntax_GetMatchTypeAbi(PX_Syntax* pSyntax, const px_char type[], px_abi* ptype_define_abi);
px_int  PX_Syntax_GetMatchTypeSize(PX_Syntax* pSyntax, const px_char type[]);

px_bool PX_Syntax_MatchTypeExist(PX_Syntax* pSyntax, const px_char type[]);

px_bool PX_Syntax_EnterScope(PX_Syntax* pSyntax);
px_bool PX_Syntax_EnterBaseScope(PX_Syntax* pSyntax);
px_bool PX_Syntax_LeaveScope(PX_Syntax* pSyntax);
px_abi* PX_Syntax_GetLastScopeAbi(PX_Syntax* pSyntax);
px_abi* PX_Syntax_GetSecondLastScopeAbi(PX_Syntax* pSyntax);
px_abi* PX_Syntax_GetLastBaseScopeAbi(PX_Syntax* pSyntax);
px_int  PX_Syntax_AllocLocal(PX_Syntax* pSyntax, px_int size);
px_int  PX_Syntax_AllocGlobal(PX_Syntax* pSyntax, px_int size);
px_int  PX_Syntax_AllocParam(PX_Syntax* pSyntax, px_int size);
px_int  PX_Syntax_AllocRdata(PX_Syntax* pSyntax,const px_byte *buffer, px_int size);
//ast
px_void PX_Syntax_AstReturn(PX_Syntax* pSyntax, PX_Syntax_ast* past,const px_char message[]);

//lexer
px_bool PX_Syntax_CallSource(PX_Syntax* pSyntax, const px_char name[], const px_char pebnf[]);
px_bool PX_Syntax_CallSourceIndex(PX_Syntax* pSyntax, px_int index, const px_char pebnf[]);
px_int PX_Syntax_GetCurrentLexerOffset(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexemeBeginSourceIndex(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexemeEndSourceIndex(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexemeBegin(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexemeEnd(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexemeLine(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexerIndex(PX_Syntax* pSyntax);
px_int PX_Syntax_GetCurrentLexerLine(PX_Syntax* pSyntax);
const px_char* PX_Syntax_GetCurrentLexerSourceName(PX_Syntax* pSyntax);
const px_char * PX_Syntax_GetCurrentLexeme(PX_Syntax* pSyntax);
const px_char* PX_Syntax_GetCurrentLexerEntrySourceName(PX_Syntax* pSyntax);
px_char PX_Syntax_PreviewNextChar(PX_Syntax* pSyntax);
px_char PX_Syntax_GetNextChar(PX_Syntax* pSyntax);
px_void PX_Syntax_LexerForward(PX_Syntax* pSyntax, px_int offset);
PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme(PX_Syntax* pSyntax);
PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme3(PX_Syntax* pSyntax);
PX_SYNTAXLEXER_LEXEME_TYPE PX_Syntax_GetNextLexeme4(PX_Syntax* pSyntax);
PX_SYNTAXLEXER_STATE PX_Syntax_GetLexerState(PX_Syntax* pSyntax);
px_void PX_Syntax_SetLexerState(PX_Syntax* pSyntax, PX_SYNTAXLEXER_STATE* pstate);

//lifetime
px_bool PX_Syntax_GrabeVariablesAbi(PX_Syntax* pSyntax, px_abi* pToAbi, const px_char father_payload[]);

//common
px_bool PX_Syntax_Execute(PX_Syntax* pSyntax,const px_char filename[], const px_char pebnf[]);

px_void PX_Syntax_Terminate(PX_Syntax* pSyntax,const px_char message[]);
px_bool PX_Syntax_IsTerminated(PX_Syntax* pSyntax);
px_void PX_Syntax_Message(PX_Syntax* pSyntax, const px_char message[]);
px_string* PX_Syntax_GetMessage(PX_Syntax* pSyntax);
px_void PX_Syntax_MessageClear(PX_Syntax* pSyntax);
px_bool PX_Syntax_RemovePebnf(PX_Syntax* pSyntax, const px_char pebnf[]);
const px_char* PX_Syntax_AllocUnnamed(PX_Syntax* pSyntax,const px_char *prefix);


//output
px_bool PX_Syntax_AppendString(PX_Syntax* pSyntax, const px_char name[], const char content_name[], const px_char content[]);
px_bool PX_Syntax_AppendBuffer(PX_Syntax* pSyntax, const px_char name[], const char data_name[], const px_byte data[], px_int datasize);
px_bool PX_Syntax_MergeLast2AbiValue(PX_Syntax* pSyntax, const px_char last_abi_name[], const px_char value_name[], const px_char second_last_abi_name[], const px_char value_name2[]);
px_bool PX_Syntax_MergeAbiFromBeginToEnd(PX_Syntax* pSyntax, const px_char begin_abi_name[], const px_char search_name[], const px_char merge_to_name[]);
px_void PX_Syntax_PopAbiFromBeginNext(PX_Syntax* pSyntax, const px_char begin_abi_name[]);//dangerous function, avoid using it


px_abi* PX_Syntax_NewMap(PX_Syntax* pSyntax, px_int sourceindex, px_int begin, px_int end, const px_char key[]);
px_abi* PX_Syntax_NewStaticMapToken(PX_Syntax* pSyntax, px_int begin_sourceindex, px_int begin, px_int end_sourceindex, px_int end, px_color color, const px_char type[]);
px_abi* PX_Syntax_NewDynamicMapToken(PX_Syntax* pSyntax, px_int begin_sourceindex, px_int begin, px_int end_sourceindex, px_int end, px_color color, const px_char type[]);
px_bool PX_Syntax_SetLastMapInfo(PX_Syntax* pSyntax, px_int sourceindex, const px_char info[]);
PX_RETURN_STRING PX_Syntax_BuildIRLocation(PX_Syntax* pSyntax, px_int source_index, px_int line);
px_bool PX_Syntax_NewIRLocation(PX_Syntax* pSyntax, const px_char merge_to_abi[]);
px_void PX_Syntax_ResetIRLocation(PX_Syntax* pSyntax);

px_bool PX_Syntax_NewIRInstructions(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char payload[]);
px_bool PX_Syntax_NewIRInstruction0(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[]);
px_bool PX_Syntax_NewIRInstruction1(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[]);
px_bool PX_Syntax_NewIRInstruction2(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[], const px_char operand2[]);
px_bool PX_Syntax_NewIRInstruction3(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char opcode[], const px_char operand1[], const px_char operand2[], const px_char operand3[]);

px_bool PX_Syntax_NewIRInstructionFormat0(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[]);
px_bool PX_Syntax_NewIRInstructionFormat1(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[],px_stringformat fm1);
px_bool PX_Syntax_NewIRInstructionFormat2(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[],px_stringformat fm1, px_stringformat fm2);
px_bool PX_Syntax_NewIRInstructionFormat3(PX_Syntax* pSyntax, const px_char merge_to_abi[], const px_char format[], px_stringformat fm1, px_stringformat fm2, px_stringformat fm3);

px_bool PX_Syntax_IR_optimize_pass1(px_string* ir);


PX_SYNTAX_FUNCTION(PX_Syntax_TokenRender);
PX_SYNTAX_FUNCTION(PX_Syntax_TokenRenderScope);
#endif // !PX_SCRIPT_STNTACTIC_H

