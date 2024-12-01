#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

// Constants
#define FIFO_NAME "logFifo"
#define LOG_FILE "gateway.log"
#define MAX_LOG_MSG 256

// Function prototypes
void *connection_manager(void *arg);
void *data_manager(void *arg);
void *storage_manager(void *arg);
void log_event(const char *message);

// Log process function
void log_process() {
    int fd;
    char buffer[MAX_LOG_MSG];
    FILE *log_file = fopen(LOG_FILE, "w");

    if (!log_file) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(FIFO_NAME, 0666) == -1 && errno != EEXIST) {
        perror("Failed to create FIFO");
        exit(EXIT_FAILURE);
    }

    fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open FIFO");
        exit(EXIT_FAILURE);
    }

    printf("Log process started, waiting for log messages...\n");

    int sequence = 1;
    while (1) {
        ssize_t bytes = read(fd, buffer, MAX_LOG_MSG);
        if (bytes > 0) {
            buffer[bytes] = '\0';

            // Get timestamp
            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

            // Write log message to file
            fprintf(log_file, "%d %s %s\n", sequence++, timestamp, buffer);
            fflush(log_file);
        }
    }

    close(fd);
    fclose(log_file);
}

// Connection manager thread
void *connection_manager(void *arg) {
    sleep(1); // Simulate delay
    log_event("Sensor node 10 has opened a new connection");
    sleep(2);
    log_event("Sensor node 10 has closed the connection");
    return NULL;
}

// Data manager thread
void *data_manager(void *arg) {
    sleep(3); // Simulate processing delay
    log_event("Sensor node 10 reports it's too cold (running avg temperature = 18°C)");
    return NULL;
}

// Storage manager thread
void *storage_manager(void *arg) {
    sleep(5); // Simulate delay for storage
    log_event("Connection to SQL server established");
    sleep(1);
    log_event("New table sensors created");
    return NULL;
}

// Function to send log events via FIFO
void log_event(const char *message) {
    int fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Failed to open FIFO for writing");
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Log process
        log_process();
    } else {
        // Main process
        pthread_t conn_thread, data_thread, storage_thread;

        printf("Main process started\n");

        // Create threads
        pthread_create(&conn_thread, NULL, connection_manager, NULL);
        pthread_create(&data_thread, NULL, data_manager, NULL);
        pthread_create(&storage_thread, NULL, storage_manager, NULL);

        // Wait for threads to finish
        pthread_join(conn_thread, NULL);
        pthread_join(data_thread, NULL);
        pthread_join(storage_thread, NULL);

        printf("Main process finished\n");
    }

    return 0;
}
