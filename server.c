#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 1000
#define ITEM_NAME_SIZE 5000

int num_clients;

typedef struct {
    char name[ITEM_NAME_SIZE];
    int value;
} Item;

typedef struct {
    int client_socket;
    Item *items;
} ClientArgs;

typedef struct {
    int time;
    int lostConection;
} WaitConection;

int stringToInt(const char *str) 
{

    int result = atoi(str);

    if (result == 0 && str[0] != '0') {
        fprintf(stderr, "Error: No se pudo convertir la cadena a un entero.\n");
        exit(EXIT_FAILURE);
    }

    return result;
}

void *timer_thread(void *args) {
    WaitConection *timeActual = (WaitConection *) args;
    int flag = timeActual->lostConection;
    int timewait = timeActual->time; 
    sleep(timewait); 
    timeActual->lostConection = 1;
    pthread_exit(NULL);
}

void *handle_client(void *args) 
{
    num_clients++;
    ClientArgs *client_args = (ClientArgs *)args;
    int client_socket = client_args->client_socket;
    Item *items = client_args->items;

    char buffer[ITEM_NAME_SIZE + 10];
    char response[100];

    while (1) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            perror("Error al recibir datos del cliente");
            break;
        }

        buffer[bytes_received] = '\0';

        if (strcmp(buffer, "Chao") == 0) {
            break;
        }

        char *item_name = strtok(buffer, ",");
        int item_value = atoi(strtok(NULL, ","));

        int prev_value = 0;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(items[i].name, item_name) == 0) {
                prev_value = items[i].value;
                break;
            }
        }

        int found = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (strcmp(items[i].name, item_name) == 0) {
                items[i].value = item_value;
                found = 1;
                break;
            } else if (items[i].name[0] == '\0') {
                strcpy(items[i].name, item_name);
                items[i].value = item_value;
                found = 1;
                break;
            }
        }

        sprintf(response, "Item: %s, Valor previo: %d, Valor actual: %d", item_name, prev_value, item_value);
        if (send(client_socket, response, strlen(response), 0) == -1) {
            perror("Error al enviar datos al cliente");
            break;
        }
    }

    close(client_socket);
    free(args);
    num_clients--;
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{

    // Convierte los argumentos en enteros
    int client_num, time_wait;
    client_num = stringToInt(argv[1]);
    time_wait = stringToInt(argv[2]);
    num_clients = 0;

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    Item items[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        items[i].name[0] = '\0';
        items[i].value = 0;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        printf("Error al enlazar el socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        printf("Error al escuchar en el socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    
    //Temporizador
    // Inicializar el temporizador
    pthread_t timer_thread_id;

    WaitConection timeconection;
    timeconection.time = time_wait;
    timeconection.lostConection = 0;

    printf("Servidor esperando conexiones...\n");

    if (pthread_create(&timer_thread_id, NULL, timer_thread, &timeconection) != 0) {
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    pthread_join(timer_thread_id, NULL);
    
    
    while (timeconection.lostConection != 1) 
    {
        
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            printf("Error al aceptar la conexión del cliente");
            continue;
        }

        pthread_t client_thread;
        ClientArgs *client_args = (ClientArgs *)malloc(sizeof(ClientArgs));
        if (client_args == NULL) {
            printf("Error al asignar memoria para los argumentos del cliente");
            close(client_socket);
            continue;
        }

        client_args->client_socket = client_socket;
        client_args->items = items;

        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_args) != 0) {
            printf("Error al crear el hilo del cliente");
            free(client_args);
            close(client_socket);
        }
    }

    close(server_socket);

    return 0;
}