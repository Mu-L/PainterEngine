/*          PainterEngine Compiler Architecture
*
* /////////////////////////////////////////////////////////////
*			 ------------------------------>
*			^								|
*			|								* 
*    syntax_source--->syntax_optimizer-->syntax_ir--->machine
* 
* //////////////////////////////////////////////////////////////
*

             PainterEngine MicroSoc(CPU FPU GPU) Architecture
*    +--------------+           +------------------------+
*    |     ALU      |           |           FPU          |
*    +--------------+           | f0.f1.f2.f3.C0.C2.C3   |
*           |                   +------------------------+
*           |                          |
*   +-------------------------------------------+    
*   |               CPU Registers               |----------------+
*   |       r0...r3 | ip | sp | bp | flag(ZCNV) |                |
*   +-------------------------------------------+                |
*                        |                                       |
*  +--------------------------------------------+    +-------------------------+
*  |                 Memory                     |----|         GPU             |
*  +--------------------------------------------+    +-------------------------+
* 
* r0...r3 integer
* f0...f3 float
* r4...r7 ip,sp,bp,flag
* gp---> linker constant of global pointer
* rp---> linker constant of resources pointer
* tp---> linker constant of text pointer
* 64bits for instruction

//pseudo-instruction
import module_name(string)
label_name:
export label_name:
db label_name:  string(eg:ff00ff001020....)

//opcode 1byte  1bit for extern 7bit for opcode
//nop
nop

*ir_const = [const][gp + const][rp + const][tp + const]

* register / const to register
mov[r0...r7,f0...f3], [r0...r7,f0...f3] //3bytes 8bits for opcode.(0~3bit)4bit for dest type(0 common,1 float),(3~7bit)4bit for index,(0~3bit)4bit for source type(0 common,1 float),(3~7bit)4bit for index
mov[r0...r7], [ir_const] //6-bytes 8bits for opcode.(0~3bit)3bit for register index,4bits reserved, 32bits for const
movf[r0...r7,f0..f3], [const] //6-bytes 8bits for opcode.(0~2bit)3bit for register index,1bits flag(0 common 1 float) .32bits for const float

* memory to memory
movn ;r0(source address), r1(destination address), r2(size) //1-byte opcode

f2i [r0...r1][f0...f1]  fx->rx //16bits // 8bits for opcode.(0~3)4bit for common register index,(4~7)4bit for float register index
i2f [f0...f1][r0...r1]  rx->fx //16bits // 8bits for opcode.(0~3)4bit for float register index,(4~7)4bit for common register index
u2f [f0...f1][r0...r1]  rx->fx //16bits // 8bits for opcode.(0~3)4bit for float register index,(4~7)4bit for common register index
ff2f //8 bit for opcode,copy fflag to flag register
* memory to register
* 
* 
* 
loadu8[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
loadu16[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
loadu32[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset

loadi8[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
loadi16[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
loadi32[r0...r7], [ir_const][bp - const][sp + const][bp + const] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset

loadu8(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(u8*)rx
loadu16(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(u16*)rx
loadu32(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(u32*)rx

loadi8(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(i8*)rx
loadi16(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(i16*)rx
loadi32(r)[r0...r7] //2-bytes 8bits for opcode,4bit for address/result register,4bit reserved; rx=*(i32*)rx

* register / const to memory
store8 [ir_const][bp - const][sp + const][bp + const], [r0...r7] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
store16 [ir_const][bp - const][sp + const][bp + const], [r0...r7] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset
store32 [ir_const][bp - const][sp + const][bp + const], [r0...r7] //6-bytes 8bits for opcode.4bit for register,4bit for param(3)[bp+n]/stack(2)[sp+n]/local(1)[bp-n]/global(0)[gp+n] 32bits for address offset

store8(r)[r0...r7],[r0...r7] //2-bytes 8bits for opcode.4bit for source register,4bit for dest register
store16(r)[r0...r7],[r0...r7] //2-bytes 8bits for opcode.4bit for source register,4bit for dest register
store32(r)[r0...r7],[r0...r7] //2-bytes 8bits for opcode.4bit for source register,4bit for dest register

store8(c) [ir_const][bp - const][sp + const][bp + const], [ir_const]//12-bytes 8bits for opcode.4bit for param(3)/stack(2)/local(1)/global(0) for dest,4bit for param(3)/stack(2)/local(1)/global(0) for source,32bits for dest address offset,32bits for source constant value
store16(c) [ir_const][bp - const][sp + const][bp + const], [ir_const]//12-bytes 8bits for opcode.4bit for param(3)/stack(2)/local(1)/global(0) for dest,4bit for param(3)/stack(2)/local(1)/global(0) for source,32bits for dest address offset,32bits for source constant value
store32(c) [ir_const][bp - const][sp + const][bp + const], [ir_const]//12-bytes 8bits for opcode.4bit for param(3)/stack(2)/local(1)/global(0) for dest,4bit for param(3)/stack(2)/local(1)/global(0) for source,32bits for dest address offset,32bits for source constant value

store8(rc) [r0...r7],[ir_const] //6-bytes  8bits for opcode.8bit for register,32bit for constant value
store16(rc) [r0...r7],[ir_const]
store32(rc) [r0...r7],[ir_const]

* stack operations
push [r0...r7, ip, sp, bp, flag] //2-bytes 8bits for opcode.(4~7)4bit for register index(4-ip 5-sp 6-bp 7-flag),(0~3)4bit for type(0-reserved 1-register)
pop[r0...r7, ip, sp, bp, flag]  //2-bytes 8bits for opcode.4bit for register index(4-ip 5-sp 6-bp 7-flag),4bit for type(0~3)(0-none 1-register)
popn [r0] //1-bytes 8bits for opcode

*ALU operations
* Arithmetic
* neg [r0...r7] // 2bytes 8bits for opcode.4bit for register index
* add [r0...r7],[r0...r7]//rd=rd+rs //2bytes 8bits for opcode,4bit for dest index,4bit for source index
* sub [r0...r7],[r0...r7]//rd=rd-rs
* mul [r0...r7],[r0...r7]//rd=rd*rs
* div [r0...r7],[r0...r7]//rd=rd/rs (unsigned)
* idiv [r0...r7],[r0...r7]//rd=rd/rs (signed)
* 
* add(c) [r0...r7],[ir_const]//rd=rd+const //6bytes 8bits for opcode,4bit for dest index,4bit reserved,32bit for const
* sub(c) [r0...r7],[ir_const]//rd=rd-const
* mul(c) [r0...r7],[ir_const]//rd=rd*const
* div(c) [r0...r7],[ir_const]//rd=rd/const (unsigned)
* idiv(c) [r0...r7],[ir_const]//rd=rd/const (signed)
* 
* mod [r0...r7],[r0...r7]//rd=rd%rs (unsigned)
* mod(c) [r0...r7],[ir_const]//rd=rd%const (unsigned)
* imod [r0...r7],[r0...r7]//rd=rd%rs (signed)
* imod(c) [r0...r7],[ir_const]//rd=rd%const (signed)
*
* FPU operations
* fneg [f0...f3]
* fadd [f0...f3],[f0...f3]//fd=fd+fs //2bytes
* fsub [f0...f3],[f0...f3]//fd=fd-fs
* fmul [f0...f3],[f0...f3]//fd=fd*fs
* fdiv [f0...f3],[f0...f3]//fd=fd/fs
*
* fadd(c) [f0...f3],[ir_const]//fd=fd+const //6bytes 8bits for opcode,4bit for dest index,4bit reserved,32bit for const
* fsub(c) [f0...f3],[ir_const]//fd=fd-const
* fmul(c) [f0...f3],[ir_const]//fd=fd*const
* fdiv(c) [f0...f3],[ir_const]//fd=fd/const

* Logical
* and [r0...r7],[r0...r7]//rd=rd&rs //2bytes
* or  [r0...r7],[r0...r7]//rd=rd|rs //2bytes
* xor [r0...r7],[r0...r7]//rd=rd^rs //2bytes
* not [r0...r7] //2bytes 8bits for opcode.4bit for register index //logical not rx=rx?0:1
* inv [r0...r7] //2bytes 8bits for opcode.4bit for register index //bitwise not rx=~rx
* andl [r0...r7][r0...r7] //2bytes 8bits for opcode,(0~3)4bit for dest/src1 index,(4~7)4bit for src2 index //logical and rx=(rx&&ry)?1:0
* orl  [r0...r7][r0...r7] //2bytes 8bits for opcode,(0~3)4bit for dest/src1 index,(4~7)4bit for src2 index //logical or  rx=(rx||ry)?1:0
* shl [r0...r7],[r0...r7]//rd=rd<<rs //2bytes
* shr [r0...r7],[r0...r7]//rd=rd>>rs //2bytes
* 
* and(c) [r0...r7],[ir_const]//rd=rd&const //6bytes
* or(c)  [r0...r7],[ir_const]//rd=rd|const //6bytes
* xor(c) [r0...r7],[ir_const]//rd=rd^const //6bytes
* shl(c) [r0...r7],[ir_const]//rd=rd<<const //6bytes
* shr(c) [r0...r7],[ir_const]//rd=rd>>const //6bytes
* 
* Comparison (set-compare: the 0/1 result is written into the destination register)
* all 2 bytes: 8bits for opcode,4bit for destination/left operand index,4bit for right operand index
*
* signed integer:
* gt  [r0...r7],[r0...r7]//rd=((int)rd> (int)rs)?1:0
* ge  [r0...r7],[r0...r7]//rd=((int)rd>=(int)rs)?1:0
* lt  [r0...r7],[r0...r7]//rd=((int)rd< (int)rs)?1:0
* le  [r0...r7],[r0...r7]//rd=((int)rd<=(int)rs)?1:0
* eq  [r0...r7],[r0...r7]//rd=(rd==rs)?1:0 (sign agnostic)
* neq [r0...r7],[r0...r7]//rd=(rd!=rs)?1:0 (sign agnostic)
*
* unsigned integer:
* ugt [r0...r7],[r0...r7]//rd=(rd> rs)?1:0
* uge [r0...r7],[r0...r7]//rd=(rd>=rs)?1:0
* ult [r0...r7],[r0...r7]//rd=(rd< rs)?1:0
* ule [r0...r7],[r0...r7]//rd=(rd<=rs)?1:0
*
* float (dest is an integer register, both sources are float registers, 3-operand form only):
* fgt  [r0...r7][f0...f3][f0...f3]//rd=(fa> fb)?1:0
* fge  [r0...r7][f0...f3][f0...f3]//rd=(fa>=fb)?1:0
* flt  [r0...r7][f0...f3][f0...f3]//rd=(fa< fb)?1:0
* fle  [r0...r7][f0...f3][f0...f3]//rd=(fa<=fb)?1:0
* feq  [r0...r7][f0...f3][f0...f3]//rd=(fa==fb)?1:0
* fneq [r0...r7][f0...f3][f0...f3]//rd=(fa!=fb)?1:0
*
* jmp [label] //5 bytes 8bits for opcode.32bits for label address
* jz  [r0...r7],[label] //6 bytes 8bits for opcode,4bit for test register index,4bit zero append,32bits for label address //rx==0?
* jnz [r0...r7],[label] //6 bytes 8bits for opcode,4bit for test register index,4bit zero append,32bits for label address //rx!=0?
* call [label] // 8bits for opcode. 32bits for label address
* 
* jmpr [r0] // 8bits for opcode.8bit for regiser-index
* callr [r0] // 8bits for opcode.8bit for regiser-index
* 
* 
* end-to-end instructions
* 
* 
* ee_push [const,rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr] //6-bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-gp+addr 6-bp-addr 7-sp+addr 8-bp+addr),32bits for address
* ee_pop [const,rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr] //6-bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-gp+addr 6-bp-addr 7-sp+addr 8-bp+addr),32bits for address

* ee_neg [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr] // 6bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr),32bits for address
* ee_add [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_sub [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_mul [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_div [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_idiv [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_mod [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
*
* FPU operations
* ee_fneg [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr] // 6bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr),32bits for address
* ee_fadd [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_fsub [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_fmul [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_fdiv [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr],[const,fx,addr,bp-addr,,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
*

* Logical
* ee_and [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_or  [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_xor [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_not [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr] // 6bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr),32bits for address
* ee_shift left [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,fx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,fx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address
* ee_shift right [rx,fx,addr,gp+addr,bp-addr,sp+addr,bp+addr],[const_int,rx,fx,addr,bp-addr,sp+addr,bp+addr],[const_int,rx,fx,addr,bp-addr,sp+addr,bp+addr] //16 bytes 8bits for opcode,8bit for dest type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr)x3,32bits for dest address,op1,op2 address

* ee_call [const_int,rx,fx,addr,gp+addr,bp-addr,label,sp+addr,bp+addr] //6-bytes 8bits for opcode.8bit for type(0-const_int 1-const_float 2-rx 3-fx 4-addr 5-bp-addr 6-sp+addr 7-bp+addr),32bits for address

/*symbols
@;go_source source_index(const_int)
@;new begin(const_int) end(const_int)
@;color argb(const_int_list)
@;info info(string)
@;local source_index(const_int) variable_index(const_int)

*/
// is_eof(function)

