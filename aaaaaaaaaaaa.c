#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define ITEM_NAME_SIZE 50

typedef struct {
    char name[ITEM_NAME_SIZE];
    int value;
} Item;

typedef struct {
    int client_socket;
    Item *items;
} ClientArgs;

void *handle_client(void *args) {
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
    pthread_exit(NULL);
}

void *timer_thread(void *args) {
    int *shutdown_flag = (int *)args;
    sleep(30); // Esperar 30 segundos antes de apagar el servidor
    *shutdown_flag = 1;
    pthread_exit(NULL);
}

int main() {
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
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(12345);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al enlazar el socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error al escuchar en el socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor esperando conexiones...\n");

    // Inicializar el temporizador
    pthread_t timer_thread_id;
    int shutdown_flag = 0;
    if (pthread_create(&timer_thread_id, NULL, timer_thread, (void *)&shutdown_flag) != 0) {
        perror("Error al crear el hilo del temporizador");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Configurar el tiempo de espera
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Conjunto de descriptores de lectura
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &read_fds);

    // Esperar hasta que haya una conexión o se alcance el tiempo de espera
    int ready = select(server_socket + 1, &read_fds, NULL, NULL, &timeout);

    if (ready == -1) {
        perror("Error en select");
        close(server_socket);
        exit(EXIT_FAILURE);
    } else if (ready == 0) {
        if (shutdown_flag) {
            printf("Tiempo de espera agotado. Cerrando el servidor.\n");
            close(server_socket);
            pthread_join(timer_thread_id, NULL);
            exit(EXIT_SUCCESS);
        }
    }

    // Se acepta la conexión del cliente
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket == -1) {
        perror("Error al aceptar la conexión del cliente");
        close(server_socket);
        pthread_join(timer_thread_id, NULL);
        exit(EXIT_FAILURE);
    }

    printf("Conexión establecida desde %s\n", inet_ntoa(client_addr.sin_addr));

    // Cancelar el temporizador
    pthread_cancel(timer_thread_id);

    // Crear un hilo para manejar al cliente
    pthread_t client_thread;
    ClientArgs *client_args = (ClientArgs *)malloc(sizeof(ClientArgs));
    if (client_args == NULL) {
        perror("Error al asignar memoria para los argumentos del cliente");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    client_args->client_socket = client_socket;
    client_args->items = items;

    if (pthread_create(&client_thread, NULL, handle_client, (void *)client_args) != 0) {
        perror("Error al crear el hilo del cliente");
        free(client_args);
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    pthread_join(client_thread, NULL);

    close(server_socket);

    return 0;
}
