#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <stdio.h>    //for fprintf, perror
#include <stdlib.h>   //for exit
#include <string.h>   //for memset
#include <unistd.h>   //for close
#include <winsock2.h> //for all socket programming
#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
void OSInit(void) {
  WSADATA wsaData;
  int WSAError = WSAStartup(MAKEWORD(2, 0), &wsaData);
  if (WSAError != 0) {
    fprintf(stderr, "WSAStartup errno = %d\n", WSAError);
    exit(-1);
  }
}
void OSCleanup(void) { WSACleanup(); }
#define perror(string)                                                         \
  fprintf(stderr, string ": WSA errno = %d\n", WSAGetLastError())
#else
#include <arpa/inet.h>  //for htons, htonl, inet_pton, inet_ntop
#include <errno.h>      //for errno
#include <netdb.h>      //for getaddrinfo
#include <netinet/in.h> //for sockaddr_in
#include <stdio.h>      //for fprintf, perror
#include <stdlib.h>     //for exit
#include <string.h>     //for memset
#include <sys/socket.h> //for sockaddr, socket, socket
#include <sys/types.h>  //for size_t
#include <unistd.h>     //for close
void OSInit(void) {}
void OSCleanup(void) {}
#endif
#include <math.h>

int initialization();
int connection(int internet_socket);
void execution(int internet_socket);
void cleanup(int internet_socket, int client_internet_socket);

int main(int argc, char *argv[]) {
  //////////////////
  // Initialization//
  //////////////////

  OSInit();
  printf("Server online\n");
  int internet_socket = initialization();

  //////////////
  // Connection//
  //////////////

  int client_internet_socket = connection(internet_socket);

  /////////////
  // Execution//
  /////////////

  execution(client_internet_socket);

  ////////////
  // Clean up//
  ////////////

  cleanup(internet_socket, client_internet_socket);

  OSCleanup();

  return 0;
}

int initialization() {
  // Step 1.1
  struct addrinfo internet_address_setup;
  struct addrinfo *internet_address_result;
  memset(&internet_address_setup, 0, sizeof internet_address_setup);
  internet_address_setup.ai_family = AF_UNSPEC;
  internet_address_setup.ai_socktype = SOCK_STREAM;
  internet_address_setup.ai_flags = AI_PASSIVE;
  int getaddrinfo_return = getaddrinfo(NULL, "24042", &internet_address_setup,
                                       &internet_address_result);
  if (getaddrinfo_return != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(getaddrinfo_return));
    exit(1);
  }

  int internet_socket = -1;
  struct addrinfo *internet_address_result_iterator = internet_address_result;
  while (internet_address_result_iterator != NULL) {
    // Step 1.2
    internet_socket = socket(internet_address_result_iterator->ai_family,
                             internet_address_result_iterator->ai_socktype,
                             internet_address_result_iterator->ai_protocol);
    if (internet_socket == -1) {
      perror("socket");
    } else {
      // Step 1.3
      int bind_return =
          bind(internet_socket, internet_address_result_iterator->ai_addr,
               internet_address_result_iterator->ai_addrlen);
      if (bind_return == -1) {
        perror("bind");
        close(internet_socket);
      } else {
        // Step 1.4
        int listen_return = listen(internet_socket, 1);
        if (listen_return == -1) {
          close(internet_socket);
          perror("listen");
        } else {
          break;
        }
      }
    }
    internet_address_result_iterator =
        internet_address_result_iterator->ai_next;
  }

  freeaddrinfo(internet_address_result);

  if (internet_socket == -1) {
    fprintf(stderr, "socket: no valid socket address found\n");
    exit(2);
  }

  return internet_socket;
}

int connection(int internet_socket) {

  // Step 2.1
  struct sockaddr_storage client_internet_address;
  socklen_t client_internet_address_length = sizeof client_internet_address;
  int client_socket =
      accept(internet_socket, (struct sockaddr *)&client_internet_address,
             &client_internet_address_length);
  if (client_socket == -1) {
    perror("accept");
    close(internet_socket);
    exit(3);
  }
  return client_socket;
}

void execution(int internet_socket) {
 
  send(internet_socket, "\nWelcome to the Calculator! It uses '+' to add, '-' to subtract, '/' to devide, '*' to multiply, '^' to get numbers to the power! good luck!\n", 140, 0);
  // Step 3.1
  int number_of_bytes_received = 0;
  char buffer[1000];

  char operator;
  int number1 = 0;
  int number2 = 0;
  int result = 0;
  int status = 1;
  int status2 = 1;
  int loadtime = rand() % 200000 + 1000;

  while ((status = 1) && (status2 = 1)) {
    number1 = 0;
    number2 = 0;
    char CharToSend[1000];

    number_of_bytes_received =
        recv(internet_socket, buffer, (sizeof buffer) - 1, 0);
    if (number_of_bytes_received == -1) {
      perror("recv");
    } else {
      buffer[number_of_bytes_received] = '\0';
      sscanf(buffer, "%d%c%d\n", &number1, &operator, & number2);
      printf("\nServer_Recieved = %s\n", buffer);
      if (strcmp(buffer, "STOP\r") == 0) {
        send(internet_socket, "OK\r", 4, 0);
        status = 0;
      } else if (strcmp(buffer, "KTHNXBYE\r") == 0) {
        status2 = 0;
        printf("shutting down");
        break;
      }
    }
    if (status) {
      printf("%d %c %d\n", number1, operator,
             number2); //'+','-','/','*','%','^'

      switch (operator) {

      case '-':
        result = number1 - number2;
        break;
      case '+':
        result = number1 + number2;
        break;
      case '/':
        result = number1 / number2;
        break;
      case '*':
        result = number1 * number2;
        break;      
      case '^':
        result = pow(number1, number2);
        break;

      default:
        break;
      }

      // Step 3.2
      itoa(result, CharToSend, 10);
      int number_of_bytes_send = 0;
      number_of_bytes_send =
          send(internet_socket, CharToSend, strlen(CharToSend), 0);
      if (number_of_bytes_send == -1) {
        perror("send");
      } else {
        printf("sent: %s\n", CharToSend);
      }
    }
    if (status == 0) {

      int number_of_bytes_send = 0;
      printf("awaiting shutdownrequest");
      for (int p = 0; p < 20; p++) {
        printf(".");
        fflush(stdout);
        usleep(loadtime);
      }
      number_of_bytes_send =
          send(internet_socket, "awaiting shutdownrequest\n", 26, 0);

      if (number_of_bytes_send == -1) {
        perror("send");
      }
    }
    if (strcmp(buffer, "KTHNXBYE\r") == 0) {
      status2 = 0;
    }
  }
}

void cleanup(int internet_socket, int client_internet_socket) {
  // Step 4.2
  int shutdown_return = shutdown(client_internet_socket, SD_RECEIVE);
  if (shutdown_return == -1) {
    perror("shutdown");
  }

  // Step 4.1
  close(client_internet_socket);
  close(internet_socket);
}