// define -- define_identifier --- value(string)
//       |
//        -- define_parameters --- identifier[] --- identifier[] --- value(string)
//       |                    |
//       |                     --- count --- value(string)
//		 |
//        -- define_content --- value(string)
//       |
//        -- format(string)

// call_define_identifier --- value(string)
//                       |
//                        --- macro_id(string)

// call_define(function)

//expand_parameters --- expand_parameter[] --- value(string)
//				   |
//					--- count(int)

// keyword(function) -- *[]["if","else","switch","case","default",\
		"while","for","do","break","continue","return","goto","typedef",\
		"extern","static","auto","register","const","volatile","sizeof",\
		"enum","struct","union","void","char","short","int","long","float",\
		"double","signed","unsigned","_Bool","_Complex","_Imaginary","inline",\
		"restrict","_Alignas","_Alignof","_Atomic","_Generic","_Noreturn",\
		"_Static_assert","_Thread_local"];

// identifier --- value(string)
//           |
//            --- source_index(int)
//           |
//            --- begin(int)
//           |
//            --- end(int)



// const_float --- value(string)


// const_int --- value(string)
//          |
//           --- unsigned(bool)

// numeric --- const_int(abi)
//        |
//         --- const_float(abi)

// bcontainer --- value(string)

// const_string --- value(sting)


