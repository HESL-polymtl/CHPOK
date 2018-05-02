//#include "test.h"

int main()
{
	//asm_cache_flood();
	__asm__ __volatile__(
	"mflr R0\n"
	"stw r0, 8(r1)\n"
	"stwu r1, -16(r1)\n"
	"BL lab_addr\n"
	"B end\n"	
);
}

/*
"lab_addr:\n"
	    "mflr r0\n"
	    "add r0, r0, 24\n"
	    "mtlr r0\n"
	    "blr\n"
	    "add r0, r0, 16\n"
	    "mtlr r0\n"
	    "blr\n"
	    "add r0, r0, 16\n"
	    "mtlr r0\n"
	    "blr\n"

	"end:\n"
	    "ori r0, r0, 0\n"

	    "addi r1, r1, 16\n"
	    "lwz r0, 8(r1)\n"
	    "mtlr r0\n"
	    "blr\n"*/
