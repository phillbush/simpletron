#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "util.h"

/* print current file, line number and syntax error on stderr */
void
syntaxerr(struct compiler *comp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%lu: error: ", comp->file, comp->ln);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	cleanup(comp);
	exit(EXIT_FAILURE);
}

/* print compiler name and compilation error on stderr */
void
comperr(struct compiler *comp, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	fprintf(stderr, "simple: error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	cleanup(comp);
	exit(EXIT_FAILURE);
}

/* free compiler memory */
void
cleanup(struct compiler *comp)
{
	size_t i;
	struct symbol *p, *tmp;

	for (i = 0; i < HASHSIZE; i++) {
		p = comp->hashtab[i];
		while (p != NULL) {
			tmp = p;
			p = p->next;
			free(tmp->name);
			free(tmp);
		}
	}

	free(comp->hashtab);

	free(comp->sml);

	for (i = 0; i < MEMSIZE; i++) {
		if (comp->flag[i] != NULL) {
			free(comp->flag[i]);
		}
	}
	free(comp->flag);
}
