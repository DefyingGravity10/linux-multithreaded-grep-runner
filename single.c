#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

// For this implementation, I used a linked-list implementation for the queue.
struct queueNode {
    char *info;
    struct queueNode *link;
};

struct queue {
    struct queueNode *front;
    struct queueNode *rear;
};

struct queue Q;

void initQueue(struct queue *Q);
int isEmpty(struct queue *Q);
void enqueue(struct queue *Q, char *x);
char * dequeue(struct queue *Q);


void grepRunner(char* rootpath, const char* search_string, struct queue Q);

int main(int argc, char* argv[]) {
    //Assume that the program will be used correctly.
    //Hence, argv[n] will not result to a NULL pointer.
    //We also assume that the path is correctly formatted.

    char *numberOfWorkers = argv[1];  // This is ignored for now
    char *rootpath = argv[2];
    const char *search_string = argv[3];

    // This portion is not directly helpful for single.c, which explains 
    // why they are commented out.
    //char *extra;
    //long N;
    //N = strtol(numberOfWorkers, &extra, 10);

    // Prepare the queue to be used
    initQueue(&Q);

    if (rootpath[0] == '/') {
        enqueue(&Q, rootpath);
        grepRunner(rootpath, search_string, Q);
    }
    else {
        // Do I assume that the input will always be a directory agad??
        char *rel = get_current_dir_name();
        char *rtpath = malloc(sizeof(char *)*255);
        strcpy(rtpath, rel);
        strcat(rtpath, "/");
        strcat(rtpath, rootpath);
        //printf("%s\n", rtpath);
        enqueue(&Q, rtpath);
        grepRunner(rtpath, search_string, Q);
        free(rtpath);
        free(rel);
    }

    return 0;
}

void grepRunner(char* rootpath, const char* search_string, struct queue Q) {
    DIR *dir;
    struct dirent *entry;
    char command[500];
    char *path = malloc(sizeof(char *)*255);
    char *path1 = malloc(sizeof(char *)*255);
    char *str;
    int returnValue;

    while(!isEmpty(&Q)) {
        // str is the directory enqueued
        str = dequeue(&Q);
        dir = opendir(str);
        
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == 8) {
                strcpy(command, "grep ");
                strcat(command, search_string);
                strcat(command, " ");
                strcat(command, str);
                strcat(command, "/");
                strcat(command, entry->d_name);
                strcat(command, " 1> /dev/null 2> /dev/null");
                //printf("%s\n", command); // Debug
                returnValue = system(command);
                if (returnValue == 0) {
                    strcpy(path, str);
                    strcat(path, "/");
                    strcat(path, entry->d_name);
                    printf("[0] PRESENT %s\n", path);
                }
                else {
                    strcpy(path, str);
                    strcat(path, "/");
                    strcat(path, entry->d_name);
                    printf("[0] ABSENT %s\n", path);
                }
            }
            if (entry->d_type == 4 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                strcpy(path1, str);
                strcat(path1, "/");
                strcat(path1, entry->d_name);
                enqueue(&Q, path1);
            }
        }
        closedir(dir);
        free(str);
    }
    free(path);
    free(path1);
}

void initQueue(struct queue *Q) {
    Q->front = NULL;
}

int isEmpty(struct queue *Q) {
    return(Q->front == NULL);
}

void enqueue(struct queue *Q, char *x) {
    struct queueNode *alpha;
    alpha = (struct queueNode *) malloc(sizeof(struct queueNode));

    // Assume that queue overflow would not happen
    char *str = malloc(sizeof(char)*500); 
    strcpy(str, x);

    alpha->info = str;
    alpha->link = NULL;

    if (Q->front == NULL) {
        Q->front = alpha;
        Q->rear = alpha;
    }
    else {
        Q->rear->link = alpha;
        Q->rear = alpha;
    }
}

char * dequeue(struct queue *Q) {
    char *x;
    struct queueNode *alpha;

    // Assume that queue underflow would not happen
    x = Q->front->info;
    alpha = Q->front;
    Q->front = Q->front->link;
    free(alpha);

    printf("[0] DIR %s\n", x);
    return x;
}