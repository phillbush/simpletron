#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "instruction.h"
#include "token.h"
#include "symbol.h"
#include "expr.h"
#include "util.h"

static void command_input(struct compiler *);
static void command_let(struct compiler *);
static void command_print(struct compiler *);
static void command_goto(struct compiler *);
static void command_if(struct compiler *);
static void command_end(struct compiler *);
static size_t evaluateexpr(struct compiler *, struct expression *);

/* return pointer to function that generates machine instructions for command s */
instruction
getinstruction(char *s)
{
	if (strcasecmp(s, "INPUT") == 0)
		return command_input;
	if (strcasecmp(s, "PRINT") == 0)
		return command_print;
	if (strcasecmp(s, "LET") == 0)
		return command_let;
	if (strcasecmp(s, "GOTO") == 0)
		return command_goto;
	if (strcasecmp(s, "IF") == 0)
		return command_if;
	if (strcasecmp(s, "END") == 0)
		return command_end;

	return NULL;
}

/* generate machine instructions for let statement */
static void
command_let(struct compiler *comp)
{
	struct symbol *sym;
	union tokenvalue tok;
	enum tokentype toktype;
	size_t var, result;
	struct expression *expr;

	/* get location of variable to be assigned by let statement */
	toktype = gettoken(comp, &tok);             /* get variable symbol */
	checktoken(comp, VARIABLE, toktype);        /* check whether got a variable */
	sym = lookupsymbol(comp, tok.s);            /* search in symbol table*/
	if (sym == NULL)                            /* if not found, insert it*/
		sym = installsymbol(comp, tok.s, variable, comp->datacount--);
	var = sym->location;

	/* get equal sign */
	toktype = gettoken(comp, &tok);
	checktoken(comp, ASSIGNMENT, toktype);

	/* get expression to assign */
	expr = getexpr(comp);

	/* generate machine instructions */
	result = evaluateexpr(comp, expr);
	comp->sml[comp->inscount++] = LOAD * MEMSIZE + result;
	comp->sml[comp->inscount++] = STORE * MEMSIZE + var;
}

/* generate machine instructions for input statement */
static void
command_input(struct compiler *comp)
{
	union tokenvalue tok;
	enum tokentype toktype;
	struct symbol *sym;

	do {
		/* get location of variable to be input */
		toktype = gettoken(comp, &tok);   /* get variable name */
		checktoken(comp, VARIABLE, toktype);
		sym = lookupsymbol(comp, tok.s);            /* search in symbol table */
		if (sym == NULL)                            /* if not found, insert it*/
			sym = installsymbol(comp, tok.s, variable, comp->datacount--);

		/* generate machine instruction */
		comp->sml[comp->inscount++] = READ * MEMSIZE + sym->location;
	} while ((toktype = gettoken(comp, &tok)) == COMMA);

	ungettoken(comp, toktype, tok);
}

/* generate machine instructions for print statement */
static void
command_print(struct compiler *comp)
{
	union tokenvalue tok;
	enum tokentype toktype;
	struct symbol *sym;

	do {
		/* get location of variable to print */
		toktype = gettoken(comp, &tok);   /* get variable name */
		checktoken(comp, VARIABLE, toktype);
		sym = lookupsymbol(comp, tok.s);            /* search in symbol table */
		if (sym == NULL)                            /* if not found, it's an error */
			syntaxerr(comp, "'%s' undeclared", tok.s);

		/* generate machine instruction */
		comp->sml[comp->inscount++] = WRITE * MEMSIZE + sym->location;
	} while ((toktype = gettoken(comp, &tok)) == COMMA);

	ungettoken(comp, toktype, tok);
}

/* generate machine instructions for goto statement */
static void
command_goto(struct compiler *comp)
{
	union tokenvalue tok;
	enum tokentype toktype;
	struct symbol *sym;
	size_t labeladdr;

	/* get location to go to */
	labeladdr = 0;
	toktype = gettoken(comp, &tok);             /* get label name */
	checktoken(comp, VARIABLE, toktype);
	sym = lookupsymbol(comp, tok.s);            /* search in symbol table */
	if (sym == NULL)                            /* if label not found, flag it*/
		comp->flag[comp->inscount] = strdup(tok.s);
	else                                        /* if label is found, set it */
		labeladdr = sym->location;

	/* generate machine instruction */
	comp->sml[comp->inscount++] = BRANCH * MEMSIZE + labeladdr;
}

