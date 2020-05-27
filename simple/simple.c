#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "compiler.h"
#include "instruction.h"
#include "token.h"
#include "symbol.h"
#include "util.h"

/* compilation passes */
static void initialize(struct compiler *, char *);
static void populate(struct compiler *);
static void optimize(struct compiler *);
static void resolve(struct compiler *);
static void assemble(struct compiler *, char *);

/* usage */
static void usage(void);

/* simpleBASIC to Simpletron Machine Language compiler */
int
main(int argc, char *argv[])
{
	struct compiler comp;
	int c;
	char *out = "a.out";
	bool dooptimize;

	dooptimize = false;
	while ((c = getopt(argc, argv, "Oo:")) != -1) {
		switch (c) {
		case 'O':
			dooptimize = true;
			break;
		case 'o':
			out = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	initialize(&comp, *argv);
	populate(&comp);
	if (dooptimize)
		optimize(&comp);
	resolve(&comp);
	assemble(&comp, out);
	cleanup(&comp);

	return EXIT_SUCCESS;
}

/* initialize symbol table, instruction array, flag array and counters */
static void
initialize(struct compiler *comp, char *filename)
{
	comp->hashtab = calloc(HASHSIZE, sizeof *comp->hashtab);
	comp->sml = reallocarray(NULL, MEMSIZE, sizeof *comp->sml);
	comp->flag = calloc(MEMSIZE, sizeof *comp->flag);
	comp->memsize = MEMSIZE;
	comp->file = filename;
	comp->ln = 1;
	comp->inscount = 0;
	comp->datacount = comp->memsize - 1;
}

/* populate symbol table, instruction array, and flag array */
static void
populate(struct compiler *comp)
{
	enum tokentype toktype;
	union tokenvalue tok;

	/* while there is a command, generate instructions for it */
	while ((toktype = gettoken(comp, &tok)) != ENDOFFILE) {

		/* ignore comments */
		if (toktype == COMMENT || toktype == NEWLINE)
			continue;

		/* if command is a label declaration, put it in symbol table */
		if (toktype == LABEL) {
			if (lookupsymbol(comp, tok.s) != NULL)
				syntaxerr(comp, "redeclaration of %s", tok.s);
			installsymbol(comp, tok.s, label, comp->inscount);
			continue;
		}

		/* generate instructions for statement */
		checktoken(comp, COMMAND, toktype);
		(*getinstruction(tok.s))(comp);

		/* get obligatory newline after statement */
		toktype = gettoken(comp, &tok);
		checktoken(comp, NEWLINE, toktype);

		/* if instructions overlap data, compilation run out of memory */
		if (comp->inscount > comp->datacount)
			comperr(comp, "compilation ran out of memory");
	}
}

/* optimize compilation by getting rid of redundant instructions */
static void
optimize(struct compiler *comp)
{
	memtype opcode0, opcode1, opcode2;  /* current, next and nextnext opcode */
	memtype operand0, operand1;         /* current and next operand */
	struct symbol *p;
	size_t i, j;

	for (i = 0; i < comp->inscount; i++) {
		opcode0  = comp->sml[i] / comp->memsize;
		operand0 = comp->sml[i] % comp->memsize;
		opcode1  = comp->sml[i+1] / comp->memsize;
		operand1 = comp->sml[i+1] % comp->memsize;
		opcode2 = comp->sml[i+2] / comp->memsize;

		if (operand0 == operand1 && opcode2 == STORE
		    && opcode0 == STORE && opcode1 == LOAD) {
			for (j = 0; j < HASHSIZE; j++)
				for (p = comp->hashtab[j]; p != NULL; p = p->next)
					if (p->type == label && p->location > i)
						p->location -= 2;
			for (j = i; j < comp->inscount; j++) {
				comp->sml[j] = comp->sml[j + 2];
				comp->flag[j] = comp->flag[j + 2];
			}
		}

	}
}

/* resolve memory locations flagged as incomplete */
static void
resolve(struct compiler *comp)
{
	struct symbol *sym;
	size_t i;

	/* traverse flag array looking for flagged memory locations */
	for (i = 0; i < comp->memsize; i++) {
		if (comp->flag[i] != NULL) {
			sym = lookupsymbol(comp, comp->flag[i]);
			if (sym == NULL)
				comperr(comp, "failed to find label %s", comp->flag[i]);
			if (sym->type != label)
				comperr(comp, "failed to find label %s", comp->flag[i]);
			comp->sml[i] += sym->location;
		}
	}
}

/* write machine code in memory to file */
static void
assemble(struct compiler *comp, char *file)
{
	FILE *fp;
	size_t i;

	if ((fp = fopen(file, "w")) == NULL)
		comperr(comp, "cannot open file %s", file);

	for (i = 0; i < comp->memsize; i++)
		fprintf(fp, "%+05d\n", comp->sml[i]);

	if (ferror(fp))
		comperr(comp, "error on file %s", file);
	fclose(fp);
}

static void
usage(void)
{
	(void) fprintf(stderr, "usage: simple [-O] [-o file.sml] file.simp\n");
	exit(EXIT_FAILURE);
}
