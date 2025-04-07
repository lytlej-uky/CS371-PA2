/*
# Copyright 2025 University of Kentucky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
*/

/* 
Please specify the group members here

# Student #1: Joshua Lytle
# Student #2: Drew Workman
# Student #3: Rian Gallagher

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_EVENTS 64
#define MESSAGE_SIZE 16
#define DEFAULT_CLIENT_THREADS 4

char *server_ip = "127.0.0.1";
int server_port = 12345;
int num_client_threads = DEFAULT_CLIENT_THREADS;
int num_requests = 1000000;

/*
 * This structure is used to store per-thread data in the client
 */
typedef struct {
    int epoll_fd;        /* File descriptor for the epoll instance, used for monitoring events on the socket. */
    int socket_fd;       /* File descriptor for the client socket connected to the server. */
    long long total_rtt; /* Accumulated Round-Trip Time (RTT) for all messages sent and received (in microseconds). */
    long total_messages; /* Total number of messages sent and received. */
    float request_rate;  /* Computed request rate (requests per second) based on RTT and total messages. */
} client_thread_data_t;

/*
 * This function runs in a separate client thread to handle communication with the server
 */
void *client_thread_func(void *arg) {
    client_thread_data_t *data = (client_thread_data_t *)arg;
    struct epoll_event event, events[MAX_EVENTS];
    char send_buf[MESSAGE_SIZE] = "ABCDEFGHIJKMLNOP"; /* Send 16-Bytes message every time */
    char recv_buf[MESSAGE_SIZE];
    struct timeval start, end;

    // Register the "connected" client_thread's socket in its epoll instance
    event.events = EPOLLOUT;
    event.data.fd = data->socket_fd;

    // Add the socket to the epoll instance
    if (epoll_ctl(data->epoll_fd, EPOLL_CTL_ADD, data->socket_fd, &event) == -1) {
        perror("epoll_ctl");
        pthread_exit(NULL);
    }

    // Set initial values to 0
    data->total_rtt = 0;
    data->total_messages = 0;
    data->request_rate = 0.0;

    for (int i = 0; i < num_requests; i++) {
        // Send message to server
        gettimeofday(&start, NULL);
        if (send(data->socket_fd, send_buf, MESSAGE_SIZE, 0) == -1) {
            perror("send");
            pthread_exit(NULL);
        }

        // Wait for response from server
        int wait_return = epoll_wait(data->epoll_fd, events, MAX_EVENTS, -1);
        if (wait_return == -1) {
            perror("epoll_wait");
            pthread_exit(NULL);
        }

        // Receive message from server
        if (recv(data->socket_fd, recv_buf, MESSAGE_SIZE, 0) == -1) {
            perror("recv");
            pthread_exit(NULL);
        }

        // Get end time and update totals
        gettimeofday(&end, NULL);
        data->total_messages++;
        data->total_rtt += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    }

    // Calculate request rate
    data->request_rate = (float)data->total_messages / (data->total_rtt / 1000000.0);

    // Close the socket and epoll instance
    close(data->socket_fd);
    close(data->epoll_fd);

    return NULL;
}

void run_client() {
    pthread_t threads[num_client_threads];
    client_thread_data_t thread_data[num_client_threads];
    struct sockaddr_in server_addr;

    for (int i = 0; i < num_client_threads; i++) {
        thread_data[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0); // create socket using SOCK_STREAM
        if (thread_data[i].socket_fd == -1) { // check if the socket was created correctly
            perror("socket");
            exit(EXIT_FAILURE);
        }

        thread_data[i].epoll_fd = epoll_create1(0); // create epoll instance
        if (thread_data[i].epoll_fd == -1) { // check if the epoll instance was created correctly
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        // convert IPv4 address from text to binary form
        if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
            perror("inet_pton");
            exit(EXIT_FAILURE);
        }

        // connect to the server
        if (connect(thread_data[i].socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_client_threads; i++) {
        // For each thread, launch a new one and pass the thread data
        pthread_create(&threads[i], NULL, client_thread_func, &thread_data[i]);
    }

    long long total_rtt = 0;
    long total_messages = 0;
    float total_request_rate = 0.0;
    for (int i = 0; i < num_client_threads; i++) {
        // Wait for the thread to complete
        pthread_join(threads[i], NULL); 

        total_rtt += thread_data[i].total_rtt;
        total_messages += thread_data[i].total_messages;
        total_request_rate += thread_data[i].request_rate;

        // Close the epoll file descriptor
        close(thread_data[i].epoll_fd);
    }

    printf("Average RTT: %lld us\n", total_rtt / total_messages);
    printf("Total Request Rate: %f messages/s\n", total_request_rate);
}

void run_server() {
    int server_fd;
    int epoll_fd;
    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    // bind the server to the ip and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // create the epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // add the server socket to the epoll instance
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        // wait for a client to connect
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            perror("epoll_wait");
            close(server_fd);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        // for each event received, handle it
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // accept the incoming connection
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd == -1) {
                    perror("accept");
                    continue; // if one errors try the next
                }
                
                // add the client socket to the epoll instance
                event.events = EPOLLIN;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("epoll_ctl");
                    close(client_fd);
                    continue; // if one errors try the next
                }
            } else {
                // existing connection receive data
                char recv_buf[MESSAGE_SIZE];
                if (recv(events[i].data.fd, recv_buf, MESSAGE_SIZE, 0) <= 0) {
                    close(events[i].data.fd);
                } else {
                    // send acknowledge message
                    if (send(events[i].data.fd, recv_buf, MESSAGE_SIZE, 0) == -1) {
                        perror("send");
                        close(events[i].data.fd);
                    }
                }
            }
        }
    }

    // close the file descriptors
    close(server_fd);
    close(epoll_fd);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "server") == 0) {
        if (argc > 2) server_ip = argv[2];
        if (argc > 3) server_port = atoi(argv[3]);

        run_server();
    } else if (argc > 1 && strcmp(argv[1], "client") == 0) {
        if (argc > 2) server_ip = argv[2];
        if (argc > 3) server_port = atoi(argv[3]);
        if (argc > 4) num_client_threads = atoi(argv[4]);
        if (argc > 5) num_requests = atoi(argv[5]);

        run_client();
    } else {
        printf("Usage: %s <server|client> [server_ip server_port num_client_threads num_requests]\n", argv[0]);
    }

    return 0;
}