// const_float_list  --- const_float[](abi)
//                 |
//                  --- list_count(int)

// const_int_list    --- const_int[](abi)
//                  |
//                   --- list_count(int)

// const_numeric_list --- const_numeric[](abi)
//                   |
//                    --- list_count(int)

// const_string_list  --- const_string[](abi)
//                   |
//                    --- list_count(int)

// const_tuple_list   --- const_tuple[](abi)
//                   |
//                    --- list_count(int)

//  numeric --- const_int(abi)
//         |
//          --- const_float(abi)
//         |
//          --- const_string(abi)

// declare_prefix --- value ['const','static','unsigned'](string)

// declare_prefixs --- [](string)

// declare_token_prefix --- type ['pointer','reference'](string)
//                     |
//                      --- pointer_level(int)

// declare_array   --- d(int)
//                |
//                 --- [d](int 0~d-1)
//				  |
//				   --- count(int)

// const_int_set --- const_int_list(abi)
// const_float_set --- const_float_list(abi)
// const_string_set --- const_string_list(abi)
// const_tuple_set --- const_tuple_list(abi)

// define_struct  --- struct_name --- value(string)
//				 |
//				 |--- member_count(int)
//				 |
//				 |--- size(int)
//				 |
//				 |--- [](variable abi)


//identifier
//pointer:any
//ix.'u8/16/32/i8/16/32/const',
//fx.'f.32/const'
//void
//struct.'struct_name'
// 
// lifetime(int 1:global others:local or define)
// scope  --- alloc(int)
//       |
//        --- type_mnemonic---string->string(eg :'int'->'ix.i.32','float'->'fx.f.32')
//       |
//        --- type_defines(type_define abis,eg:ix.i.32)      
//       |
//        --- variables--variable_identifier---variable abi

