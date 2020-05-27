#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "expr.h"
#include "token.h"
#include "util.h"

static void enqueue(struct compiler *comp, struct expression **head, struct expression **tail, struct expression operand);
static void push(struct compiler *comp, struct stacknode **top, char c);
static char pop(struct compiler *comp, struct stacknode **top);
static char top(struct compiler *comp, struct stacknode *top);

/* Convert infix expression in s into postfix expression in *expr list */
struct expression *
getexpr(struct compiler *comp)
{
	struct expression *head, *tail;
	struct stacknode *stack;
	enum tokentype toktype;
	union tokenvalue tok;

	head = tail = NULL;
	stack = NULL;

	push(comp, &stack, '(');
	while (isexpression(toktype = gettoken(comp, &tok))) {
		switch (toktype) {
		case VARIABLE:
			enqueue(comp, &head, &tail, (struct expression) {.type = symb, .u.s = strdup(tok.s)});
			break;
		case CONSTANT:
			enqueue(comp, &head, &tail, (struct expression) {.type = num, .u.n = tok.n});
			break;
		case LEFTPAREN:
			push(comp, &stack, '(');
			break;
		case RIGHTPAREN:
			while (isarithmetic(top(comp, stack))) {
				enqueue(comp, &head, &tail, (struct expression) {.type = op, .u.c = pop(comp, &stack)});
			}
			if (top(comp, stack) == '(')
				pop(comp, &stack);
			break;
		case ARITHMETIC:
			while (isarithmetic(top(comp, stack))
			       && isarithmetic(top(comp, stack)) >= isarithmetic(tok.c)) {
				enqueue(comp, &head, &tail, (struct expression) {.type = op, .u.c = pop(comp, &stack)});
			}
			push(comp, &stack, tok.c);
			break;
		default:
			break;
		}
	}
	while (isarithmetic(top(comp, stack))) {
		enqueue(comp, &head, &tail, (struct expression) {.type = op, .u.c = pop(comp, &stack)});
	}
	ungettoken(comp, toktype, tok);

	if(stack->next != NULL || stack->op != '(')
		syntaxerr(comp, "improper expression, %c left", stack->op);
	return head;
}

/* enqueue an operand or operator (op) into the expression list */
static void
enqueue(struct compiler *comp, struct expression **head, struct expression **tail, struct expression op)
{
	struct expression *p;

	if ((p = malloc(sizeof *p)) == NULL)
		comperr(comp, "malloc returned NULL");

	p->type = op.type;
	if (p->type == num)
		p->u.n = op.u.n;
	else if (p->type == symb)
		p->u.s = op.u.s;
	else
		p->u.c = op.u.c;
	p->next = NULL;

	if (*head == NULL)
		*head = p;
	else
		(*tail)->next = p;
	*tail = p;
}

/* push an operator into expression stack */
static void
push(struct compiler *comp, struct stacknode **top, char c)
{
	struct stacknode *p;

	if ((p = malloc(sizeof *p)) == NULL)
		comperr(comp, "malloc returned NULL");

	p->op = c;
	p->next = *top;
	*top = p;
}

/* pop an operator from expression stack */
static char
pop(struct compiler *comp, struct stacknode **top)
{
	struct stacknode *tmp;
	char retval;

	if (*top == NULL)
		comperr(comp, "trying to pop empty stack");

	tmp = *top;
	retval = (*top)->op;
	*top = (*top)->next;
	free(tmp);

	return retval;
}

/* get top operator from expression stack */
static char
top(struct compiler *comp, struct stacknode *top)
{
	if (top == NULL)
		comperr(comp, "trying to get element from empty stack");

	return top->op;
}
