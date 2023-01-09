// Include all the libraries we need
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

// For this implementation, I used a linked-list implementation for the queue.
struct queueNode {
    char *info;
    struct queueNode *next;
};

struct queue {
    struct queueNode *front;
    struct queueNode *rear;
    pthread_mutex_t  frontLock, rearLock;
};

// Are global variables
struct queue Q;
pthread_mutex_t threadLock;
pthread_cond_t threadQueue;
int numActiveThreads = 0;

// A struct variable to contain the paramters needed by each thread
struct threadData {
    char *str;
    const char *searchString;
    uint workerNumber;
};

// For queue operations
void initQueue(struct queue *Q);
int isEmpty(struct queue *Q);
void enqueue(struct queue *Q, char *x);
int dequeue(struct queue *Q, char **x);

// For grep runner
void threadCreator(const char *search_string, long N);
void threadHandler(struct threadData *t_data);
void grepRunner(struct threadData *t_data);
void formPathName(char *path, char *str, char *entryName);


int main(int argc, char* argv[]) {
    //Assume that the program will be used correctly.
    //Hence, argv[n] will not result to a NULL pointer.
    //We also assume that the path is correctly formatted.

    char *numberOfWorkers = argv[1];  // This is ignored for now
    char *rootpath = argv[2];
    const char *search_string = argv[3];

    // Process N
    char *extra;
    long N;
    N = strtol(numberOfWorkers, &extra, 10);

    // Initialize the queue
    initQueue(&Q);
    if (pthread_mutex_init(&threadLock, NULL) != 0) {
        printf("Mutex threadLock has failed\n");
    }
    if (pthread_cond_init(&threadQueue, NULL) != 0) {
        printf("Cond variable threadQueue has failed\n");
    }
    
    // Check if the path provided is a relative or absolute path
    if (rootpath[0] == '/') {
        enqueue(&Q, rootpath);
        threadCreator(search_string, N);
    }
    else {
        char *rel = get_current_dir_name();
        char *rtpath = malloc(sizeof(char *)*255);
        strcpy(rtpath, rel);
        strcat(rtpath, "/");
        strcat(rtpath, rootpath);
        enqueue(&Q, rtpath);
        threadCreator(search_string, N);
        free(rel);
        free(rtpath);
    }

    // Necessary so that we will not have memory leaks
    free(Q.front);
    pthread_mutex_destroy(&Q.frontLock);
    pthread_mutex_destroy(&Q.rearLock);
    
    // Destroy the mutex and condition variable
    pthread_mutex_destroy(&threadLock);
    pthread_cond_destroy(&threadQueue);

    return 0;
}

// Functions that handle queue operations
void initQueue(struct queue *Q) {
    // Create a new node to be placed in queue
    struct queueNode *alpha;
    alpha = (struct queueNode *) malloc(sizeof(struct queueNode));

    alpha->next = NULL;
    Q->front = alpha;
    Q->rear = alpha;

    // Initialize the locks to be used
    pthread_mutex_init(&Q->frontLock, NULL);
    pthread_mutex_init(&Q->rearLock, NULL);    
}

int isEmpty(struct queue *Q) {
    return (Q->front->next == NULL);
}

void enqueue(struct queue *Q, char *x) {
    struct queueNode *alpha;
    alpha = (struct queueNode *) malloc(sizeof(struct queueNode));

    assert(alpha != NULL);

    // Create a new copy of the str
    char *str = malloc(sizeof(char)*255); 
    strcpy(str, x);

    // Points to str
    alpha->info = str;
    alpha->next = NULL;

    pthread_mutex_lock(&threadLock);
    pthread_mutex_lock(&Q->rearLock);
    Q->rear->next = alpha;
    Q->rear = alpha;
    pthread_mutex_unlock(&Q->rearLock);
    pthread_mutex_unlock(&threadLock);
}

int dequeue(struct queue *Q, char **x) {
    pthread_mutex_lock(&Q->frontLock);
    struct queueNode *temp = Q->front;
    struct queueNode *alpha = Q->front->next;

    if (alpha == NULL) {
        pthread_mutex_unlock(&threadLock);
        return -1;
    }

    // Let x be the new string
    *x = alpha->info;
    Q->front = alpha;
    pthread_mutex_unlock(&Q->frontLock);

    free(temp);
    return 0;
}

// Functions that handle the grep handler logic
void threadCreator(const char *search_string, long N) {
    pthread_t threads[N];
    struct threadData t_data[N];

    for (int i=0; i<N; i++) {
        t_data[i].searchString = search_string;
        t_data[i].workerNumber = i;
        pthread_create(&threads[i], NULL, (void *) threadHandler, &t_data[i]);
    }

    for (int i=0; i<N; i++) {
        pthread_join(threads[i], NULL);
    }
    printf("Finished\n");
}

void threadHandler(struct threadData *t_data) {
    // Will be used to hold the dequeued paths
    pthread_mutex_lock(&threadLock);
    char *str;
    while(1) {
        while (isEmpty(&Q) && numActiveThreads > 0) {
            pthread_cond_wait(&threadQueue, &threadLock);
        }
        if (isEmpty(&Q) && numActiveThreads == 0) {
            break;
        }

        // Dequeue
        if (dequeue(&Q, &str) == -1) {
            break;
        }
        t_data->str = str;
        numActiveThreads++;
        printf("[%d] DIR %s\n", t_data->workerNumber, str);
        pthread_mutex_unlock(&threadLock);
        
        grepRunner(t_data);

        pthread_mutex_lock(&threadLock);
        numActiveThreads--;
        pthread_cond_broadcast(&threadQueue);
    }
    pthread_mutex_unlock(&threadLock);
}

void grepRunner(struct threadData *t_data) {
    // Initialization of required components
    DIR *dir;
    struct dirent *entry;
    char command[500];
    char path[255];
    int returnValue;

    dir = opendir(t_data->str);
    while ((entry = readdir(dir)) != NULL) {
        // Implies that the entry is a regular file
        if (entry->d_type == 8) {
            // Create the grep command to be used
            strcpy(command, "grep ");
            strcat(command, t_data->searchString);
            strcat(command, " ");
            strcat(command, t_data->str);
            strcat(command, "/");
            strcat(command, entry->d_name);
            strcat(command, " 1> /dev/null 2> /dev/null");

            // Return value after invoking grep (i.e. to indicate if search_string is present)
            returnValue = system(command);

            if (returnValue == 0) {
                formPathName(path, t_data->str, entry->d_name);
                pthread_mutex_lock(&threadLock);
                printf("[%d] PRESENT %s\n", t_data->workerNumber, path);
                pthread_mutex_unlock(&threadLock);
            }
            else {
                formPathName(path, t_data->str, entry->d_name);
                pthread_mutex_lock(&threadLock);
                printf("[%d] ABSENT %s\n", t_data->workerNumber, path);    
                pthread_mutex_unlock(&threadLock);            
            }
        }
        else if (entry->d_type == 4 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                formPathName(path, t_data->str, entry->d_name);
                pthread_mutex_lock(&threadLock);
                printf("[%d] ENQUEUE %s\n", t_data->workerNumber, path);
                pthread_mutex_unlock(&threadLock);
                enqueue(&Q, path);
        }
    }
    closedir(dir);
    free(t_data->str);
}

void formPathName(char *path, char *str, char *entryName) {
    strcpy(path, str);
    strcat(path, "/");
    strcat(path, entryName);
}