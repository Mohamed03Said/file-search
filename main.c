#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "search.h"

int main() {
 
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    init_shared_memory();
    
    pthread_t input_thread, worker_thread;
    // search arguments
    SearchTask task;

    pthread_create(&input_thread, NULL, search_input, &task);
    // wait until the input taken
    pthread_join(input_thread, NULL);

    pthread_create(&worker_thread, NULL, search, &task);
    // wait until the search completed
    pthread_join(worker_thread, NULL);
    
    // wait until extended dir threads completed
    for (int i = 0; i < thread_index; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nSearch results:\n");
    for (int i = 0; i < result_count; i++) {
        printf("- %s ", results[i].path);
        if (results[i].found_in_name) printf("[name match] ");
        if (results[i].found_in_content) printf("[content match]");
        printf("\n");
    }

    cleanup_shared_memory();
    return 0;
}

