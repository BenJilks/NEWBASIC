#ifndef LINKER_H
#define LINKER_H

void linker_init();
void linker_add_code(const char *code, int len);
char *linker_link(int *len);
int linker_find_addr(const char *name);
void linker_close();

#endif // LINKER_H

