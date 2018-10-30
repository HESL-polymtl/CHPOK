/*******************************************************************************
 * ARINC-653 Multicore RTOS Benchmark
 *
 * Developped by:
 *    Alexy Torres Aurora Dugo
 *    Jean-Baptiste Lefoul
 *
 * Generate assembly code allowing to fill the instruction tlb with instruction
 * pages.
 * The generator is able to generate x86, ARMv5 and PowerPC assembly code.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_FILENAME_LENGTH 33

void write_asm_x86(const unsigned int prog_size, const char align,
	               FILE* output_file)
{
	unsigned int current_size;

	/* Program header */
	fprintf(output_file, "void asm_tbl_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Align on page boundary */
	if(align)
	{
		fprintf(output_file, "        \".align 4096\\n\"\n");
	}

	/* Get the current instruction address */
    fprintf(output_file, "        \"call lab_addr\\n\"\n");      /* 5B */
    fprintf(output_file, "        \"lab_addr:\\n\"\n");          /* 0B Label */
    fprintf(output_file, "        \"pop %%eax\\n\"\n");          /* 1B */

	/* Compute the instruction address for the next jump */
	fprintf(output_file, "        \"addl $0x0FFB, %%eax\\n\"\n");   /* 3B */
	/* Jump on next intruction */
	fprintf(output_file, "        \"jmp *%%eax\\n\"\n");       /* 2B */

	/* Write successive instruction, blocks of 4096B */
	current_size = 4096;
	while(current_size <= prog_size - 4096)
	{
		fprintf(output_file, "        \".align 4096\\n\"\n");
		/* Compute the instruction address for the next jump */
	    fprintf(output_file, "        \"addl $0x1000, %%eax\\n\"\n");   /* 3B */
		/* Jump on next intruction */
        fprintf(output_file, "        \"jmp *%%eax\\n\"\n");       /* 2B */

		current_size += 4096;
	}

	/* Add padding with NOP, one page to prevent jump in the unknown! */
	current_size = 0;
	while(current_size < 4096)
	{
		/* NOP */
		fprintf(output_file, "        \"nop\\n\"\n"); /* 1B */
		++current_size;
	}

	/* End assembly function */
	fprintf(output_file, "    );\n}");
}

