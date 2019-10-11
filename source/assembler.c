#include "assembler.h"
#include "tokenizer.h"
#include "bytecode.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define CHUNK_SIZE 80

// Instructions
#define INST_ERROR 	0
#define INST_MOV 	1
#define INST_CMP	2
#define INST_ADD	3
#define INST_SUB	4
#define INST_B		5
#define INST_BEQ	6
#define INST_BNE	7
#define INST_BGT	8
#define INST_BLT	9
#define INST_LET	10
#define INST_INT	11
#define INST_HLT	12
#define INST_PUSH	13
#define INST_POP	14
#define INST_CALL	15
#define INST_RET	16

// Arg types
#define ARG_REG		0
#define ARG_CONST	1
#define ARG_ADDR	2
#define ARG_INDIRECT	3

// Named registers
static char *named_registers[] = 
{
	"PC", "SP"
};

// Assembly data
static char 	*code;
static int 	pointer;
static int 	len;

// Arg data
struct Arg
{
	int type;

	int const_type;
	int is_addr_label;

	union
	{
		int reg_id;
		int const_i;
		int addr;
	};

	union
	{
		char const_str[80];
		char addr_label[80];
	};
};
static char 		instruction_name[80];
static struct Arg 	args[3];
static int 		arg_count;

static char read_instruction()
{
	char *name = instruction_name;
	tokenizer_word(name);

	if (!strcmp(name, "MOVE")) return INST_MOV;
	if (!strcmp(name, "COMPARE")) return INST_CMP;
	if (!strcmp(name, "ADD")) return INST_ADD;
	if (!strcmp(name, "SUB")) return INST_SUB;
	if (!strcmp(name, "GOTO"))   return INST_B;
	if (!strcmp(name, "GOTO_IF_EQUAL")) return INST_BEQ;
	if (!strcmp(name, "GOTO_IF_NOT_EQUAL")) return INST_BNE;
	if (!strcmp(name, "GOTO_IF_GREATER_THAN")) return INST_BGT;
	if (!strcmp(name, "GOTO_IF_LESS_THAN")) return INST_BLT;
	if (!strcmp(name, "LET")) return INST_LET;
	if (!strcmp(name, "HULT")) return INST_HLT;
	if (!strcmp(name, "INTERUPT")) return INST_INT;
	if (!strcmp(name, "PUSH")) return INST_PUSH;
	if (!strcmp(name, "POP")) return INST_POP;
	if (!strcmp(name, "CALL")) return INST_CALL;
	if (!strcmp(name, "RETURN")) return INST_RET;
	return INST_ERROR;
}

static int get_named_reg(const char *name)
{
	int i;
	for (i = 0; i < sizeof(named_registers) / sizeof(char*); i++)
		if (!strcmp(name, named_registers[i]))
			return 10 + i;
	
	return 0;
}

static struct Arg read_constant(char c)
{
	struct Arg arg;
	char buffer[80];
	int buffer_pointer = 0;
	arg.type = ARG_CONST;

	if (isdigit(c))
	{
		// Read number
		while (isdigit(c) && tokenizer_has_next())
		{
			buffer[buffer_pointer++] = c;
			c = tokenizer_next();
		}
		tokenizer_push_back(c);
		buffer[buffer_pointer] = '\0';

		arg.const_type = CONST_INT;
		arg.const_i = atoi(buffer);
	}
	else if (isalpha(c))
	{
		// Read label
		while ((isdigit(c) || isalpha(c) || c == '_') && tokenizer_has_next())
		{
			buffer[buffer_pointer++] = c;
			c = tokenizer_next();
		}
		tokenizer_push_back(c);
		buffer[buffer_pointer] = '\0';

		int reg = get_named_reg(buffer);
		if (reg)
		{
			arg.reg_id = reg;
			arg.type = ARG_REG;
		}
		else
		{
			arg.is_addr_label = 1;
			strcpy(arg.addr_label, buffer);
			arg.type = ARG_ADDR;
		}
	}
	else if (c == '"')
	{
		// Read string constant
		while ((c = tokenizer_next()) != '"' && tokenizer_has_next())
			buffer[buffer_pointer++] = c;
		buffer[buffer_pointer] = '\0';
		
		arg.const_type = CONST_STRING;
		strcpy(arg.const_str, buffer);
	}

