#ifdef TOKENIZER_H
#define TOKENIZER_H

void tokenizer(char *str, const char *delim, char **token);
void tokenizer_r(char *str, const char *delim, char **saveptr, char **token);

#endif //TOKENIZER_H