#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1100
#define ITR 10000000

int q[N];
int st=0, nd=0,i;

int main(int argc, char *argv[]) {

	srand(time(0));

	char** arr = malloc(sizeof(char*) * N);

	for (i=0; i<ITR; i++) {
	    int idx = rand() % N;
	    if (arr[idx]) {
		    free(arr[idx]);
            arr[idx] = 0;
	    } else {
		    arr[idx] = malloc(rand()%10000 + 3);
	    }
    }

	for (i=0; i<N; i++)
		if (arr[i]) free(arr[i]);

	free(arr);

    printf("done\n");

    return 0;
}
