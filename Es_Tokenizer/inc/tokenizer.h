#ifdef TOKENIZER_H
#define TOKENIZER_H

char *tokenizer(char *str, const char *delim);
char *tokenizer_r(char *str, const char *delim, char **saveptr);

#endif //TOKENIZER_H