// type  --- value(string) (eg:'ix.i.32','ix.u.8','fx.f.32','pointer.array.ix.i.32','struct_name')


// variable --- identifier(string)
//         |
//          --- type(string)
//         |
//          --- offset(int)
//		   |
//			--- from(string global/local/param)


// 
// type_define(*define_struct)   --- activate(bool) (only an activated node is a real type,
//								|                    the nodes which only exist as the parent of
//								|                    another type(eg:'ix' of 'ix.i.32') have no activate)
//								|-- size(int)
//								|
//								|-- *member_count(int)
//								|
//								|-- *members--variable identifier--variable(abi)
//								|
//							     --- operates(operate abis) -- [] -- opcode_index(int) 
//							                                  |
//							                                   -- *other_type(string) (eg:ix:i:8)
//							                                  |
//							                                   -- function(data) PX_Syntax_Operate_Function


// function_block --- return_type(string)
//				  |
//				   --- function_name(string)
// 				  |
//				   --- params--*(parameters abi variable)
//				  |
//				   --- params_size ((int)<--push params stack size)

// call_function   --- value(string)
//                |
//                 --- check_param_count(int)
//				  |
//				   --- function_block(abi function_block)

// variable map  --- type(string must be variable)
//				|
//				 --- variable_name(string)
//				|
//				 --- variable_type(string)
//				|
//				 --- variable_from(string global/local/param)
//				|
//				 --- variable_offset(int)
//				|
//				 --- variable_size(int)

