#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "symbol.h"
#include "util.h"

static size_t hash(char *s);

/* search for symbol in symbol table */
struct symbol *
lookupsymbol(struct compiler *comp, char *s)
{
	struct symbol *p;

	for (p = comp->hashtab[hash(s)]; p != NULL; p = p->next) {
		if (strcmp(s, p->name) == 0)
			return p;
	}
	return NULL;
}

/* insert symbol in symbol table */
struct symbol *
installsymbol(struct compiler *comp, char *s, int type, size_t loc)
{
	struct symbol *p;
	size_t hashval;

	if ((p = lookupsymbol(comp, s)) == NULL) {           /* not found */
		if ((p = malloc(sizeof *p)) == NULL)
			comperr(comp, "malloc returned NULL");
		if ((p->name = strdup(s)) == NULL)
			comperr(comp, "strdup returned NULL");
		hashval = hash(s);
		p->next = comp->hashtab[hashval];
		p->type = type;
		p->location = loc;
		comp->hashtab[hashval] = p;
	}
	return p;
}

/*
 * form hash value for string s
 * algorithm from K&R, second edition
 */
static size_t
hash(char *s)
{
	size_t hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = *s + 31 * hashval;
	hashval %= HASHSIZE;
	return hashval;
}
