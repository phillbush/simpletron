#include <err.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include "simpletron.h"

enum exitval {
	ERR_NONE = 0,
	ERR_INPUT,
	ERR_DIVZERO,
	ERR_REMZERO,
	ERR_SIGOVERFLOW,
	ERR_INTOVERFLOW,
	ERR_INVALID,
	ERR_MEMORY
};

static const char *warnmsg[] = {
	"",
	"invalid input",
	"division by zero",
	"remainder by zero",
	"signed integer overflow",
	"integer overflow",
	"invalid instruction",
	"execution reached end of memory without halting"
};

static enum exitval execute(struct simpletron *, bool trace);
static void meminit(struct simpletron *);
static void load(struct simpletron *, char *);
static void dump(struct simpletron);
static void printwarn(struct simpletron simp, enum exitval eval);
static int getinstruction(FILE *, memtype *);
static void usage(void);

/* load a program in the Simpletron Machine Language into memory and execute it*/
int
main(int argc, char *argv[])
{
	enum exitval eval;
	struct simpletron simp;
	bool coredump, trace;
	int c;

	coredump = false;
	trace = false;
	while ((c = getopt(argc, argv, "cv")) != -1) {
		switch (c) {
		case 'c':
			coredump = true;
			break;
		case 'v':
			trace = true;
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

	meminit(&simp);                 /* initialize simpletron's memory */
	load(&simp, *argv);             /* load program into memory */
	eval = execute(&simp, trace);   /* execute instructions in memory */
	printwarn(simp, eval);          /* print warning message */
	if (coredump)                   /* do dump, if needed */
		dump(simp);

	return eval;
}

/* run instructions from memory; return 1 if error occurs, return 0 otherwise */
static enum exitval
execute(struct simpletron *simp, bool trace)
{
	simp->pc = 0;   /* memory location of next instruction */
	simp->acc = 0;  /* reset accumulator register */

	/* this loop implements the "instruction execution cycle" */
	/* simulation begins with the instruction in the location 00 and continues sequentially */
	while (simp->pc < simp->memsize) {
		simp->ir = simp->memory[simp->pc];
		simp->opcode = simp->ir / 100;  /* opcode is the leftmost two digits of instruction register*/
		simp->operand = simp->ir % 100; /* operand is the rightmost two digits of instruction register*/

		if (trace)
			fprintf(stderr, "%02lu: %+05d\n", simp->pc, simp->ir);

		/* this switch statement determine the operation to be performed */
		/* each case set the counter for next instruction accordingly */
		switch (simp->opcode) {
		case READ:
			if (getinstruction(stdin, &(simp->memory[simp->operand])) == 0)
				return ERR_INPUT;
			simp->pc++;
			break;
		case WRITE:
			printf("%+05d\n", simp->memory[simp->operand]);
			simp->pc++;
			break;
		case LOAD:
			simp->acc = simp->memory[simp->operand];
			simp->pc++;
			break;
		case STORE:
			simp->memory[simp->operand] = simp->acc;
			simp->pc++;
			break;
		case ADD:
			simp->acc += simp->memory[simp->operand];
			simp->pc++;
			break;
		case SUBTRACT:
			simp->acc -= simp->memory[simp->operand];
			simp->pc++;
			break;
		case DIVIDE:
			if (simp->memory[simp->operand] == 0)
				return ERR_DIVZERO;
			if (simp->acc == MEM_MIN && simp->memory[simp->operand] == -1)
				return ERR_SIGOVERFLOW;
			simp->acc /= simp->memory[simp->operand];
			simp->pc++;
			break;
		case MULTIPLY:
			simp->acc *= simp->memory[simp->operand];
			simp->pc++;
			break;
		case REMAINDER:
			if (simp->memory[simp->operand] == 0)
				return ERR_REMZERO;
			if (simp->acc == MEM_MIN && simp->memory[simp->operand] == -1) 
				return ERR_SIGOVERFLOW;
			simp->acc %= simp->memory[simp->operand];
			simp->pc++;
			break;
		case ADD_I:
			simp->acc += simp->operand;
			simp->pc++;
			break;
		case SUBTRACT_I:
			simp->acc -= simp->operand;
			simp->pc++;
			break;
		case DIVIDE_I:
			if (simp->operand == 0)
				return ERR_DIVZERO;
			if (simp->acc == MEM_MIN && simp->operand == -1)
				return ERR_SIGOVERFLOW;
			simp->acc /= simp->operand;
			simp->pc++;
			break;
		case MULTIPLY_I:
			simp->acc *= simp->operand;
			simp->pc++;
			break;
		case REMAINDER_I:
			if (simp->operand == 0)
				return ERR_REMZERO;
			if (simp->acc == MEM_MIN && simp->operand == -1)
				return ERR_SIGOVERFLOW;
			simp->acc %= simp->operand;
			simp->pc++;
			break;
		case BRANCH:
			simp->pc = simp->operand;
			break;
		case BRANCHNEG:
			if (simp->acc < 0)
				simp->pc = simp->operand;
			else
				simp->pc++;
			break;
		case BRANCHZERO:
			if (simp->acc == 0)
				simp->pc = simp->operand;
			else
				simp->pc++;
			break;
		case HALT:
			return ERR_NONE;
		default:
			return ERR_INVALID;
		}

		if (simp->acc < MEM_MIN || simp->acc > MEM_MAX)
			return ERR_INTOVERFLOW;
	}

	return ERR_MEMORY;
}

/* initialize simpletron memory */
static void
meminit(struct simpletron *simp)
{
	static memtype mem[MEMSIZE];

	simp->memsize = MEMSIZE;
	simp->memory = mem;
}

/* load memory from file */
static void
load(struct simpletron *simp, char *file)
{
	memtype instruction;
	FILE *fp;
	size_t i;

	fp = fopen(file, "r");
	if (fp == NULL)
		err(EXIT_FAILURE, "%s", file);

	for(i = 0; !feof(fp) && i < simp->memsize; i++) {
		if (getinstruction(fp, &instruction) == 0)
			errx(1, "improper program");
		simp->memory[i] = instruction;
	}

	fclose(fp);
}

/* write a core dump of memory and registers into stdout */
static void
dump(struct simpletron simp)
{
	size_t i, j;

	fprintf(stderr, "\nREGISTERS:\n"
	       "accumulator          %+05d\n"
	       "instruction register %+05d\n"
	       "program counter      %05lu\n"
	       "operation code       %02d\n"
	       "operand              %02d\n",
	       simp.acc, simp.ir, simp.pc, simp.opcode, simp.operand);
	fprintf(stderr, "\nMEMORY:\n"
	       "        0      1      2      3      4      5      6      7      8      9\n");
	for (i = 0; i < simp.memsize / 10; i++) {
		fprintf(stderr, "%2lu  ", i * 10);
		for (j = 0; j < simp.memsize / 10; j++)
			fprintf(stderr, "%+05d%s", simp.memory[(i*10)+j],
			       (j == simp.memsize / 10 - 1) ? "\n" : "  ");
	}
}

static void
printwarn(struct simpletron simp, enum exitval eval)
{
	if (eval == ERR_NONE)
		return;

	if (eval == ERR_INVALID)
		warnx("%+05d: %s", simp.ir, warnmsg[eval]);
	else
		warnx("%s", warnmsg[eval]);
}

/* get instruction from fp; return 0 if instruction is improper */
static int
getinstruction(FILE *fp, memtype *instruction)
{
	size_t i;
	int c, num, sign;

	num = 0;

	/* get initial blank */
	while (isspace(c = getc(fp)))
		;
	
	if (c == EOF)
		return 1;

	/* get instruction/data sign */
	sign = (c == '-') ? -1 : 1;
	if (c != '+' && c != '-')
		return 0;
	else
		c = getc(fp);

	/* get instruction/data number */
	for (i = 0; i < INSTRUCTIONSIZE; i++) {
		if (!isdigit(c))
			return 0;
		num = num * 10 + c - '0';
		c = getc(fp);
	}

	/* get remaining of command line */
	while (c != '\n' && c != EOF)
		c = getc(fp);

	*instruction = sign * num;
	return 1;
}

static void
usage(void)
{
	(void) fprintf(stderr, "usage: simpletron [-cv] file\n");
	exit(EXIT_FAILURE);
}
