
#include <stdio.h>
//#include <stdlib.h>

#include "mem_alloc.h"

#define SIZE_BUFFER 128

void aide(void) {

    printf("-------------- memory shell --------------\n");
	printf("Interactive memory shell of size of %d bytes\n", MEMORY_SIZE);
	printf("Supported commands:\n");
	printf("\t a[n]: allocates [n] bytes\n");
	printf("\t f[p]: free allocated memory at address [p]\n");
	printf("\t p   : print free blocks\n");
	printf("\t b   : print busy blocks\n");
	printf("\t g   : print fragmentation info\n");
	printf("\t h   : print this help message\n");
	printf("\t q   : quit\n\n");
}

int main(int argc, char *argv[]) {
    char buffer[SIZE_BUFFER];
    char cmd;
    char *addr;
    int offset;
    int size;

    memory_init();

    while (1) {
        printf("? ");
        fflush(stdout);
        cmd = getchar();
        switch (cmd) {
            case 'a':
                scanf("%d", &size);
                addr = memory_alloc(size);
                if (addr == NULL)
                    printf("Allocation failed\n");
                else
                    printf("Memory allocated at %d\n", (int) (addr - heap_base()));
                break;
            case 'f':
                scanf("%d", &offset);
                memory_free(heap_base() + offset);
            case 'p':
                print_free_blocks();
                break;
            case 'b':
                print_busy_blocks();
                break;
            case 'g':
				printf("Fragmentation at this point is %.3f\n", calculate_fragmentation());
                break;
            case 'h':
                aide();
                break;
            case 'q':
				/*if (is_memory_leak_exist()) {
					printf("Warning.. Memory Leak!!\n");
					print_busy_blocks();
				}*/
                exit(0);
            default: {
                fprintf(stderr, "Command not found !\n");
	            aide();
			}
        }
        fgets(buffer, SIZE_BUFFER, stdin);
    }
    return 0;
}
