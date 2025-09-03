#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    fd_set readfds;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    //    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    printf("Connected to server. Type messages...\n");

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);    // stdin
        FD_SET(sock, &readfds); // server
        int max_fd = sock;

        select(max_fd + 1, &readfds, NULL, NULL, NULL);
        printf("sender: ");
        if(FD_ISSET(0, &readfds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            send(sock, buffer, strlen(buffer), 0);
        }

        if(FD_ISSET(sock, &readfds)) {
            int valread = read(sock, buffer, BUFFER_SIZE);
            if(valread > 0) {
                buffer[valread] = '\0';
                printf("Message: %s", buffer);
            }
        }
    }
    return 0;
}
