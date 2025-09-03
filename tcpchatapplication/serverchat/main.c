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
#define NAME_LEN 32
#define BUFFER_SIZE 256

typedef struct {
    int fd;
    char name[NAME_LEN];
    int named; // 0 = not yet sent name, 1 = name set
} client_t;

client_t clients[MAX_CLIENT];

void initialise()
{
    for(int i = 0; i < MAX_CLIENT; i++) {
        clients[i].fd = -1;
        clients[i].named = 0;
        memset(clients[i].name, 0, NAME_LEN);
    }
}

void add_client(int sock)
{
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd == -1) {
            clients[i].fd = sock;
            clients[i].named = 0; // will set after first message
            break;
        }
    }
}

void remove_client(int fd)
{
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd == fd) {
            close(clients[i].fd);
            clients[i].fd = -1;
            clients[i].named = 0;
            memset(clients[i].name, 0, NAME_LEN);
            break;
        }
    }
}

void re_initfd(fd_set* readfd, int master_socket)
{
    FD_ZERO(readfd);
    FD_SET(master_socket, readfd);
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd != -1) {
            FD_SET(clients[i].fd, readfd);
        }
    }
}

int get_max(int master_socket)
{
    int maxi = master_socket;
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd > maxi) {
            maxi = clients[i].fd;
        }
    }
    return maxi;
}

void broadcast_user_list()
{
    char list[BUFFER_SIZE];
    strcpy(list, "[Users Online]: ");

    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd != -1 && clients[i].named) {
            strcat(list, clients[i].name);
            strcat(list, " ");
        }
    }
    strcat(list, "\n");

    // send to all clients
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd != -1) {
            send(clients[i].fd, list, strlen(list), 0);
        }
    }

    // also print on server
    printf("%s", list);
}

void broadcast_message(const char* msg, int exclude_fd)
{
    for(int i = 0; i < MAX_CLIENT; i++) {
        if(clients[i].fd != -1 && clients[i].fd != exclude_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

int main()
{
    int master_socket, comm_socket, ret;
    fd_set readfd;
    socklen_t client_len;
    struct sockaddr_in server, client;
    char buffer[BUFFER_SIZE];

    initialise();

    // master socket
    master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(master_socket < 0) {
        perror("socket");
        exit(1);
    }

    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);
    server.sin_family = AF_INET;

    ret = bind(master_socket, (const struct sockaddr*)&server, sizeof(server));
    if(ret == -1) {
        perror("bind");
        exit(1);
    }

    ret = listen(master_socket, MAX_CLIENT);
    if(ret == -1) {
        perror("listen");
        exit(1);
    }

    printf("Chat server started on port %d...\n", PORT);

    while(1) {
        re_initfd(&readfd, master_socket);
        ret = select(get_max(master_socket) + 1, &readfd, NULL, NULL, NULL);
        if(ret == -1) {
            perror("select");
            exit(1);
        }

        client_len = sizeof(client);

        // new connection
        if(FD_ISSET(master_socket, &readfd)) {
            comm_socket = accept(master_socket, (struct sockaddr*)&client, &client_len);
            if(comm_socket <= -1) {
                perror("accept");
                exit(1);
            }
            add_client(comm_socket);
            printf("New connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            send(comm_socket, "Enter your name: ", 17, 0);
        }

        // handle all clients
        for(int i = 0; i < MAX_CLIENT; i++) {
            if(clients[i].fd != -1 && FD_ISSET(clients[i].fd, &readfd)) {
                memset(buffer, 0, sizeof(buffer));
                int n = read(clients[i].fd, buffer, sizeof(buffer) - 1);

                if(n <= 0) {
                    printf("Client (fd=%d) disconnected\n", clients[i].fd);
                    remove_client(clients[i].fd);
                } else {
                    buffer[n] = '\0';

                    // first message = username
                    if(clients[i].named == 0) {
                        strncpy(clients[i].name, buffer, NAME_LEN - 1);
                        clients[i].named = 1;
                        printf("Client (fd=%d) set name: %s\n", clients[i].fd, clients[i].name);

                        char welcome[BUFFER_SIZE];
                        snprintf(welcome, sizeof(welcome), "Welcome %s!\n", clients[i].name);
                        send(clients[i].fd, welcome, strlen(welcome), 0);

                        // announce new user
                        char announce[BUFFER_SIZE];
                        snprintf(announce, sizeof(announce), "[Server]: %s joined the chat\n", clients[i].name);
                        broadcast_message(announce, clients[i].fd);

                        // send updated user list
                        broadcast_user_list();
                    } else {
                        // normal message
                        printf("Sender [%s]: %s", clients[i].name, buffer);

                        if(strncmp(buffer, "exit", 4) == 0) {
                            printf("%s disconnected.\n", clients[i].name);

                            char announce[BUFFER_SIZE];
                            snprintf(announce, sizeof(announce), "[Server]: %s left the chat\n", clients[i].name);
                            broadcast_message(announce, clients[i].fd);

                            remove_client(clients[i].fd);

                            // update user list
                            broadcast_user_list();

                        } else {
                            // broadcast to others
                            char msg[BUFFER_SIZE + NAME_LEN];
                            snprintf(msg, sizeof(msg), "%s: %s", clients[i].name, buffer);
                            for(int j = 0; j < MAX_CLIENT; j++) {
                                if(clients[j].fd != -1 && clients[j].fd != master_socket &&
                                   clients[j].fd != clients[i].fd) {
                                    send(clients[j].fd, msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