	LOG("%s, ", buffer);
	return arg;
}

static struct Arg read_addr()
{
	struct Arg arg;
	char buffer[80];
	int buffer_pointer = 0;
	char c;
	arg.type = ARG_ADDR;
	arg.is_addr_label = 0;

	// Read '#' followed by base 10 address
	while (isdigit(c = tokenizer_next()) && tokenizer_has_next())
		buffer[buffer_pointer++] = c;
	tokenizer_push_back(c);
	buffer[buffer_pointer] = '\0';
	arg.addr = atoi(buffer);

	LOG("#%i, ", arg.addr);
	return arg;
}

static struct Arg read_register()
{
	struct Arg arg;
	arg.type = ARG_REG;
	arg.reg_id = tokenizer_next() - '0';

	LOG("R%i, ", arg.reg_id);
	return arg;
}

static struct Arg read_indirect()
{
	struct Arg arg;
	char c;
	char buffer[80];

	tokenizer_skip_white_space();
	arg.type = ARG_INDIRECT;
	c = tokenizer_next();

	// If it starts with 'R', it's a register
	if (c == 'R')
	{
		arg.reg_id = tokenizer_next() - '0';
		return arg;
	}

	// Otherwise, check named registers
	buffer[0] = c;
	tokenizer_read_until(buffer + 1, ']', 0);
	arg.reg_id = get_named_reg(buffer);
	tokenizer_next(); // Skip ']'

	// If it's not a named register, then it 
	// can't be an indirect
	if (arg.reg_id == 0)
	{
		ERROR("Uknown register '%s'", buffer);
	}
}

static struct Arg read_arg(char c)
{
	switch (c)
	{
		case 'R': return read_register();
		case '#': return read_addr();
		case '[': return read_indirect();
		default: return read_constant(c);
	}
}

static void read_all_args()
{
	arg_count = 0;

	char c;
	while ((c = tokenizer_next()) != '\n' && tokenizer_has_next())
	{
		// Skip white space
		while (isspace(c) && c != '\n' && tokenizer_has_next())
			c = tokenizer_next();

		// If end of line, exit
		if (c == '\n' || !tokenizer_has_next())
			break;

		args[arg_count++] = read_arg(c);
	}
}

static void check_mem(int size)
{
	// If there's not enough space, then 
	// allocate another chunk
	while (pointer + size >= len)
	{
		len += CHUNK_SIZE;
		code = realloc(code, len);
	}
}

static void write_byte(char b)
{
	check_mem(1);
	code[pointer++] = b;
}

static void write_int(int i)
{
	check_mem(sizeof(int));
	memcpy(code + pointer, &i, sizeof(int));
	pointer += sizeof(int);
}

static void write_string(const char *str)
{
	int i, len = strlen(str);
	check_mem(len + 2);
	write_byte((char)len);
	
	for (i = 0; i < len + 1; i++)
		write_byte(str[i]);
}

static void write_reg(struct Arg arg)
{
	write_byte(arg.reg_id);
}

static void write_const(struct Arg arg)
{
	int i;
	write_byte((char)arg.const_type);

	switch (arg.const_type)
	{
		case CONST_INT: write_int(arg.const_i); break;
		case CONST_STRING: write_string(arg.const_str); break;
	}
}

static void write_addr(struct Arg arg)
{
	if (arg.is_addr_label)
	{
		write_byte(BC_GET_LABEL);
		write_string(arg.addr_label);
	}
	else
	{
		write_int(arg.addr);
	}
}

static void write_args()
{
	int i;
	for (i = 0; i < arg_count; i++)
	{
		struct Arg arg = args[i];
		switch (arg.type)
		{
			case ARG_REG: write_reg(arg); break;
			case ARG_CONST: write_const(arg); break;
			case ARG_ADDR: write_addr(arg); break;
			case ARG_INDIRECT: write_reg(arg); break;
		}
	}
}

static void read_label()
{
	char *name = instruction_name;
	name[strlen(name) - 1] = '\0';

	write_byte(BC_SET_LABEL);
	write_string(name);
}

static void read_let()
{
	char name[80];
	tokenizer_word(name);


}

