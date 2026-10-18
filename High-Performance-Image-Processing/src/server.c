#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include<errno.h>
#include<sys/resource.h>
#include "logging.h"
#define PORT 8080
#define IMG_WIDTH 512
#define IMG_HEIGHT 512
#define CHANNELS 4
#define IMG_SIZE (IMG_WIDTH*IMG_HEIGHT*CHANNELS)
#define POOL_SIZE 2

void process_image(unsigned char *data, size_t size);

typedef struct 
{
    unsigned char *buffer;
    int in_use;
}MemoryBlock;

typedef struct 
{
    MemoryBlock blocks[POOL_SIZE];
    pthread_mutex_t lock;
    pthread_cond_t cond;
}MemoryPool;

MemoryPool pool;
volatile sig_atomic_t server_running = 1;
int total_clients_processed = 0;
pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;

void handle_sigint(int sig) {
    (void)sig; // unused
    log_message(LOG_INFO, "Shutdown signal received. Stopping server...");
    server_running = 0;
}

void init_pool()
{
    pthread_mutex_init(&pool.lock, NULL);
    pthread_cond_init(&pool.cond, NULL);
    for(int i=0; i<POOL_SIZE; i++)
    {
        pool.blocks[i].buffer = malloc(IMG_SIZE);
        if(!pool.blocks[i].buffer)
        {
            log_message(LOG_ERROR, "Memory allocation failed");
            exit(1);
        }
        pool.blocks[i].in_use = 0;
    }
}

unsigned char* pool_alloc(int *block_index)
{
    pthread_mutex_lock(&pool.lock);
    while(1)
    {
        for(int i=0; i<POOL_SIZE; i++)
        {
            if(!pool.blocks[i].in_use)
            {
                pool.blocks[i].in_use = 1;
                *block_index = i;
                pthread_mutex_unlock(&pool.lock);
                return pool.blocks[i].buffer;
            }
        }
        log_message(LOG_INFO, "Pool full. Waiting for a slot...");
        pthread_cond_wait(&pool.cond, &pool.lock);
    }
    pthread_mutex_unlock(&pool.lock);
    return NULL;
}

void pool_free(int block_index)
{
    pthread_mutex_lock(&pool.lock);
    pool.blocks[block_index].in_use = 0;
    pthread_cond_signal(&pool.cond);
    pthread_mutex_unlock(&pool.lock);
}

void *handle_client(void *arg)
{
    int client_socket = *(int*)arg;
    free(arg);
    int block_idx = -1;
    unsigned char *img_buffer = pool_alloc(&block_idx);
    if(!img_buffer)
    {
        log_message(LOG_WARN, "Server busy: Memory pool is full.");
        close(client_socket);
        return NULL;
    }
    int total_read = 0;
    while(total_read < IMG_SIZE)
    {
        int bytes = recv(client_socket, img_buffer+total_read, IMG_SIZE-total_read, 0);
        if(bytes <= 0) break;
        total_read = total_read + bytes;
    }
    if(total_read == IMG_SIZE)
    {
        process_image(img_buffer, IMG_SIZE);

        // Get unique client ID
        pthread_mutex_lock(&id_lock);
        total_clients_processed++;
        int my_id = total_clients_processed;
        pthread_mutex_unlock(&id_lock);

        // Send ID then Data
        send(client_socket, &my_id, sizeof(int), 0);
        send(client_socket, img_buffer, IMG_SIZE, 0);
        log_message(LOG_INFO, "Processed image for client on socket %d (Assigned ID: %d)", client_socket, my_id);
    }
    else
        log_message(LOG_WARN, "Incomplete data received.");
    
    pool_free(block_idx);
    close(client_socket);
    return NULL;
}

void process_image(unsigned char *data, size_t size)
{
    log_message(LOG_INFO, "Simulating heavy processing (Wait in the queue for 3s)...");
    sleep(3);
    size_t i;
    for(i=0; i<size; i++)
    {
        if((i+1)%4 != 0)
            data[i] = ~data[i];
    }
}

int main()
{
    init_pool();
    log_init("SERVER");
    
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Explicitly NO SA_RESTART
    sigaction(SIGINT, &sa, NULL);

    int server_fd, *new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        log_message(LOG_ERROR, "Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        log_message(LOG_ERROR, "Binding failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 10) < 0)
    {
        log_message(LOG_ERROR, "Listen failed");
        exit(EXIT_FAILURE);
    }
    log_message(LOG_INFO, "Server listening on port %d with SIMD and Memory Pool..", PORT);

    while(server_running)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if(client_fd < 0) {
            if (errno == EINTR) {
                if (!server_running) break;
                continue; 
            }
            log_message(LOG_WARN, "Accept failed");
            continue;
        }

        pthread_t thread_id;
        new_sock = malloc(sizeof(int));
        *new_sock = client_fd;
        
        if(pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0)
        {
            log_message(LOG_ERROR, "Could not create thread");
            free(new_sock);
        }
        pthread_detach(thread_id);
    }
    
    log_message(LOG_INFO, "Server shutdown complete.");
    pthread_mutex_destroy(&pool.lock);
    pthread_cond_destroy(&pool.cond);
    close(server_fd);
    
    // --- System Resource Report ---
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        printf("\n=== System Resource Usage ===\n");
        printf("User CPU Time: %ld.%06ld sec\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        printf("System CPU Time: %ld.%06ld sec\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        printf("Voluntary Context Switches: %ld\n", usage.ru_nvcsw);
        printf("Involuntary Context Switches: %ld\n", usage.ru_nivcsw);
        printf("=============================\n");
    }

    // --- Log Dump ---
    printf("\n=== Application Log Dump ===\n");
    FILE *fp = fopen("application.log", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
        }
        fclose(fp);
    } else {
        printf("Could not read application.log\n");
    }
    printf("=============================\n");

    return 0;
}