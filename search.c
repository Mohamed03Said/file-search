#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include "search.h"

pthread_t *threads = NULL;
SearchResult *results = NULL;
int result_count = 0;
int thread_index = 0;

pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;

// prevent caching, atomic access
volatile sig_atomic_t stop = 0;

void handle_signal(int signo) {
    printf("\n[!] Caught signal %d, exiting...\n", signo);
    stop = 1;
}

void init_shared_memory() {
    // create shared memory space for storing results
    results = mmap(NULL, sizeof(SearchResult) * MAX_RESULTS,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (results == MAP_FAILED) {
        perror("results memory map failed");
        exit(EXIT_FAILURE);
    }
    
    threads = mmap(NULL, sizeof(pthread_t) * MAX_THREADS,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (threads == MAP_FAILED) {
        perror("threads memory map failed");
        exit(EXIT_FAILURE);
    }
}

void cleanup_shared_memory() {
    // release the memory allocation of results
    if (results) {
        if (munmap(results, sizeof(SearchResult) * MAX_RESULTS) == -1) {
          perror("results memory unmap failed");
        }
    }
    
    if (threads) {
        if (munmap(threads, sizeof(pthread_t) * MAX_THREADS) == -1) {
          perror("threads memory unmap failed");
        }
    }
}

void add_result(const char *path, int in_name, int in_content) {
    pthread_mutex_lock(&result_mutex);
    if (result_count < MAX_RESULTS) {
        strncpy(results[result_count].path, path, MAX_PATH);
        results[result_count].found_in_name = in_name;
        results[result_count].found_in_content = in_content;
        result_count++;
    }
    pthread_mutex_unlock(&result_mutex);
}

int search_in_file_content(const char *filepath, const char *search_key) {
    if (stop) {
      pthread_exit(NULL);
    }
    
    FILE *fp = fopen(filepath, "r");
    if (!fp) 
      return 0;

    char line[1024];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, search_key)) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

void extend_dir_search(const char *start_dir, const char *search_key) {
    pthread_mutex_lock(&thread_mutex);

    if (thread_index >= MAX_THREADS) {
        pthread_mutex_unlock(&thread_mutex);
        return;
    }

    SearchTask *task = malloc(sizeof(SearchTask));
    if (!task) {
        pthread_mutex_unlock(&thread_mutex);
        return;
    }

    strncpy(task->start_dir, start_dir, 250);
    strncpy(task->search_key, search_key, 250);

    printf("Extend thread %d , dir: %s\n", thread_index, start_dir);
    pthread_create(&threads[thread_index++], NULL, search, task);

    pthread_mutex_unlock(&thread_mutex);
}


void recursive_search(const char *dir_path, const char *search_key) {
    if (stop) {
      pthread_exit(NULL);
    }
    
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH];
        
        // prevent infinite recursion
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        // final path, format, path, name
        snprintf(path, MAX_PATH, "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1)
            continue;

      // check type
        if (S_ISDIR(st.st_mode)) {
            extend_dir_search(path, search_key);
        } 
        else if (S_ISREG(st.st_mode)) {
            int in_name = strstr(entry->d_name, search_key) != NULL;
            int in_content = search_in_file_content(path, search_key);
            if (in_name || in_content) {
                add_result(path, in_name, in_content);
            }
        }
    }
    closedir(dir);
}

void *search(void *arg) {
    if (stop) {
      pthread_exit(NULL);
    }
    
    SearchTask *task = (SearchTask *)arg;
    recursive_search(task->start_dir, task->search_key);
    return NULL;
}

void *search_input(void *arg) {
    if (stop) {
      pthread_exit(NULL);
    }
    
    SearchTask *task = (SearchTask *)arg;

    printf("Enter search key: ");
    fgets(task->search_key, sizeof(task->search_key), stdin);
    task->search_key[strcspn(task->search_key, "\n")] = '\0';
    
    printf("Enter directory to search: ");
    fgets(task->start_dir, sizeof(task->start_dir), stdin);
    task->start_dir[strcspn(task->start_dir, "\n")] = '\0';
    
    return NULL;
}

