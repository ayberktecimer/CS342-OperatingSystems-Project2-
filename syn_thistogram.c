#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

struct node {
	struct node *next;
	double data;
};

struct queue {
	struct node *head;
	struct node *tail;
	long count;
};

struct buffer {
	struct queue *queue;
	pthread_mutex_t queueMutex;
	pthread_cond_t hasItemCondition;
};

void initialize(struct queue *queue) {
	queue->count = 0;
	queue->head = NULL;
	queue->tail = NULL;
}

void enqueue(struct queue *queue, struct node *node) {
	if (queue->count == 0) {
		queue->head = node;
		queue->tail = node;
	} else {
		queue->tail->next = node;
		queue->tail = node;
	}
	queue->count++;
}

/**
 * Add a linked list to the tail of another linked list
 * @param baseQueue
 * @param queueToBeAdded
 */
void enqueueLinkedList(struct queue *baseQueue, struct queue *queueToBeAdded) {
	if (baseQueue->count == 0) {
		baseQueue->head = queueToBeAdded->head;
		baseQueue->tail = queueToBeAdded->tail;
	} else {
		baseQueue->tail->next = queueToBeAdded->head;
		baseQueue->tail = queueToBeAdded->tail;
	}
	baseQueue->count += queueToBeAdded->count;
}

struct node *dequeue(struct queue *queue) {
	if (queue->count == 0) {
		return NULL;
	} else {
		struct node *result;
		result = queue->head;
		queue->head = queue->head->next;
		queue->count--;
		return result;
	}
}

/**
 * Prints elements by dequeuing
 * @param queue
 */
void printQueue(struct queue *queue) {
	printf("Number of nodes: %li\n", queue->count);
	printf("Queue content:\n");
	while (queue->count > 0) {
		printf("%lf\n", dequeue(queue)->data);
	}
}

void insertDataToHistogram(double data, double minValue, double binWidth, int *histogram, int binCount) {
	int count = 0;

	while (1) {
		if (count == binCount - 1 || (minValue * count) <= data &&
			minValue + ((count + 1) * binWidth) > data) {
			histogram[count]++;
			break;
		}
		count++;
	}
}

void writeHistogramToFile(int *histogram, long binCount, char *outputFileName) {
	FILE *outputFile = fopen(outputFileName, "w");
	if (outputFile == NULL) {
		printf("Error creating file\n");
		exit(-1);
	}

	for (int i = 0; i < binCount; i++) {
		fprintf(outputFile, "%d: %d", (i + 1), histogram[i]);
		fprintf(outputFile, "\n");
	}
	fclose(outputFile);
};

// Global variables
struct buffer *buffer;
struct threadArguments {
	char *fileName;
	long batch;
};

/**
 * Thread function to read a file and
 * add its content to the global linked list
 */
