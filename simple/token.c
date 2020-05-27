#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "token.h"
#include "util.h"

static char *tokenstring[] = {
	[ENDOFFILE] = "end of file",
	[COMMENT] = "comment",
	[CONSTANT] = "literal number",
	[RELATIONAL] = "relational operator",
	[ARITHMETIC] = "arithmetic operator",
	[ASSIGNMENT] = "equal sign",
	[GOTOKEYWRD] = "goto",
	[COMMA] = "comma",
	[VARIABLE] = "variable",
	[EXPRESSION] = "expression",
	[LABEL] = "label",
	[NEWLINE] = "newline",
	[COMMAND] = "command",
	[LEFTPAREN] = "left parenthesis",
	[RIGHTPAREN] = "right parenthesis",
	[UNKNOWN] = "unknown token"
};

static enum tokentype ungottoktype = UNKNOWN;
static union tokenvalue ungottok;

static memtype getint(struct compiler *, FILE *fp);

/* return precedence of operator c, or 0 if it is not an operator */
int
isarithmetic(char c)
{
	switch (c) {
	case '+': case '-':
		return 1;
	case '/': case '*': case '%':
		return 2;
	default:
		break;
	}
	return 0;
}

/* check whether s is a command */
bool
iscommand(char *s)
{
	if (strcasecmp(s, "INPUT") == 0)
		return true;
	if (strcasecmp(s, "PRINT") == 0)
		return true;
	if (strcasecmp(s, "LET") == 0)
		return true;
	if (strcasecmp(s, "GOTO") == 0)
		return true;
	if (strcasecmp(s, "IF") == 0)
		return true;
	if (strcasecmp(s, "END") == 0)
		return true;

	return false;
}

/* check whether a token is a valid expression operand or operator */
bool
isexpression(enum tokentype toktype)
{
	switch (toktype) {
	case CONSTANT: case ARITHMETIC: case VARIABLE:
	case LEFTPAREN: case RIGHTPAREN:
		return true;
	default:
		return false;
	}
}

/* get next token, return the type of it and set *tok to it */
enum tokentype
gettoken(struct compiler *comp, union tokenvalue *tok)
{
	static char s[TOKENSIZE];
	static FILE *fp = NULL;
	enum tokentype retval;
	int c;
	size_t i;

	if (fp == NULL) {
		fp = fopen(comp->file, "r");
		if (fp == NULL)
			comperr(comp, "cannot open file %s", comp->file);
	}

	/* get blank on the beginning */
	while (isblank(c = getc(fp)))
		;

	i = 0;
	retval = UNKNOWN;

	/* try to get previously ungotten token */
	if (ungottoktype != UNKNOWN) {
		ungetc(c, fp);
		retval = ungottoktype;
		ungottoktype = UNKNOWN;
		*tok = ungottok;
		return retval;
	}

	switch (c) {
	case EOF:
		fclose(fp);
		retval = ENDOFFILE;
		break;
	case '\n':
		comp->ln++;
		retval = NEWLINE;
		break;
	case '(':
		retval = LEFTPAREN;
		break;
	case ')':
		retval = RIGHTPAREN;
		break;
	case ',':
		retval = COMMA;
		break;
	case ';':
		while (c != '\n')
			c = getc(fp);
		comp->ln++;
		retval = COMMENT;
		break;
	case '=':
		s[i++] = c;
		c = getc(fp);
		if (c == '=') {
			s[i++] = c;
			retval = RELATIONAL;
			tok->s = s;
		} else {
			ungetc(c, fp);
			retval = ASSIGNMENT;
		}
		s[i] = '\0';
		break;
	case '!':
		s[i++] = c;
		c = getc(fp);
		if (c != '=')
			syntaxerr(comp, "unexpected character '%c'", c);
		s[i++] = c;
		s[i] = '\0';
		retval = RELATIONAL;
		tok->s = s;
		break;
	case '<': case '>':
		s[i++] = c;
		c = getc(fp);
		if (c == '=')
			s[i++] = c;
		else
			ungetc(c, fp);
		s[i] = '\0';
		retval = RELATIONAL;
		tok->s = s;
		break;
	default:
		if (c != '-' && c != '+' && isarithmetic(c)) {
			tok->c = c;
			retval = ARITHMETIC;
		} else if (isalpha(c)) {
			do {
				s[i++] = c;
				c = getc(fp);
			} while (isalpha(c) && i < TOKENSIZE - 1);
			s[i] = '\0';

			if (isalpha(c))
				syntaxerr(comp, "token too long");

			if (c == ':') {
				retval = LABEL;
			} else if (iscommand(s)) {
				ungetc(c, fp);
				retval = COMMAND;
			} else {
				ungetc(c, fp);
				retval = VARIABLE;
			}
			tok->s = s;
		} else if (c == '-' || c == '+' || isdigit(c)) {
			int op;

			op = c;
			if (c == '-' || c == '+') {
				c = getc(fp);
				if (!isdigit(c)) {
					ungetc(c, fp);
					tok->c = op;
					retval = ARITHMETIC;
					break;
				}
			}
			ungetc(c, fp);
			tok->n = getint(comp, fp);
			if (op == '-')
				tok->n = -1 * tok->n;
			retval = CONSTANT;
		} else
			syntaxerr(comp, "unexpected character '%c'", c);
		break;
	}

	if (ferror(fp))
		comperr(comp, "error on file %s", comp->file);

	return retval;
}

/* unget a token */
void
ungettoken(struct compiler *comp, enum tokentype toktype, union tokenvalue tok)
{
	ungottoktype = toktype;
	ungottok = tok;
}

/* check if expected and got token are different, exit with error if they are */
void
checktoken(struct compiler *comp, enum tokentype expected, enum tokentype got)
{
	if (expected != got)
		syntaxerr(comp, "%s expected (got %s)\n", tokenstring[expected], tokenstring[got]);
}

/* check if expected and got commands are different, exit with error if they are */
void
checkcommand(struct compiler *comp, char *expected, char *got)
{
	if (strcasecmp(expected, got) != 0)
		syntaxerr(comp, "%s expected (got %s)\n", expected, got);
}

/* get an integer from the file */
static memtype
getint(struct compiler *comp, FILE *fp)
{
	int c;
	long n;

	n = 0;
	while (isdigit(c = getc(fp))) {
		n = n * 10 + c - '0';
		if (n < MEM_MIN || n > MEM_MAX)
			syntaxerr(comp, "integer overflow");
	}
	ungetc(c, fp);
	return (memtype) n;
}
