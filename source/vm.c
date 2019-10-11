#include "vm.h"
#include "bytecode.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

// Debug functions
#define DEBUG_REGISTERS 0
#define DEBUG_CODE 	0

#if DEBUG_CODE
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...) ;
#endif

// Memory sizes
#define CODE_SIZE 	1024
#define MEMORY_SIZE	100
#define REGISTER_SIZE	10
#define PC_LOC		REGISTER_SIZE + 0
#define SP_LOC		REGISTER_SIZE + 1

// Interupts
#define INT_PRINT 	0

// Flags
#define FLAG_EQUAL 	0b100
#define FLAG_LESS_THAN	0b010
#define FLAG_MORE_THAN	0b001

// Helper functions
#define R(i)			registers[i]
#define PC 			R(PC_LOC).i
#define SP			R(SP_LOC).i
#define RA			R(code[PC])
#define RB			R(code[PC+1])
#define RC			R(code[PC+2])
#define NEXT_BYTE		code[PC++]
#define NEXT_REGISTER		R(NEXT_BYTE)
#define NEXT_CONST		next_const()
#define NEXT_DATA(out, type)	memcpy(&out, code + PC, sizeof(type)); PC += sizeof(type)
#define NEXT_ADDR		NEXT_DATA(addr, int);
#define ADDR(offset)		memcpy(&addr, code + PC + offset, sizeof(int));

typedef struct Register
{
	char type;
	union
	{
		int i;
		float f;
		char *str;
	};
} Register;

static char 	*code;
static Register *memory;
static Register registers[REGISTER_SIZE + 2];
static char flags;
static int addr;

void vm_init()
{
	// Init memory and registers
	code = malloc(CODE_SIZE);
	memory = malloc(sizeof(Register) * MEMORY_SIZE);
	memset(code, 0, CODE_SIZE);
	memset(registers, 0, REGISTER_SIZE + 2);
}

void vm_load(int offset, const char *program, int len)
{
	// Copy code into memory
	memcpy(code + offset, program, len);
}

static Register next_const()
{
	Register data;
	data.type = code[PC++];

	switch (data.type)
	{
		case CONST_INT: NEXT_DATA(data.i, int); break;
		case CONST_STRING: data.str = code + PC + 1; PC += code[PC] + 2; break;
		default: break; // Do error
	}
	
	return data;
}

static void print_register(Register r)
{
	switch (r.type)
	{
		case CONST_INT: printf("%i\n", r.i); break;
		case CONST_STRING: printf("%s\n", r.str); break;
		default: printf("\n"); break; // Do error
	}
}

static void run_int(int id)
{
	switch (id)
	{
		case INT_PRINT: print_register(R(0)); break;
		default: break; // Do error
	}
}

#define SET_FLAGS(a, b) \
	flags = 0; \
	flags |= (a) == (b) ? FLAG_EQUAL : 0; \
	flags |= (a) < (b) ? FLAG_LESS_THAN : 0; \
	flags |= (b) > (b) ? FLAG_MORE_THAN : 0;

#define OPERATION(name, out, op) \
	static out name(Register a, Register b) \
	{ \
		switch (a.type) \
		{ \
			case CONST_INT: \
				switch (b.type) \
				{ \
					case CONST_INT: op(a.i, b.i); break; \
				} \
				break; \
		} \
	}

#define OP(a, b, op) return (Register) { CONST_INT, (a) op (b) }
#define OP_ADD(a, b) OP(a, b, +)
#define OP_SUB(a, b) OP(a, b, -)
#define OP_MUL(a, b) OP(a, b, *)
#define OP_DIV(a, b) OP(a, b, /)
OPERATION(op_compare, void, SET_FLAGS);
OPERATION(op_add, Register, OP_ADD);
OPERATION(op_sub, Register, OP_SUB);
OPERATION(op_mul, Register, OP_MUL);
OPERATION(op_div, Register, OP_DIV);

