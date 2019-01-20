#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHAREDMEM_NAME "my_shared_region"

int sem1_value = -1;
int sem2_value = -1;

int main(int argc, char *argv[]) {
	double minvalue = atof(argv[1]);
	double maxvalue = atof(argv[2]);
	int bincount = atoi(argv[3]);
	int N = atoi(argv[4]);

	int fd;
	void *sptr;
	int *intp;
	sem_t *sem1; // First semaphore
	sem_t *sem2; // Second semaphore

	int SHAREDMEM_SIZE = sizeof(int) * bincount;

	sem_unlink("/semaphore1");
	sem_unlink("/semaphore2");
	shm_unlink(SHAREDMEM_NAME);
	fd = shm_open(SHAREDMEM_NAME, O_RDWR | O_CREAT, 0660);

	ftruncate(fd, SHAREDMEM_SIZE);

	sptr = mmap(NULL, SHAREDMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	close(fd);

	intp = (int *) sptr;
	// create, initialize semaphores
	sem1 = sem_open("/semaphore1", O_CREAT, 0660, 1);
	sem_getvalue(sem1, &sem1_value);

	sem2 = sem_open("/semaphore2", O_CREAT, 0660, 0);
	sem_getvalue(sem2, &sem2_value);


	/*
		arg[0] = ./xxxx
		arg[1] = minvalue
		arg[2] = maxvalue
		arg[3] = bincount
		arg[4] = N
		arg[5...] = file1 .. filen 
	*/

	FILE *fp;

	char *input_holder[N];
	for (int i = 5; i < N + 5; i++) {
		input_holder[i - 5] = argv[i];
	}
	int outfile_array[bincount];
	memset(outfile_array, 0, sizeof outfile_array);

	int temp_for_child[bincount];
	int combined_arr[bincount];
	memset(combined_arr, 0, sizeof combined_arr);
	memset(temp_for_child, 0, sizeof temp_for_child);
	double w = (maxvalue - minvalue) / bincount;

	for (int q = 0; q < N; q++) {
		if (fork()) {
			sem_getvalue(sem1, &sem1_value);
			sem_getvalue(sem2, &sem2_value);

			sem_wait(sem2);
			for (int i = 0; i < bincount; i++) {
				combined_arr[i] += intp[i];

			}
			sem_post(sem1);

		} else {

			sem_getvalue(sem1, &sem1_value);

			sem_getvalue(sem2, &sem2_value);

			fp = fopen(input_holder[q], "r"); // read mode
			if (fp == NULL) {
				perror("Error while opening the file.\n");
				exit(EXIT_FAILURE);
			}

			char temp[100];

			while (fgets(temp, sizeof(temp), fp) != NULL) {
				double temp_value = atof(temp);
				for (int i = 0; i < bincount; i++) {
					if ((i * w) + minvalue <= temp_value && temp_value < ((i + 1) * w) + minvalue) {
						temp_for_child[i] = temp_for_child[i] + 1;
					}
				}
				if (fabs(temp_value - maxvalue) < FLT_MIN) {
					temp_for_child[bincount - 1] = temp_for_child[bincount - 1] + 1;

				}

			}
			sem_getvalue(sem1, &sem1_value);
			sem_wait(sem1);
			sem_getvalue(sem1, &sem1_value);
			sem_getvalue(sem2, &sem2_value);

			for (int i = 0; i < bincount; i++) {
				intp[i] = temp_for_child[i];
			}
			sem_post(sem2);
			sem_getvalue(sem2, &sem2_value);
			fclose(fp);

			exit(0);
		}
	}

	shm_unlink(SHAREDMEM_NAME);
	sem_close(sem1);
	sem_unlink("/semaphore1");
	sem_close(sem2);
	sem_unlink("/semaphore2");

	FILE *fp_out = fopen(argv[argc - 1], "w");
	for (int qq = 0; qq < bincount; qq++) {
		fprintf(fp_out, "%d: %d\n", (qq + 1), combined_arr[qq]);

	}

	return 0;
}

