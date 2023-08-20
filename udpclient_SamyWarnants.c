#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define MAX_NUMBERS 10

void processTokens(const char *tokenString, int *numbersReceived, int *localMaxValue) {
    char *token = strtok((char *)tokenString, ",");
    while (token != NULL && *numbersReceived < MAX_NUMBERS) {
        int receivedValue = atoi(token);
        printf("Last received value: %d\n", receivedValue);
        if (receivedValue > *localMaxValue) {
            *localMaxValue = receivedValue;
        }
        token = strtok(NULL, ",");
        (*numbersReceived)++;
    }
}

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 0), &wsaData);

    struct addrinfo internet_address_setup;
    struct addrinfo *internet_address = NULL;
    memset(&internet_address_setup, 0, sizeof internet_address_setup);
    internet_address_setup.ai_family = AF_UNSPEC;
    internet_address_setup.ai_socktype = SOCK_DGRAM;
    getaddrinfo("::1", "24042", &internet_address_setup, &internet_address);
    int internet_socket;
    internet_socket = socket(internet_address->ai_family, internet_address->ai_socktype, internet_address->ai_protocol);

    srand(time(NULL)); // Seed the random number generator

    char buffer[1000];
    socklen_t internet_address_length = internet_address->ai_addrlen;
    int numbersReceived = 0;
    int localMaxValue = -2147483647;

    while (1) {
        char go_message[] = "GO";
        sendto(internet_socket, go_message, strlen(go_message), 0, internet_address->ai_addr, internet_address->ai_addrlen);
        printf("%s\n", go_message);

        for (int attempt = 0; attempt < 3; attempt++) {
            numbersReceived = 0;
            localMaxValue = -2147483647;

            while (numbersReceived < MAX_NUMBERS) {
                int number_of_bytes_received = recvfrom(internet_socket, buffer, (sizeof buffer) - 1, 0, internet_address->ai_addr, &internet_address_length);
                buffer[number_of_bytes_received] = '\0';

                if (strcmp(buffer, "OK") == 0) {
                    printf("Received %s, shutting down\n", buffer);
                    goto cleanup; // Exit the loop and go to cleanup
                }

                processTokens(buffer, &numbersReceived, &localMaxValue);
            }

            if (localMaxValue != -2147483647) {
                printf("Sorting through %d received numbers...\n", numbersReceived);
                printf("The highest value among the received numbers is: %d\n", localMaxValue);

                for (int i = 0; i < 3; i++) {
                    // Add a delay of one second
                    Sleep(1000);

                    char max_value_string[10];
                    sprintf(max_value_string, "%d", localMaxValue);
                    sendto(internet_socket, max_value_string, strlen(max_value_string), 0, internet_address->ai_addr, internet_address->ai_addrlen);
                    printf("Sent %s (Attempt %d)\n", max_value_string, i + 1);
                }
            } else {
                printf("No valid numbers were received.\n");
            }

            Sleep(2000); // Wait for 2 seconds before the next attempt
        }
    }

cleanup:
    // cleanup
    freeaddrinfo(internet_address);
    closesocket(internet_socket);
    WSACleanup();
    return 0;
}
//uint32_t network_number = htonl(random_num); 