/* This has to be tested */
void write_asm_ppc(const unsigned int prog_size, const char align,
	               FILE* output_file)
{
	unsigned int current_size;

	/* Program header */
	fprintf(output_file, "void asm_tbl_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Align on page boundary */
	if(align)
	{
		fprintf(output_file, "        \".align 12\\n\"\n");
	}

	/* Stack frame save */
	fprintf(output_file, "        \"mflr r0\\n\"\n");          /* 4B */
	fprintf(output_file, "        \"stw r0, 8(r1)\\n\"\n");    /* 4B */
	fprintf(output_file, "        \"stwu r1, -16(r1)\\n\"\n"); /* 4B */

	/* Get current instruction address */
    fprintf(output_file, "        \"BL lab_addr\\n\"\n");    /* 4B */
    fprintf(output_file, "        \"lab_addr:\\n\"\n");      /* 0B Label */
    fprintf(output_file, "        \"mflr r0\\n\"\n");        /* 4B */

	/* Compute next instruction to jump to with blr */
	fprintf(output_file, "        \"add r0, r0, 4076\\n\"\n"); /* 4B */

	/* Load branch address in LR */
	fprintf(output_file, "        \"mtlr r0\\n\"\n");        /* 4B */

	/* Jump on next intruction and set next instruction address */
	fprintf(output_file, "        \"blr\\n\"\n");            /* 4B */

	/* Write successive instruction, blocks of 5B */
	current_size = 4096;
	while(current_size <= prog_size - 4096)
	{
		fprintf(output_file, "        \".align 12\\n\"\n");

		/* Compute the next instruction address */
		fprintf(output_file, "        \"add r0, r0, 4096\\n\"\n"); /* 4B */

		/* Load branch address in LR */
		fprintf(output_file, "        \"mtlr r0\\n\"\n");        /* 4B */

		/* Jump on next intruction and set next instruction address */
		fprintf(output_file, "        \"blr\\n\"\n");            /* 4B */

		current_size += 4096;
	}

	/* Add padding */
	current_size = 0;
	while(current_size < 4084)
	{
		/* NOP */
		fprintf(output_file, "        \"ori r0, r0, 0\\n\"\n");  /* 4B */
		current_size += 4;
	}

	/* Restore stack context */
	fprintf(output_file, "        \"addi r1, r1, 16\\n\"\n"); /* 4B */
	fprintf(output_file, "        \"lwz r0, 8(r1)\\n\"\n");   /* 4B */
	fprintf(output_file, "        \"mtlr r0\\n\"\n");         /* 4B */

	/* End assembly function */
	fprintf(output_file, "    );\n}");
}

/* This has been tested on a CPU simulator but not yet on a real hardware */
void write_asm_arm(const unsigned int prog_size, const char align,
	               FILE* output_file)
{
	unsigned int current_size;

	/* Program header NON THUMB ARM GENERATED */
	fprintf(output_file, "void asm_tbl_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Align on page boundary */
	if(align)
	{
		fprintf(output_file, "        \"ALIGN 4096\\n\"\n");
	}

	/* Save stack frame */
	fprintf(output_file, "        \"PUSH {LR}\\n\"\n");        /* 4B */


	/* Get current intruction address */
    fprintf(output_file, "        \"BL lab_addr\\n\"\n");      /* 4B */
    fprintf(output_file, "        \"lab_addr\\n\"\n");         /* 0B Label */
    fprintf(output_file, "        \"MOV R0, LR\\n\"\n");       /* 4B */

	/* Compute next instruction to jump to with BX */
    fprintf(output_file, "        \"ADD R0, R0, #4090\\n\"\n");  /* 4B */

	/* Jump to the next instruction */
	fprintf(output_file, "        \"BX  R0\\n\"\n");         /* 4B */

	/* Write successive instruction, blocks of 4096B */
	current_size = 4096;
	while(current_size <= prog_size - 4096)
	{
		fprintf(output_file, "        \"ALIGN 4096\\n\"\n");
		/* Compute the instruction address for the next jump */
	    fprintf(output_file, "        \"ADD R0, R0, #4096\\n\"\n"); /* 4B */
		/* Jump on next intruction */
        fprintf(output_file, "        \"BX  R0\\n\"\n");            /* 4B */
		current_size += 4096;
	}

	/* Add padding with NOP, one page to prevent jump in the unknown! */
	current_size = 0;
	while(current_size < 4092)
	{
		/* NOP */
		fprintf(output_file, "        \"MOV R0, R0\\n\"\n"); /* 4B */
		current_size += 4;
	}

	/* Restore stack context */
	fprintf(output_file, "        \"POP {LR}\\n\"\n");  /* 4B */

	/* End assembly function */
	fprintf(output_file, "    );\n}");
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	unsigned char select;
	unsigned int  prog_size;
	char          align;
	char          filename[MAX_FILENAME_LENGTH];
	FILE*         output_file;

	printf("Output file name (%u characters max): ", MAX_FILENAME_LENGTH - 1);
	scanf("%32s", filename);
	scanf("%*[^\n]");
	getchar();
	filename[MAX_FILENAME_LENGTH - 1] = 0;

	printf("Set the program size in byte (at least two pages): ");
	scanf("%10u", &prog_size);
	scanf("%*[^\n]");
	getchar();

	do
	{
		printf("Align on page boundary (y/n): ");
		scanf("%c", &align);
		scanf("%*[^\n]");
		getchar();

		if(align < 97)
		{
			align += 32;
		}
	} while(align != 'y' && align != 'n');
	align = (align == 'y') ? 1 : 0;

	/* Open output file */
	output_file = fopen(filename, "w");
	if(output_file == NULL)
	{
		perror("Cannot open output file:\t");
		return -1;
	}

	printf("Select arch:\n\t0. X86\n\t1. PowerPC\n\t2. ARM\n");
	do
	{
		printf("Arch: ");
		scanf("%3hhu", &select);
		scanf("%*[^\n]");
		getchar();

		if(select > 2)
		{
			printf("Wrong architecture\n");
		}
	} while(select > 2);

	/* Arch selection */
	switch(select)
	{
		case 0:
			write_asm_x86(prog_size, align, output_file);
			break;
		case 1:
			write_asm_ppc(prog_size, align, output_file);
			break;
		case 2:
			write_asm_arm(prog_size, align, output_file);
			break;
		default:
			printf("Wrong selection, this is not supposed to happen.\n");
			return -1;
	}

	/* Clone output file */
	if(fclose(output_file) != 0)
	{
		perror("Error while closing the output file:\t");
		return -1;
	}

	printf("File saved\n");

	return 0;
}