// expr -- ir(string)
//     |
//		-- type(string) ['ix.i.32','fx.f.8'...]
//	   |
//		-- type_size(int)

//pointer:any
//array:any
//ix:u/i/const:8/16/32/i8/16/32',
//fx:'f/const:32'
//void
//struct:'struct_name'

//operand --- type(string) ['ix.i.32','fx.f.8'...] <----opcode target
//      |
//	     --- type_size(int) 
//		|
//       --- from(int)
/*
*			PX_SYNTAX_OPERAND_FROM_ADDRESS=0,    //n
			PX_SYNTAX_OPERAND_FROM_GLOBAL,		//gp+n
			PX_SYNTAX_OPERAND_FROM_LOCAL,       //bp-n
			PX_SYNTAX_OPERAND_FROM_TEMP,
			PX_SYNTAX_OPERAND_FROM_PARAM,       //bp+8+n
			PX_SYNTAX_OPERAND_FROM_STACK,       //sp+n
			PX_SYNTAX_OPERAND_FROM_CONST,
			PX_SYNTAX_OPERAND_FROM_MEMBER,      //struct member [bp+8]+n

*/
//      |
//		 --- *array-- --d(int) [array dimension]
//                   |  
//                   --[](int) [array dimension]
//                   |
//                   --count(int) (count = size/type_size)
//      |
//       --- *value(string) if 'const' then is const value

//      |
//       --- *offset(int)


// opcode --- index(index)


//ir --- value(string)


