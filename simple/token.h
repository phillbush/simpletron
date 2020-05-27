#include <stdbool.h>

#define TOKENSIZE 63

enum tokentype {
	ENDOFFILE = 0,
	COMMENT,
	CONSTANT,
	RELATIONAL,
	ARITHMETIC,
	ASSIGNMENT,
	GOTOKEYWRD,
	COMMA,
	VARIABLE,
	EXPRESSION,
	LABEL,
	NEWLINE,
	COMMAND,
	LEFTPAREN,
	RIGHTPAREN,
	UNKNOWN
};

union tokenvalue {
	char c;
	char *s;
	memtype n;
};

/* return precedence of operator c, or 0 if it is not an operator */
int isarithmetic(char c);

/* return whether s is a command */
bool iscommand(char *s);

/* check whether a token is a valid expression operand or operator */
bool isexpression(enum tokentype toktype);

/* get next token, return the type of it and set *tok to it */
enum tokentype gettoken(struct compiler *comp, union tokenvalue *tok);

/* unget a token */
void ungettoken(struct compiler *comp, enum tokentype toktype, union tokenvalue tok);

/* check if expected and got token are different, exit with error if they are */
void checktoken(struct compiler *comp, enum tokentype expected, enum tokentype got);

/* check if expected and got commands are different, exit with error if they are */
void checkcommand(struct compiler *comp, char *expected, char *got);
