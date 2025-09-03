#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENT 8

int monitor_fd[MAX_CLIENT];

void initialise() {
    for (int i = 0; i < MAX_CLIENT; i++) {
        monitor_fd[i] = -1;
    }
}

void add_tofd(int sock) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (monitor_fd[i] == -1) {
            monitor_fd[i] = sock;
            break;
        }
    }
}

void re_initfd(fd_set *readfd) {
    FD_ZERO(readfd);
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (monitor_fd[i] != -1) {
            FD_SET(monitor_fd[i], readfd);
        }
    }
}

int get_max() {
    int maxi = -1;
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (monitor_fd[i] > maxi) {
            maxi = monitor_fd[i];
        }
    }
    return maxi;
}

void remove_fd(int fd) {
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (monitor_fd[i] == fd) {
            monitor_fd[i] = -1;
        }
    }
}

int main(int argc, char **argv) {
    int master_socket, comm_socket, ret;
    fd_set readfd;
    socklen_t client_len;
    struct sockaddr_in server, client;
    char buffer[108];

    initialise();

    // creating master socket
    master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (master_socket < 0) {
        perror("socket");
        exit(1);
    }

    // initialise server structure
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    server.sin_family = AF_INET;

    ret = bind(master_socket, (const struct sockaddr *)&server, sizeof(server));
    if (ret == -1) {
        perror("bind");
        exit(1);
    }

    ret = listen(master_socket, MAX_CLIENT);
    if (ret == -1) {
        perror("listen");
        exit(1);
    }

    add_tofd(master_socket);

    while (1) {
        re_initfd(&readfd);
        printf("Waiting for client........\n");
        ret = select(get_max() + 1, &readfd, NULL, NULL, NULL);
        client_len = sizeof(client);

        if (ret == -1) {
            perror("select");
            exit(1);
        }

        // check if new connection arrives on master socket
        if (FD_ISSET(master_socket, &readfd)) {
            comm_socket = accept(master_socket, (struct sockaddr *)&client, &client_len);
            if (comm_socket <= -1) {
                perror("accept");
                exit(1);
            }
            add_tofd(comm_socket);
            printf("Connection established with %s and port %d\n",
                   inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        }

        // Check all client sockets
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (monitor_fd[i] != -1 && monitor_fd[i] != master_socket &&
                FD_ISSET(monitor_fd[i], &readfd)) {
                comm_socket = monitor_fd[i];
                memset(buffer, 0, sizeof(buffer));
                int n = read(comm_socket, buffer, sizeof(buffer) - 1);

                if (n <= 0) {
                    printf("Client (fd=%d) disconnected\n", comm_socket);
                    close(comm_socket);
                    remove_fd(comm_socket);
                } else {
                    buffer[n] = '\0';
                    printf("Server read (fd=%d): %s\n", comm_socket, buffer);

                    if (strcmp(buffer, "exit\n") == 0) {
                        printf("Closing client %d\n", comm_socket);
                        close(comm_socket);
                        remove_fd(comm_socket);
                    } else {
                        // Broadcast to all other clients
                        for (int j = 0; j < MAX_CLIENT; j++) {
                            if (monitor_fd[j] != -1 &&
                                monitor_fd[j] != master_socket &&
                                monitor_fd[j] != comm_socket) {
                                send(monitor_fd[j], buffer, strlen(buffer), 0);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