/*PX_Machine
ir_rx ---- index(int) [r0,r1,r2,r3]

ir_addr --- *global_offset(int)
		 |
		  --- *local_offset(int)
		 |
		  --- *stack_offset(int)
		 |
		  --- *param_offset(int)


operate  --- operand_type[](string)
        |
		 --- operand_count(int)
		|
		--- pfunction(data PX_Syntax_Operate_Function*)


define_opcode --- opcode(string)
			 |
			  --- type(int) [PX_SYNTAX_OPCODE_TYPE]
			 |
			  --- precedence(int)
			 |
			  --- operate_count(int)
			 |
			  --- operate[](abi)

Machine Memory map
=============================================
   ^		^            					^
   |		|								|
   |		|		    bp-alloc			|
   |		|     stack(128k default)	 	|
   |		|								|
   |		|================================
   |		|			......				|
   |		|================================
   |		|								|
 module x	|			 heap				|
memory size	|			(global gp)			|
   |		|================================
   |		|								|
   |		|		rdata segment			|
   |		|								|
   |		|================================
   |		|           					|
   |		|       text segment			|
module_base	|								|
=============================================
Machine Execute abi
*opcode --- ["load","run","pause","reset","stop","get_state","get_registers","set_register","set_memorysize","set_stacksize","read_memory","write_memory","breakpoint","memory_breakpoint","call",]

//id(dword) bin(abi) address/ip(dword)

param bin(abi) --- module_name(string)
              |
               --- export --- name(abi)---address(ip)
			  |
			   --- import --- name(abi)---address(ip)
			  |
			   --- text(data)
			  |
			   --- rdata(data)



opcode load --- bin(abi) --> return_abi: return  -- opcode(string)
		   |							   |
			--- id(dword)					--- "ok"/"error message"
                                           |
                                            ---id(dword)

opcode run ---id(dword) --> return_abi: return  -- opcode(string)
											   |
												--- "ok"/"error message"
											   |
												---id(dword)

opcode pause---id(dword) --> return_abi: return  -- opcode(string)
												|
												 --- "ok"/"error message"
												|
												 ---id(dword)

opcode step  --tick(dword) --> return_abi: return  -- opcode(string)
			|			     					 |
			 --id(dword)						  --- "ok"/"error message"
												 |
												  ---id(dword)
												 |
												  ---ip(dword)

opcode reset---id(dword) --> return_abi: return  -- opcode(string)
												|
												 --- "ok"/"error message"
												|
												 ---id(dword)

opcode stop---id(dword) --> return_abi: return	 -- opcode(string)
												|
												 --- "ok"/"error message"
												|
												 ---id(dword)

opcode get_state---id(dword) --> return_abi:  -- opcode(string)
											|
											  --return "ok"/"error message"
											|
											  ---id(dword)
											|
											  ---state(string) ["idle","running","pause","error"]
											|
											  ---r[0~7](data array of px_dword)
											|
											  ---f[0~3](data array of px_float32)
											|
											  ---ip_breakpoint(data array of px_dword)

opcode set_memorysize---id(dword) --- return_abi: return  --- "ok"/"error message"
													     |
														   ---id(dword)
									   

opcode set_stacksize---id(dword) --> return_abi: return  --- "ok"/"error message"
					|									 |
					 --size(dword)						 ---id(dword)

opcode set_register---register(string) --> return_abi: return  --- "ok"/"error message"
				  |											  |
				   ---value(dword)							   --- id(dword)
				  |
				   ---id(dword)
																				               

opcode read_memory ---address(dword) --> return_abi: return    ---"ok"/"error message"
                   |                                          |
                    ---size(dword)                              --- data(px_byte[])
															  |
															   ---id(dword)

opcode write_memory(id, address, data) --- return_abi: return   ---"ok"/"error message"
															  |
															    ---id(dword)

opcode breakpoint(id, ip) --- return_abi: return   ---"ok"/"error message"
												  |
												   ---id(dword)

opcode memory_breakpoint(id, address, size, mask) --- return_abi: return   ---"ok"/"error message"
															  |
															    ---id(dword)

opcode call(id, address) --- return_abi: return   ---"ok"/"error message"
												 |
												   ---id(dword)
*/
