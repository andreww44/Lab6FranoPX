#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int client_socket;
    struct sockaddr_in server_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(12345);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al conectar al servidor");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    while (1) {
        char item_name[50];
        int item_value;

        printf("Ingrese el nombre del ítem (o 'Chao' para salir): ");
        fgets(item_name, sizeof(item_name), stdin);
        item_name[strcspn(item_name, "\n")] = '\0';

        if (strcmp(item_name, "Chao") == 0) {
            if (send(client_socket, item_name, strlen(item_name), 0) == -1) {
                perror("Error al enviar datos al servidor");
            }
            break;
        }

        printf("Ingrese el valor del ítem: ");
        scanf("%d", &item_value);

        char data[60];
        sprintf(data, "%s,%d", item_name, item_value);

        if (send(client_socket, data, strlen(data), 0) == -1) {
            perror("Error al enviar datos al servidor");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        char response[100];
        int bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
        if (bytes_received == -1) {
            perror("Error al recibir datos del servidor");
            close(client_socket);
            exit(EXIT_FAILURE);
        } else if (bytes_received == 0) {
            printf("El servidor ha cerrado la conexión\n");
            close(client_socket);
            exit(EXIT_SUCCESS);
        }

        response[bytes_received] = '\0';
        printf("%s\n", response);

        // Limpiar el buffer de entrada
        while ((getchar()) != '\n');
    }

    close(client_socket);

    return 0;
}

