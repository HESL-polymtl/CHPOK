#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_FILENAME_LENGTH 33

void write_asm_x86(const unsigned int prog_size, FILE* output_file)
{
	unsigned int current_size;

	/* Program header */
	fprintf(output_file, "void asm_cache_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Get the current instruction address */
    fprintf(output_file, "        \"call lab_addr\\n\"\n");      /* 5B */
    fprintf(output_file, "        \"lab_addr:\\n\"\n");          /* 0B Label */
    fprintf(output_file, "        \"pop %%eax\\n\"\n");          /* 1B */

	/* Compute the instruction address for the next jump */
    fprintf(output_file, "        \"addl $0x6, %%eax\\n\"\n");   /* 3B */

	/* Write successive instruction, blocks of 5B */
	current_size = 9;
	while(current_size <= prog_size - 5)
	{
		/* Jump on next intruction and set compute next jump
		   instruction address */
        fprintf(output_file, "        \"jmp *%%eax\\n\"\n");       /* 2B */
		fprintf(output_file, "        \"addl $0x5, %%eax\\n\"\n"); /* 3B */
		current_size += 5;
	}

	/* Add padding with NOP */
	while(current_size < prog_size)
	{
		/* NOP */
		fprintf(output_file, "        \"mov %%ax, %%ax\\n\"\n"); /* 1B */
		++current_size;
	}

	/* End assembly function */
	fprintf(output_file, "    );\n}");
}

/* This has to be tested */
void write_asm_ppc(const unsigned int prog_size, FILE* output_file)
{
	unsigned int current_size;

	/* Program header */
	fprintf(output_file, "void asm_cache_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Stack frame save */
	fprintf(output_file, "        \"mflr r0\\n\"\n");          /* 4B */
	fprintf(output_file, "        \"stw r0, 8(r1)\\n\"\n");    /* 4B */
	fprintf(output_file, "        \"stwu r1, -16(r1)\\n\"\n"); /* 4B */

	/* Get current instruction address */
    fprintf(output_file, "        \"BL lab_addr\\n\"\n");    /* 4B */
    fprintf(output_file, "        \"lab_addr:\\n\"\n");      /* 0B Label */
    fprintf(output_file, "        \"mflr r0\\n\"\n");        /* 4B */

	/* Compute next instruction to jump to with blr */
	fprintf(output_file, "        \"add r0, r0, 24\\n\"\n"); /* 4B */

	/* Write successive instruction, blocks of 5B */
	current_size = 24;
	while(current_size <= prog_size - 24)
	{

		/* Load branch address in LR */
		fprintf(output_file, "        \"mtlr r0\\n\"\n");        /* 4B */

		/* Jump on next intruction and set next instruction address */
		fprintf(output_file, "        \"blr\\n\"\n");            /* 4B */
		fprintf(output_file, "        \"add r0, r0, 16\\n\"\n"); /* 4B */
		current_size += 12;
	}

	/* Add padding */
	while(current_size < prog_size - 12)
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
void write_asm_arm(const unsigned int prog_size, FILE* output_file)
{
	unsigned int current_size;

	/* Program header NON THUMB ARM GENERATED */
	fprintf(output_file, "void asm_cache_flood(void)\n{\n");
	fprintf(output_file, "    __asm__ __volatile__(\n");

	/* Save stack frame */
	fprintf(output_file, "        \"PUSH {LR}\\n\"\n");        /* 4B */

	/* Get current intruction address */
    fprintf(output_file, "        \"BL lab_addr\\n\"\n");      /* 4B */
    fprintf(output_file, "        \"lab_addr\\n\"\n");         /* 0B Label */
    fprintf(output_file, "        \"MOV R0, LR\\n\"\n");       /* 4B */

	/* Compute next instruction to jump to with BX */
    fprintf(output_file, "        \"ADD R0, R0, #12\\n\"\n");  /* 4B */

	/* Write successive instruction, blocks of 5B */
	current_size = 16;
	while(current_size <= prog_size - 12)
	{
		/* Jump on next intruction and set next instruction address */
        fprintf(output_file, "        \"BX  R0\\n\"\n");         /* 4B */
		fprintf(output_file, "        \"ADD R0, R0, #8\\n\"\n"); /* 4B */
		current_size += 8;
	}

	/* Add padding */
	while(current_size < prog_size - 4)
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
	char          filename[MAX_FILENAME_LENGTH];
	FILE*         output_file;

	printf("Output file name (%u characters max): ", MAX_FILENAME_LENGTH - 1);
	scanf("%32s", filename);
	scanf("%*[^\n]");
	getchar();
	filename[MAX_FILENAME_LENGTH - 1] = 0;

	printf("Set the program size in byte: ");
	scanf("%10u", &prog_size);
	scanf("%*[^\n]");
	getchar();

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
			write_asm_x86(prog_size, output_file);
			break;
		case 1:
			write_asm_ppc(prog_size, output_file);
			break;
		case 2:
			write_asm_arm(prog_size, output_file);
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
