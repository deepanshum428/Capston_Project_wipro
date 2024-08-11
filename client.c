#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

void send_message(int sock, const char *message)
{
    if (send(sock, message, strlen(message), 0) < 0)
    {
        perror("Send failed");
        exit(1);
    }
}

int receive_message(int sock, char *buffer)
{
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0)
    {
        perror("Receive failed");
        exit(1);
    }
    buffer[bytes_received] = '\0';
    return bytes_received;
}

void print_prompt(const char *buffer)
{
    char *prompt = strstr(buffer, "Enter command");
    if (prompt)
    {
        printf("%s", prompt);
        fflush(stdout);
    }
}

int main()
{
    int sock = 0, exit = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to server.\n");

    while (exit == 0)
    {
        printf("Enter command (LIST / BOOK <train_id> <num_seats>): ");

        if (fgets(input, BUFFER_SIZE, stdin) == 0)
        {
            break;
        }
        input[strcspn(input, "\n")] = 0;

        send_message(sock, input);

        if (receive_message(sock, buffer) > 0)
        {
            printf("%s", buffer);
            print_prompt(buffer);
        }

        while (1)
        {
            printf("Are you willing to book? (YES/NO): ");
            if (fgets(input, BUFFER_SIZE, stdin) == 0)
            {
                break;
            }
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "NO") == 0)
            {
                exit = 1;
                break;
            }
            else if (strcmp(input, "YES") == 0)
            {
                break;
            }
            else
            {
                printf("Invalid command!\n");
            }
        }
    }

    close(sock);
    return 0;
}