#define IMPLEMENT_OP(func, name) \
	case BC_##name##_RRC: { int a = NEXT_BYTE, b = NEXT_BYTE; R(a) = func(R(b), NEXT_CONST); } break; \
	case BC_##name##_RRR: RA = func(RB, RC); PC += 3; break


void vm_run(int offset)
{
	// Run code starting at offset
	registers[PC_LOC].type = CONST_INT;
	registers[SP_LOC].type = CONST_INT;
	PC = offset;
	SP = 0;

	int is_running = 1;
	while (is_running)
	{
		char bytecode = code[PC++];
		LOG("%i: %s\n", PC, bytecode_names[bytecode]);
		
		switch (bytecode)
		{
			case BC_HULT: 	is_running = 0; break;
			case BC_INT_A:	NEXT_ADDR; run_int(addr); break;

			case BC_MOV_RR: RA = RB; PC += 2; break;
			case BC_MOV_RC: NEXT_REGISTER = NEXT_CONST; break;

			case BC_MOV_AR: NEXT_ADDR; memory[addr] = NEXT_REGISTER; break;
			case BC_MOV_AC: NEXT_ADDR; memory[addr] = NEXT_CONST; break;
			case BC_MOV_IR: memory[RA.i] = RB; PC += 2; break;
			case BC_MOV_IPR: memory[RA.i + code[PC+2]] = RB; PC += 3; break;
			case BC_MOV_ISR: memory[RA.i - code[PC+2]] = RB; PC += 3; break;

			case BC_MOV_IC: memory[NEXT_REGISTER.i] = NEXT_CONST; break;
			case BC_MOV_IPC: { int ra = NEXT_BYTE; Register c = NEXT_CONST; memory[R(ra).i + NEXT_BYTE] = c; break; }
			case BC_MOV_ISC: { int ra = NEXT_BYTE; Register c = NEXT_CONST; memory[R(ra).i - NEXT_BYTE] = c; break; }

			case BC_MOV_RA: ADDR(1); RA = memory[addr]; PC += sizeof(int) + 1; break;
			case BC_MOV_RI: RA = memory[RB.i]; PC += 2; break;
			case BC_MOV_RIP: RA = memory[RB.i + code[PC+2]]; PC += 3; break;
			case BC_MOV_RIS: RA = memory[RB.i - code[PC+2]]; PC += 3; break;

			case BC_CMP_RC: { Register r = NEXT_REGISTER; op_compare(r, NEXT_CONST); } break;
			case BC_CMP_RR: op_compare(RA, RB); PC += 2; break;

			case BC_PUSH_R: memory[SP++] = NEXT_REGISTER; break;
			case BC_PUSH_C: memory[SP++] = NEXT_CONST; break;
			case BC_POP_R: NEXT_REGISTER = memory[--SP]; break;
			case BC_CALL_A: NEXT_ADDR; memory[SP++] = R(PC_LOC); PC = addr; break;
			case BC_RET: R(PC_LOC) = memory[--SP]; break;

			case BC_B_A: NEXT_ADDR; PC = addr; break;
			case BC_BEQ_A: NEXT_ADDR; if (flags & FLAG_EQUAL) PC = addr; break;
			case BC_BNE_A: NEXT_ADDR; if (!(flags & FLAG_EQUAL)) PC = addr; break;
			case BC_BLT_A: NEXT_ADDR; if (flags & FLAG_LESS_THAN) PC = addr; break;
			case BC_BGT_A: NEXT_ADDR; if (flags & FLAG_MORE_THAN) PC = addr; break;
			
			IMPLEMENT_OP(op_add, ADD);
			IMPLEMENT_OP(op_sub, SUB);
		}
		
#if DEBUG_REGISTERS
		int i;
		for (i = 0; i < REGISTER_SIZE + 2; i++)
		{
			printf("	=> R%i = ", i);
			print_register(registers[i]);
		}
		printf("	=> mem 0 = ");
		print_register(memory[0]);
#endif

	}
}

void vm_close()
{
	free(code);
	free(memory);
}

