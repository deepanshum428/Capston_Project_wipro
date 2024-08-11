#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_TRAINS 10
#define MAX_SEATS 120

typedef struct
{
    int available_seats;
    char train_name[50];
} Train;

Train trains[MAX_TRAINS];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void init_trains()
{
    for (int i = 0; i < MAX_TRAINS; i++)
    {
        sprintf(trains[i].train_name, "Train_%d", i + 1);
        trains[i].available_seats = MAX_SEATS;
    }
}

void list_trains(int client_sock)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < MAX_TRAINS; i++)
    {
        sprintf(buffer + strlen(buffer), "Train: %s, Available Seats: %d\n", trains[i].train_name, trains[i].available_seats);
    }
    if (send(client_sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("Send failed");
    }
    else
    {
        printf("Train data send");
    }
}

void book_seats(int client_sock, int train_id, int num_seats)
{
    pthread_mutex_lock(&lock);
    if (train_id >= 0 && train_id < MAX_TRAINS)
    {
        if (trains[train_id].available_seats >= num_seats)
        {
            trains[train_id].available_seats -= num_seats;
            char response[256];
            sprintf(response, "Booking successful. %d seats booked on %s.\n", num_seats, trains[train_id].train_name);
            if (send(client_sock, response, strlen(response), 0) < 0)
            {
                perror("Send failed");
            }
        }
        else
        {
            char response[] = "Not enough seats available.\n";
            if (send(client_sock, response, strlen(response), 0) < 0)
            {
                perror("Send failed");
            }
        }
    }
    else
    {
        char response[] = "Invalid train ID.\n";
        if (send(client_sock, response, strlen(response), 0) < 0)
        {
            perror("Send failed");
        }
    }
    pthread_mutex_unlock(&lock);
}

void *handle_client(void *arg)
{
    int client_sock = *(int *)arg;
    free(arg);

    char buffer[1024];
    int bytes_read;


    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_read] = '\0';
        if (strncmp(buffer, "LIST", 4) == 0)
        {
            list_trains(client_sock);
        }
        else if (strncmp(buffer, "BOOK", 4) == 0)
        {
            int train_id, num_seats;
            sscanf(buffer, "BOOK %d %d", &train_id, &num_seats);
            book_seats(client_sock, train_id - 1, num_seats);
        }
        else if (strncmp(buffer, "NO", 2) == 0)
        {
            char message[256];
            sprintf(message, "Goodbye!\n");
            if (send(client_sock, message, strlen(message), 0) < 0)
            {
                perror("Send failed");
            }
            break;
        }
    }

    close(client_sock);
    return 0;
}

int main()
{
    init_trains();

    int server_sock, client_sock;

    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 1;
    }
    else
    {
        printf("Socket creation success");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        return 1;
    }

    printf("Server started. Listening for incoming connections...\n");

    while (1)
    {
        if ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            perror("Accept failed");
            continue;
        }

        printf("Connected to client IP address %s and port %d...\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        int *arg = malloc(sizeof(int));
        *arg = client_sock;

        pthread_t thread;
        if (pthread_create(&thread, 0, handle_client, arg) < 0)
        {
            perror("pthread_create failed");
            continue;
        }
    }

    return 0;
}
