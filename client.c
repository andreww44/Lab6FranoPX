#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8000

int create_socket()
{
    int client_socket;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) 
    {
        printf("Error al crear el socket");
        exit(0);
    }

    return client_socket;
}

int connect_to_server(int client_socket)
{
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) 
    {
        printf("Error al conectar al servidor");
        close(client_socket);
        exit(0);
    }
}

int send_item_data(client_socket)
{
    char item_name[50];
    int item_value;

    printf("Ingrese el nombre del ítem (o 'Chao' para salir): ");
    fgets(item_name, sizeof(item_name), stdin);
    item_name[strcspn(item_name, "\n")] = '\0';

    if (strcmp(item_name, "Chao") == 0) {
        if (send(client_socket, item_name, strlen(item_name), 0) == -1) {
            printf("Error al enviar datos al servidor");
        }
        return 0;
    }

    printf("Ingrese el valor del ítem: ");
    scanf("%d", &item_value);

    char data[60];
    sprintf(data, "%s,%d", item_name, item_value);

    if (send(client_socket, data, strlen(data), 0) == -1) {
        printf("Error al enviar datos al servidor");
        close(client_socket);
        return -1;
    }
    return 1;
}

int server_response(int client_socket)
{
    char response[100];
    int bytes_received = recv(client_socket, response, sizeof(response) - 1, 0);
    if (bytes_received == -1) {
        printf("Error al recibir datos del servidor");
        close(client_socket);
        exit(0);
    } else if (bytes_received == 0) {
        printf("El servidor ha cerrado la conexión\n");
        close(client_socket);
        exit(0);
    }
    response[bytes_received] = '\0';
    printf("%s\n", response);
}

int main() {
    
    int socket = create_socket();
    connect_to_server(socket);

    while (1) {

        // Enviar datos de los items
        int item = send_item_data(socket);
        if(item == 0){
            break;
        } 
        else if(item == -1)
        {
            exit(0);
        }

        // Recibir respuesta del servidor (Cuando actualiza los datos)
        server_response(socket);

        // Limpiar el buffer de entrada
        while ((getchar()) != '\n');
    }

    close(socket);

    return 0;
}

