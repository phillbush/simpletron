#include "memory.h"

struct simpletron {
	/* Simpletron's memory is simulated with a one-dimensional array */
	memtype *memory;
	size_t memsize;

	/* Simpletron's registers are simulated with the following variables */
	int acc;     /* accumulator register (value being processed) */
	int ir;      /* instruction register (current instruction) */
	size_t pc;   /* program counter */
	int opcode;  /* operation currently being performed */
	int operand; /* immediate, or memory location on which current instruction operates */
};
