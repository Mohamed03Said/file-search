#ifndef SEARCH_H
#define SEARCH_H

#include <pthread.h>

#define MAX_RESULTS 1000
#define MAX_PATH 1000
#define MAX_THREADS 1000

typedef struct {
    char path[MAX_PATH];
    int found_in_name;
    int found_in_content;
} SearchResult;

typedef struct {
    char search_key[250];
    char start_dir[250];
} SearchTask;

extern pthread_t *threads;
extern int thread_index;
extern SearchResult *results;
extern int result_count;

extern pthread_mutex_t thread_mutex;
extern pthread_mutex_t result_mutex;

void *search_input(void *arg);
void *search(void *arg);
void handle_signal(int signo);
void init_shared_memory();
void cleanup_shared_memory();

#endif

