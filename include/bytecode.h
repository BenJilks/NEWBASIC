#ifndef BYTECODE_H
#define BYTECODE_H

// Const types
#define CONST_NULL	0
#define CONST_INT 	1
#define CONST_FLOAT	2
#define CONST_STRING 	3

// Argument codes:
// 	R - Register
// 	A - Address
// 	C - Constant
// 	I - Indirect
// 		P - Plus constant
// 		S - Sub constant

#define BYTECODE(GEN) \
	GEN(BC_HULT), \
	GEN(BC_INT_A), \
	GEN(BC_MOV_RR), \
	GEN(BC_MOV_RC), \
	 \
	GEN(BC_MOV_AR), \
	GEN(BC_MOV_AC), \
	GEN(BC_MOV_IR), \
	GEN(BC_MOV_IC), \
	 \
	GEN(BC_MOV_RA), \
	GEN(BC_MOV_RI), \
	GEN(BC_MOV_RIP), \
	GEN(BC_MOV_RIS), \
	 \
	GEN(BC_CMP_RR), \
	GEN(BC_CMP_RC), \
	 \
	GEN(BC_ADD_RRR), \
	GEN(BC_ADD_RRC), \
	GEN(BC_SUB_RRR), \
	GEN(BC_SUB_RRC), \
	 \
	GEN(BC_PUSH_R), \
	GEN(BC_PUSH_C), \
	GEN(BC_POP_R), \
	GEN(BC_CALL_A), \
	GEN(BC_RET), \
	 \
	GEN(BC_B_A), \
	GEN(BC_BEQ_A), \
	GEN(BC_BNE_A), \
	GEN(BC_BGT_A), \
	GEN(BC_BLT_A), \
	 \
	GEN(BC_SET_LABEL), \
	GEN(BC_GET_LABEL)

#define ARGS_INT_A(GEN)		GEN(ADDR)
#define ARGS_MOV_RR(GEN)	GEN(REG) GEN(REG)
#define ARGS_MOV_RC(GEN)	GEN(REG) GEN(CONST)

#define ARGS_MOV_AR(GEN)	GEN(ADDR) GEN(REG)
#define ARGS_MOV_AC(GEN)	GEN(ADDR) GEN(CONST)
#define ARGS_MOV_IR(GEN)	GEN(INDIRECT) GEN(REG)
#define ARGS_MOV_IC(GEN)	GEN(INDIRECT) GEN(CONST)

#define ARGS_MOV_RA(GEN)	GEN(ADDR) GEN(REG)
#define ARGS_MOV_RI(GEN)	GEN(REG) GEN(INDIRECT)
#define ARGS_MOV_RIP(GEN)	GEN(REG) GEN(INDIRECT_PLUS)
#define ARGS_MOV_RIS(GEN)	GEN(REG) GEN(INDIRECT_SUB)

#define ARGS_CMP_RR(GEN)	GEN(REG) GEN(REG)
#define ARGS_CMP_RC(GEN)	GEN(REG) GEN(CONST)

#define ARGS_ADD_RRR(GEN)	GEN(REG) GEN(REG) GEN(REG)
#define ARGS_ADD_RRC(GEN)	GEN(REG) GEN(REG) GEN(CONST)
#define ARGS_SUB_RRR(GEN)	GEN(REG) GEN(REG) GEN(REG)
#define ARGS_SUB_RRC(GEN)	GEN(REG) GEN(REG) GEN(CONST)

#define ARGS_PUSH_R(GEN)	GEN(REG)
#define ARGS_PUSH_C(GEN)	GEN(CONST)
#define ARGS_POP_R(GEN)		GEN(REG)
#define ARGS_CALL_A(GEN)	GEN(ADDR)

#define ARGS_B_A(GEN)		GEN(ADDR)
#define ARGS_BEQ_A(GEN)		GEN(ADDR)
#define ARGS_BNE_A(GEN)		GEN(ADDR)
#define ARGS_BLT_A(GEN)		GEN(ADDR)
#define ARGS_BGT_A(GEN)		GEN(ADDR)

#define GEN_ENUM(name) 		name
#define GEN_STRING(name) 	#name

enum Bytecode { BYTECODE(GEN_ENUM) };
static const char *bytecode_names[] = { BYTECODE(GEN_STRING) };

#endif // BYTECODE_H

