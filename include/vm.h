#ifndef VM_H
#define VM_H

void vm_init();
void vm_load(int offset, const char *code, int len);
void vm_run(int offset);
void vm_close();

#endif // VM_H

