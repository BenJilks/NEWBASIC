#ifndef TOKENIZER_H
#define TOKENIZER_H

void tokenizer_open(const char *file_path);

char tokenizer_next();
void tokenizer_push_back(char c);
void tokenizer_skip_white_space();
void tokenizer_read_until(char *out, char until, int ignore_space);
void tokenizer_word(char *out);
int tokenizer_read_int();

int tokenizer_has_next();
void tokenizer_close();

#endif // TOKENIZER_H

