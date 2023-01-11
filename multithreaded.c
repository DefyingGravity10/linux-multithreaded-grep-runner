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

// For this implementation, a linked-list implementation for the queue was used.
struct queueNode {
    char *info;
    struct queueNode *next;
};

struct queue {
    struct queueNode *front;
    struct queueNode *rear;
    pthread_mutex_t  frontLock;
    pthread_mutex_t  rearLock;
};

// Global variables used
struct queue Q;
pthread_mutex_t threadLock;
pthread_cond_t threadQueue;
int numActiveThreads = 0;

// A struct variable to contain the parameters needed by each thread
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

// Functions that facilitate in performing parallelized grep
void threadCreator(const char *search_string, long N);
void threadHandler(struct threadData *t_data);
void processTask(struct threadData *t_data);
void formPathName(char *path, char *str, char *entryName);


int main(int argc, char* argv[]) {
    //Assume that the program will be used correctly.
    //Hence, argv[n] will not result to a NULL pointer.
    //We also assume that the path is correctly formatted.

    char *numberOfWorkers = argv[1]; 
    char *rootpath = argv[2];
    const char *search_string = argv[3];

    // Process the number of workers
    char *extra;
    long N;
    N = strtol(numberOfWorkers, &extra, 10);

    // Initialize the queue
    initQueue(&Q);

    // Initialize the mutex threadLock and the cond variable threadQueue
    if (pthread_mutex_init(&threadLock, NULL) != 0) {
        printf("Mutex threadLock has failed\n");
    }
    if (pthread_cond_init(&threadQueue, NULL) != 0) {
        printf("Cond variable threadQueue has failed\n");
    }
    
    // Check if the path provided is a relative or absolute path
    if (rootpath[0] == '/') {   // Absolute
        enqueue(&Q, rootpath);
        threadCreator(search_string, N);
    }
    else {  // Relative
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

    // Destroy the mutexes/cond variables used
    pthread_mutex_destroy(&Q.frontLock);
    pthread_mutex_destroy(&Q.rearLock);
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

    // Initialize the locks to be used for the queue
    if (pthread_mutex_init(&Q->frontLock, NULL) != 0) printf("Mutex frontLock has failed\n");
    if (pthread_mutex_init(&Q->rearLock, NULL) != 0) printf("Mutex rearLock has failed\n");     
}

int isEmpty(struct queue *Q) {
    return (Q->front->next == NULL);
}

void enqueue(struct queue *Q, char *x) {
    struct queueNode *alpha;
    alpha = (struct queueNode *) malloc(sizeof(struct queueNode));

    assert(alpha != NULL);

    // Create a new copy of the str
    char *str = malloc(sizeof(char)*strlen(x)); 
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

// Functions that help the program perform a parallelized grep runner
void threadCreator(const char *search_string, long N) {
    pthread_t threads[N];
    struct threadData t_data[N];    // Used to store all the "data" needed by each thread

    // Thread creation
    for (int i=0; i<N; i++) {
        t_data[i].searchString = search_string;
        t_data[i].workerNumber = i;
        pthread_create(&threads[i], NULL, (void *) threadHandler, &t_data[i]);
    }

    for (int i=0; i<N; i++) {
        pthread_join(threads[i], NULL);
    }
}

void threadHandler(struct threadData *t_data) {
    pthread_mutex_lock(&threadLock);
    char *str;  // Will be used to hold the dequeued paths
    while(1) {
        // Sends a thread to sleep if it is expected to be used again later
        while (isEmpty(&Q) && numActiveThreads > 0) {   
            pthread_cond_wait(&threadQueue, &threadLock);
        }
        // Implies that no more content will be enqueued into the task queue
        if (isEmpty(&Q) && numActiveThreads == 0) {
            break;
        }

        // Dequeue next task from task queue
        if (dequeue(&Q, &str) == -1) {
            break;
        }
        // Update str (i.e. the directory)
        printf("[%d] DIR %s\n", t_data->workerNumber, str);
        t_data->str = str;
        numActiveThreads++;
        pthread_mutex_unlock(&threadLock);
        
        processTask(t_data);

        pthread_mutex_lock(&threadLock);
        numActiveThreads--;
        pthread_cond_signal(&threadQueue);
    }
    pthread_cond_signal(&threadQueue);
    pthread_mutex_unlock(&threadLock);
}

void processTask(struct threadData *t_data) {
    // Initialization of required components
    DIR *dir;
    struct dirent *entry;
    char *command = malloc(sizeof(char *)*strlen(t_data->str)*2);
    char *path = malloc(sizeof(char *)*strlen(t_data->str)*2);
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
    free(command);
    free(path);
    closedir(dir);
    free(t_data->str);
}

void formPathName(char *path, char *str, char *entryName) {
    strcpy(path, str);
    strcat(path, "/");
    strcat(path, entryName);
}