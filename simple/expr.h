struct expression {
	enum {
		symb,
		num,
		op
	} type;
	union {
		memtype n;
		char c;
		char *s;
	} u;
	struct expression *next;
};

struct stacknode {
	char op;
	struct stacknode *next;
};

/* Convert infix expression in s into postfix expression in *expr list */
struct expression *getexpr(struct compiler *comp);
