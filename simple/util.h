/* print current file, line number and syntax error on stderr */
void syntaxerr(struct compiler *comp, const char *fmt, ...);

/* print compiler name and compilation error on stderr */
void comperr(struct compiler *comp, const char *fmt, ...);

/* free compiler memory */
void cleanup(struct compiler *comp);
