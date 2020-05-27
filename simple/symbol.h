/* search for symbol in symbol table */
struct symbol *lookupsymbol(struct compiler *comp, char *s);

/* insert symbol in symbol table */
struct symbol *installsymbol(struct compiler *comp, char *s, int type, size_t loc);
