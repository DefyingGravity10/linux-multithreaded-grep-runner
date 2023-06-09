// Include all the libraries we need
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <assert.h>

// For this implementation, I used a linked-list implementation for the queue.
struct queueNode {
    char *info;
    struct queueNode *next;
};

struct queue {
    struct queueNode *front;
    struct queueNode *rear;
};

// Is a global variable
struct queue Q;

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
void threadCreator(const char *search_string);
void *threadHandler(struct threadData *t_data);
void processTask(struct threadData *t_data);
void formPathName(char *path, char *str, char *entryName);


int main(int argc, char* argv[]) {
    //Assume that the program will be used correctly.
    //Hence, argv[n] will not result to a NULL pointer.
    //We also assume that the path is correctly formatted.

    char *numberOfWorkers = argv[1];  // This is ignored for now
    char *tempRootpath = argv[2];
    const char *search_string = argv[3];

    // This portion is not directly helpful for single.c, which explains 
    // why they are commented out.
    //char *extra;
    //long N;
    //N = strtol(numberOfWorkers, &extra, 10);

    // Initialize the queue
    initQueue(&Q);

    char *rootpath = malloc(sizeof(char *)*255);
    strcpy(rootpath, tempRootpath);
    if (rootpath[strlen(rootpath)-1] == '/') {
        rootpath[strlen(rootpath)-1] = '\0';
    }

    // Check if the path provided is a relative or absolute path
    if (rootpath[0] == '/') {
        enqueue(&Q, rootpath);
        threadCreator(search_string);
    }
    else {
        char *rel = get_current_dir_name();
        char *rtpath = malloc(sizeof(char *)*255);
        strcpy(rtpath, rel);
        strcat(rtpath, "/");
        strcat(rtpath, rootpath);
        enqueue(&Q, rtpath);
        threadCreator(search_string);
        free(rel);
        free(rtpath);
    }

    // Necessary so that we will not have memory leaks
    free(rootpath);    
    free(Q.front);

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
}

int isEmpty(struct queue *Q) {
    return (Q->front->next == NULL);
}

void enqueue(struct queue *Q, char *x) {
    struct queueNode *alpha;
    alpha = (struct queueNode *) malloc(sizeof(struct queueNode));

    assert(alpha != NULL);

    // Create a new copy of the str
    char *str = malloc(sizeof(char)*1000); 
    strcpy(str, x);

    // Points to str
    alpha->info = str;
    alpha->next = NULL;

    Q->rear->next = alpha;
    Q->rear = alpha;
}

int dequeue(struct queue *Q, char **x) {
    // Lock here
    struct queueNode *temp = Q->front;
    struct queueNode *alpha = Q->front->next;

    if (alpha == NULL) {
        return -1;
    }

    // Let x be the new string
    *x = alpha->info;
    Q->front = alpha;

    free(temp);
    return 0;
}

// Functions that handle the grep handler logic
void threadCreator(const char *search_string) {
    // For now, the struct will only be created to accomodate one worker (main)
    struct threadData t_data[1];
    t_data[0].searchString = search_string;
    t_data[0].workerNumber = 0;
    threadHandler(&t_data[0]);
}

void *threadHandler(struct threadData *t_data) {
    // Will be used to hold the dequeued paths
    char *str;
    while(!isEmpty(&Q)) {
        if (dequeue(&Q, &str) == -1) {
            break;
        }
        t_data->str = str;
        printf("[%d] DIR %s\n", t_data->workerNumber, str);
        processTask(t_data);
    }
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
        // Implies that the entry is a directory
        if (entry->d_type == 4 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                formPathName(path, t_data->str, entry->d_name);
                enqueue(&Q, path);
                printf("[%d] ENQUEUE %s\n", t_data->workerNumber, path);
        }
        // Implies that the entry is a regular file
        else if (entry->d_type == 8) {
            // Create the grep command to be used
            strcpy(command, "grep ");
            strcat(command, "\"");
            strcat(command, t_data->searchString);
            strcat(command, "\"");
            strcat(command, " ");
            strcat(command, "\"");
            strcat(command, t_data->str);
            strcat(command, "\"");
            strcat(command, "/");
            strcat(command, entry->d_name);
            strcat(command, " 1> /dev/null 2> /dev/null");

            // Return value after invoking grep (i.e. to indicate if search_string is present)
            returnValue = system(command);

            if (returnValue == 0) {
                formPathName(path, t_data->str, entry->d_name);
                printf("[%d] PRESENT %s\n", t_data->workerNumber, path);
            }
            else {
                formPathName(path, t_data->str, entry->d_name);
                printf("[%d] ABSENT %s\n", t_data->workerNumber, path);                
            }
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