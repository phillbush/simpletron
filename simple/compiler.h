#include "memory.h"

#define HASHSIZE 101

struct symbol {
	enum {
		label,
		variable,
		none
	} type;
	char *name;
	size_t location;
	struct symbol *next;
};

struct compiler {
	size_t memsize;
	struct symbol **hashtab; /* the symbol table */
	memtype *sml;            /* the sml instructions */
	char **flag;             /* the flag array */
	char *file;              /* name of file to be compiled*/
	size_t ln;               /* current line of file to be compiled */
	size_t inscount;
	size_t datacount;
};