/* generate machine instructions for if statement */
static void
command_if(struct compiler *comp)
{
	union tokenvalue tok;
	char relop[3];      /* relational operator */
	enum tokentype toktype;
	struct symbol *sym;
	struct expression *expr;
	size_t op1, op2, labeladdr;

	/* get location of first variable */
	expr = getexpr(comp);
	op1 = evaluateexpr(comp, expr);

	/* get relational operator */
	toktype = gettoken(comp, &tok);
	checktoken(comp, RELATIONAL, toktype);
	strncpy(relop, tok.s, 2);
	relop[2] = '\0';

	/* get location of second variable */
	expr = getexpr(comp);
	op2 = evaluateexpr(comp, expr);

	/* get obligatory comma and 'goto' keyword */
	toktype = gettoken(comp, &tok);
	checktoken(comp, COMMA, toktype);
	toktype = gettoken(comp, &tok);
	if (toktype == COMMAND)
		toktype = GOTOKEYWRD;
	checktoken(comp, GOTOKEYWRD, toktype);
	checkcommand(comp, "goto", tok.s);

	/* get address of label to go to */
	labeladdr = 0;
	toktype = gettoken(comp, &tok);      /* get label */
	checktoken(comp, VARIABLE, toktype);
	sym = lookupsymbol(comp, tok.s);            /* search in symbol table */
	if (sym != NULL)                            /* if label is found, set it */
		labeladdr = sym->location;

	/* generate instructions based on branch type */
	if (strcmp(relop, "==") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op1;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op2;
		comp->sml[comp->inscount++] = BRANCHZERO * MEMSIZE + labeladdr;
		if (sym == NULL)                            /* if label not found, flag it */
			comp->flag[comp->inscount-1] = strdup(tok.s);
	} else if (strcmp(relop, "!=") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op1;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op2;
		comp->sml[comp->inscount++] = BRANCHZERO * MEMSIZE + 2;
		comp->sml[comp->inscount++] = BRANCH * MEMSIZE + labeladdr;
		if (sym == NULL)                            /* if label not found, flag it */
			comp->flag[comp->inscount-1] = strdup(tok.s);
	} else if (strcmp(relop, "<") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op1;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op2;
		comp->sml[comp->inscount++] = BRANCHNEG * MEMSIZE + labeladdr;
		if (sym == NULL)                            /* if label not found, flag it */
			comp->flag[comp->inscount-1] = strdup(tok.s);
	} else if (strcmp(relop, ">") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op2;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op1;
		comp->sml[comp->inscount++] = BRANCHNEG * MEMSIZE + labeladdr;
		if (sym == NULL)                            /* if label not found, flag it */
			comp->flag[comp->inscount-1] = strdup(tok.s);
	} else if (strcmp(relop, "<=") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op1;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op2;
		comp->sml[comp->inscount++] = BRANCHNEG * MEMSIZE + labeladdr;
		comp->sml[comp->inscount++] = BRANCHZERO * MEMSIZE + labeladdr;
		if (sym == NULL) {                          /* if label not found, flag it */ 
			comp->flag[comp->inscount-1] = strdup(tok.s);
			comp->flag[comp->inscount-2] = strdup(tok.s);
		}
	} else if (strcmp(relop, ">=") == 0) {
		comp->sml[comp->inscount++] = LOAD * MEMSIZE + op2;
		comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op1;
		comp->sml[comp->inscount++] = BRANCHNEG * MEMSIZE + labeladdr;
		comp->sml[comp->inscount++] = BRANCHZERO * MEMSIZE + labeladdr;
		if (sym == NULL) {                          /* if label not found, flag it */
			comp->flag[comp->inscount-1] = strdup(tok.s);
			comp->flag[comp->inscount-2] = strdup(tok.s);
		}
	}


}

/* generate machine instructions for end statement */
static void
command_end(struct compiler *comp)
{

	comp->sml[comp->inscount++] = HALT * MEMSIZE + 0;
}

/*
 * generate machine instructions for expression evaluation,
 * return memory location of the result
 */
static size_t
evaluateexpr(struct compiler *comp, struct expression *p)
{
	size_t i;
	size_t op1, op2;
	struct symbol *sym;
	struct expression *tmp;
	size_t *stack, size;

	size = 100;
	stack = calloc(size, sizeof *stack);

	i = 0;
	while (i < size && p != NULL) {

		if (p->type == num) {
			comp->sml[comp->datacount] = p->u.n;
			stack[i++] = comp->datacount--;
		} else if (p->type == symb) {
			if ((sym = lookupsymbol(comp, p->u.s)) == NULL)
				syntaxerr(comp, "'%s' undeclared", p->u.s);
			free(p->u.s);
			stack[i++] = sym->location;
		} else {
			if (i < 2)
				syntaxerr(comp, "improper expression");
			op2 = stack[--i];
			op1 = stack[--i];
			comp->sml[comp->inscount++] = LOAD * MEMSIZE + op1;

			switch (p->u.c) {
			case '+':
				comp->sml[comp->inscount++] = ADD * MEMSIZE + op2;
				break;
			case '-':
				comp->sml[comp->inscount++] = SUBTRACT * MEMSIZE + op2;
				break;
			case '*':
				comp->sml[comp->inscount++] = MULTIPLY * MEMSIZE + op2;
				break;
			case '/':
				comp->sml[comp->inscount++] = DIVIDE * MEMSIZE + op2;
				break;
			case '%':
				comp->sml[comp->inscount++] = REMAINDER * MEMSIZE + op2;
				break;
			default:
				/* unreachable */
				break;
			}

			comp->sml[comp->inscount++] = STORE * MEMSIZE + comp->datacount;
			stack[i++] = comp->datacount--;
		}
		tmp = p;
		p = p->next;
		free(tmp);
	}
	if (i != 1)
		syntaxerr(comp, "improper expression");

	return stack[--i];
}
