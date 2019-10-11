#include <stdio.h>
#include <stdlib.h>
#include "assembler.h"
#include "tokenizer.h"
#include "linker.h"
#include "vm.h"

void assemble_file(const char *file)
{
	int len;
	char *code;

	// Assemble code and add to linker
	tokenizer_open(file);
	code = assemble(&len);
	linker_add_code(code, len);
	
	// Clean up
	tokenizer_close();
	free(code);
}

int main()
{
	// Init
	vm_init();
	linker_init();

	// Assemble all code
	assemble_file("test.asm");
	
	// Link the code together
	int len, main_addr = linker_find_addr("start");
	char *code = linker_link(&len);

	// If no start point was found, start at the beginning
	if (main_addr == -1)
		main_addr = 0;

	// Load and run the code
	vm_load(0, code, len);
	vm_run(main_addr);

	// Clean up
	vm_close();
	linker_close();
	return 0;
}