void *readFile(void *arg) {
	struct threadArguments *args = (struct threadArguments *) arg;
	char *inputFileName = args->fileName;
	long batch = args->batch;

	FILE *inputFile = fopen(inputFileName, "r");
	if (inputFile == NULL) {
		printf("Error while opening the file.\n");
		exit(-1);
	}

	// Parse file line by line
	char line[100];
	struct queue *batchQueue = malloc(sizeof(struct queue)); // Used for batching numbers
	initialize(batchQueue);
	int batchCounter = 0;
	while (fgets(line, sizeof(line), inputFile) != NULL) {
		double lineValue = strtod(line, NULL);

		// Wrap line value in a linked list node
		struct node *newNode = malloc(sizeof(struct node));
		newNode->data = lineValue;
		newNode->next = NULL;

		enqueue(batchQueue, newNode);
		batchCounter++;

		if (batchCounter == batch) {
			// Start critical section
			pthread_mutex_lock(&buffer->queueMutex);
			enqueueLinkedList(buffer->queue, batchQueue); // Add node to the queue

			if (buffer->queue->count > 0) { // Send signal when queue is not empty
				pthread_cond_signal(&buffer->hasItemCondition);
//				printf("Queue contains 1 node now\n");
			}
			pthread_mutex_unlock(&buffer->queueMutex);
			// End critical section

			initialize(batchQueue);
			batchCounter = 0;
		}
	}

	// Add remaining numbers if any exists
	if (batchCounter > 0) {
		// Start critical section
		pthread_mutex_lock(&buffer->queueMutex);
		enqueueLinkedList(buffer->queue, batchQueue); // Add node to the queue

		if (buffer->queue->count > 0) { // Send signal when queue is not empty
			pthread_cond_signal(&buffer->hasItemCondition);
//			printf("Queue contains 1 node now\n");
		}
		pthread_mutex_unlock(&buffer->queueMutex);
		// End critical section
	}

	fclose(inputFile);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	// Check arguments
	if (argc < 8) {
		printf("Invalid number of arguments...\n");
		exit(-1);
	}

	/*
	 * Parse arguments
	 * arg[0] = ./xxxx
	 * arg[1] = minValue
	 * arg[2] = maxValue
	 * arg[3] = binCount
	 * arg[4] = n
	 * arg[5...] = file1 .. filen
	 * arg[last-1] = outfile
	 * arg[last] = batch
	*/
	double minValue = strtod(argv[1], NULL);
	double maxValue = strtod(argv[2], NULL);
	long binCount = strtol(argv[3], NULL, 10);
	long n = strtol(argv[4], NULL, 10);
	char *outputFileName = argv[argc - 2];
	long batch = strtol(argv[argc - 1], NULL, 10);
	double binWidth = (maxValue - minValue) / binCount;

	char *inputFileNames[n];
	for (int i = 0; i < n; i++) {
		inputFileNames[i] = argv[i + 5];
	}

	if (batch < 1) {
		printf("Batch must be positive\n");
		exit(-1);
	}

	// Count all numbers for break condition
	long countAllNumbers = 0;
	for (int i = 0; i < n; i++) {
		FILE *inputFile = fopen(inputFileNames[i], "r");
		char line[100];
		while (fgets(line, sizeof(line), inputFile) != NULL) {
			countAllNumbers++;
		}
		fclose(inputFile);
	}

	// Initialize histogram array
	int histogram[binCount];
	for (int i = 0; i < binCount; i++) {
		histogram[i] = 0;
	}

	// Initialize mutual linked list (queue)
	buffer = malloc(sizeof(struct buffer));
	buffer->queue = malloc(sizeof(struct queue));
	initialize(buffer->queue);

	// Initialize mutex/condition variables
	pthread_mutex_init(&buffer->queueMutex, NULL);
	pthread_cond_init(&buffer->hasItemCondition, NULL);

	// Create a thread for each file
	pthread_t tids[n]; // Thread IDs
	for (int i = 0; i < n; i++) {
		struct threadArguments *args = malloc(sizeof(struct threadArguments));
		args->batch = batch;
		args->fileName = inputFileNames[i];
		pthread_create(&tids[i], NULL, readFile, args);
	}

	int countConsumedNumbers = 0;
	while (1) {
		// Start critical section
		pthread_mutex_lock(&buffer->queueMutex);
		while (buffer->queue->count == 0 && countConsumedNumbers < countAllNumbers) { // Wait if queue is empty
//			printf("Queue is empty...Waiting...\n");
			pthread_cond_wait(&buffer->hasItemCondition, &buffer->queueMutex);
		}

		// Add numbers in queue to the histogram in size of batch
		for (int batchCounter = 0; batchCounter < batch && buffer->queue->count != 0; batchCounter++) {
			insertDataToHistogram(dequeue(buffer->queue)->data, minValue, binWidth, histogram, binCount);
			countConsumedNumbers++;
		}

		pthread_mutex_unlock(&buffer->queueMutex);
		// End critical section

		// Break condition
		if (countConsumedNumbers == countAllNumbers) {
			goto allFinish;
		}
	}

	allFinish:
	// Write histogram to the file
	writeHistogramToFile(histogram, binCount, outputFileName);

	return 0;
}
