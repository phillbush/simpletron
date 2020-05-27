#include <stdint.h>

#define MEMSIZE         100
#define MEM_MAX         9999
#define MEM_MIN         -9999
#define INSTRUCTIONSIZE 4
#define OPSIZE          2

typedef int16_t memtype;

enum operation {
	READ        = 10,
	WRITE       = 11,
	LOAD        = 20,
	STORE       = 21,
	ADD         = 30,
	SUBTRACT    = 31,
	DIVIDE      = 32,
	MULTIPLY    = 33,
	REMAINDER   = 34,
	ADD_I       = 40,
	SUBTRACT_I  = 41,
	DIVIDE_I    = 42,
	MULTIPLY_I  = 43,
	REMAINDER_I = 44,
	BRANCH      = 50,
	BRANCHNEG   = 51,
	BRANCHZERO  = 52,
	HALT        = 53
};
