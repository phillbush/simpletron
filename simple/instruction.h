typedef void (*instruction)(struct compiler *);

/* return pointer to function that generates machine instructions for command s */
instruction getinstruction(char *s);
