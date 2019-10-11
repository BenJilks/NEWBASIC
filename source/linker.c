#include "linker.h"
#include "bytecode.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CHUNK_SIZE 80

// Label data
struct Label
{
	char name[80];
	int refs[80];
	int ref_count, addr;
};

static struct Label *labels;
static int label_count;
static int label_max_len;

// Output code
static char *out_code;
static int code_pointer;
static int code_max_len;

// Helper functions
#define REG 		out_code[code_pointer++] = code[i++]
#define CONST		i += copy_len(code, i, skip_const(code, i))
#define ADDR		i += skip_addr(code, i)
#define INDIRECT 	REG
#define INDIRECT_PLUS	i += copy_len(code, i, 2)
#define INDIRECT_SUB	INDIRECT_PLUS

void linker_init()
{
	labels = malloc(sizeof(struct Label) * CHUNK_SIZE);
	label_max_len = CHUNK_SIZE;
	label_count = 0;

	out_code = NULL;
	code_pointer = 0;
	code_max_len = 0;
}

static struct Label *find_label(const char *name)
{
	int i;

	// Find label if it exists
	for (i = 0; i < label_count; i++)
		if (!strcmp(labels[i].name, name))
			return &labels[i];
	
	// Otherwise, create a new one
	struct Label *label = &labels[label_count++];
	strcpy(label->name, name);
	label->addr = -1;
	label->ref_count = 0;
	return label;
}

int skip_const(const char *code, int i)
{
	char type = code[i];
	switch (type)
	{
		case CONST_INT: memcpy(out_code + code_pointer, code + i, 5);  return 5;
		case CONST_STRING: return code[i + 1] + 3;
	}

	return 0;
}

int copy_len(const char * code, int i, int len)
{
	memcpy(out_code + code_pointer, code + i, len);
	code_pointer += len;
	return len;
}

int get_addr(const char *code, int i);
int skip_addr(const char *code, int i)
{
	if (code[i] == BC_GET_LABEL)
		return get_addr(code, i + 1) + 1;

	return copy_len(code, i, 4);
}

#define GEN_SKIP(type) type;
#define SKIP(name) case BC_##name: ARGS_##name(GEN_SKIP); break

int set_addr(const char *code, int i)
{
	int len = code[i];
	const char *name = code + i + 1;

	// Set the labels address
	struct Label *label = find_label(name);
	label->addr = code_pointer;

	return len + 2;
}

int get_addr(const char *code, int i)
{
	int len = code[i];
	const char *name = code + i + 1;

	// Store the ref
	struct Label *label = find_label(name);
	label->refs[label->ref_count++] = code_pointer;

	// Allocate space for the ref
	code_pointer += 4;

	return len + 2;
}

void linker_add_code(const char *code, int len)
{
	// If no code exists, allocate space for the new code
	if (out_code == NULL)
	{
		out_code = malloc(len);
		code_max_len = len;
	}

	int i = 0;
	while (i < len)
	{
		char bytecode = code[i++];
		out_code[code_pointer++] = bytecode;

		switch (bytecode)
		{
			SKIP(MOV_RR); SKIP(MOV_RC);
			SKIP(MOV_AR); SKIP(MOV_AC); 
			SKIP(MOV_IR); SKIP(MOV_IPR); SKIP(MOV_ISR); 
			SKIP(MOV_IC); SKIP(MOV_IPC); SKIP(MOV_ISC);
			SKIP(MOV_RA); SKIP(MOV_RI); SKIP(MOV_RIP); SKIP(MOV_RIS);
			SKIP(CMP_RC); SKIP(CMP_RR);
			SKIP(ADD_RRC); SKIP(ADD_RRR);
			SKIP(SUB_RRC); SKIP(SUB_RRR);
			SKIP(PUSH_R); SKIP(PUSH_C); SKIP(POP_R);
			SKIP(CALL_A);
			SKIP(B_A); SKIP(BEQ_A); SKIP(BNE_A); SKIP(BLT_A); SKIP(BGT_A);
			SKIP(INT_A);

			case BC_SET_LABEL: i += set_addr(code, i); break;
		}
	}
}

char *linker_link(int *len)
{
	int i, j;

	for (i = 0; i < label_count; i++)
	{
		struct Label label = labels[i];
		if (label.addr == -1)
			ERROR("Undefined reference '%s'", label.name);
		LOG("Linking '%s' (%i)\n", label.name, label.addr);

		for (j = 0; j < label.ref_count; j++)
		{
			memcpy(out_code + label.refs[j], 
				&label.addr, sizeof(int));
			LOG("	=> ref %i\n", label.refs[j]);
		}
	}

	*len = code_pointer;
	return out_code;
}

int linker_find_addr(const char *name)
{
	struct Label *label = find_label(name);
	return label->addr;
}

void linker_close()
{
	free(labels);
	if (out_code != NULL)
		free(out_code);
}