struct Instruction
{
	char bytecode;
	int arg_size;
	int args[3];
};

struct InstructionGroup
{
	int type;
	int instruction_count;
	struct Instruction instructions[10];
};

#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N)
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N,...) N
#define PP_RSEQ_N 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define GEN_ARGS(type)		ARG_##type, 
#define GEN_ARG_LIST(name) 	{ ARGS_##name(GEN_ARGS) }
#define ARG_COUNT(name)		PP_NARG(GEN_ARG_LIST(name)) - 1
#define INSTRUCTION(name) 	{ BC_##name, ARG_COUNT(name), GEN_ARG_LIST(name) }

struct InstructionGroup instruction_groups[] = 
{
	{ INST_MOV, 8, { INSTRUCTION(MOV_RC), INSTRUCTION(MOV_RR), 
		INSTRUCTION(MOV_AR), INSTRUCTION(MOV_AC), 
		INSTRUCTION(MOV_IR), INSTRUCTION(MOV_IC), 
		INSTRUCTION(MOV_RA), INSTRUCTION(MOV_RI) } },
	{ INST_CMP, 2, { INSTRUCTION(CMP_RC), INSTRUCTION(CMP_RR) } },
	{ INST_ADD, 2, { INSTRUCTION(ADD_RRC), INSTRUCTION(ADD_RRR) } },
	{ INST_SUB, 2, { INSTRUCTION(SUB_RRC), INSTRUCTION(SUB_RRR) } },
	{ INST_B, 1, INSTRUCTION(B_A) },
	{ INST_BEQ, 1, INSTRUCTION(BEQ_A) },
	{ INST_BNE, 1, INSTRUCTION(BNE_A) },
	{ INST_BLT, 1, INSTRUCTION(BLT_A) },
	{ INST_BGT, 1, INSTRUCTION(BGT_A) },
	{ INST_INT, 1, INSTRUCTION(INT_A) },
	{ INST_HLT, 1, { BC_HULT, 0 } },
	{ INST_PUSH, 2, { INSTRUCTION(PUSH_R), INSTRUCTION(PUSH_C) } },
	{ INST_POP, 1, INSTRUCTION(POP_R) },
	{ INST_CALL, 1, INSTRUCTION(CALL_A) },
	{ INST_RET, 1, { BC_RET, 0 } }
};

#define INSTRUCTION_GROUP_SIZE sizeof(instruction_groups) / sizeof(instruction_groups[0])

static void parse_instruction_group(struct InstructionGroup group)
{
	int i, j;
	char bytecode = -1;

	for (i = 0; i < group.instruction_count; i++)
	{
		struct Instruction inst = group.instructions[i];
		int is_match = 1;

		for (j = 0; j < inst.arg_size; j++)
		{
			if (inst.args[j] != args[j].type)
			{
				is_match = 0;
				break;
			}
		}

		if (is_match)
		{
			bytecode = inst.bytecode;
			break;
		}
	}

	if (bytecode == -1)
	{
		ERROR("Invalid arguments");
		return;
	}

	LOG("%s\n", bytecode_names[bytecode]);
	write_byte(bytecode);
	write_args();
}

static void read_line()
{
	char inst = read_instruction();
	if (inst == INST_ERROR)
	{
		if (instruction_name[strlen(instruction_name)-1] == ':')
		{
			read_label();
			return;
		}

		if (strlen(instruction_name) > 0)
			ERROR("Uknown instruction '%s'", instruction_name);
		return;
	}
	else if (inst == INST_LET)
	{
		read_let();
		return;
	}

	LOG("%s [", instruction_name);
	read_all_args();
	LOG("] - ");

	int i;
	for (i = 0; i < INSTRUCTION_GROUP_SIZE; i++)
	{
		// Find the instruction group
		struct InstructionGroup group = instruction_groups[i];
		if (group.type == inst)
		{
			parse_instruction_group(group);
			break;
		}
	}
}

char *assemble(int *out_len)
{
	code = malloc(CHUNK_SIZE);
	len = CHUNK_SIZE;
	pointer = 0;

	while (tokenizer_has_next())	
		read_line();
	write_byte(BC_HULT);
	
	*out_len = pointer;
	return code;
}

