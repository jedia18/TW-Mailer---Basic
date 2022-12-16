#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>

///////////////////////////////////////////////////////////////////////////////

#define BUF 1024
//#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

int isQuit;

///////////////////////////////////////////////////////////////////////////////

char userInput(char buffer[BUF]);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    int create_socket;
    char buffer[BUF];
    struct sockaddr_in address;
    int size;
    int PORT;
    int input_rows;

    if (argc != 3)
    {
        printf("Usage: %s <ip-adress> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else
    {
        PORT = atoi(argv[2]);
    }

    ////////////////////////////////////////////////////////////////////////////
    // CREATE A SOCKET
    // https://man7.org/linux/man-pages/man2/socket.2.html
    // https://man7.org/linux/man-pages/man7/ip.7.html
    // https://man7.org/linux/man-pages/man7/tcp.7.html
    // IPv4, TCP (connection oriented), IP (same as server)
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // INIT ADDRESS
    // Attention: network byte order => big endian
    memset(&address, 0, sizeof(address)); // init storage with 0
    address.sin_family = AF_INET;         // IPv4
    // https://man7.org/linux/man-pages/man3/htons.3.html
    address.sin_port = htons(PORT);
    // https://man7.org/linux/man-pages/man3/inet_aton.3.html
    if (argc < 2)
    {
        inet_aton("127.0.0.1", &address.sin_addr);                                                                    //!anschauen
    }
    else
    {
        inet_aton(argv[1], &address.sin_addr);
    }

    ////////////////////////////////////////////////////////////////////////////
    // CREATE A CONNECTION
    // https://man7.org/linux/man-pages/man2/connect.2.html
    if (connect(create_socket,
                (struct sockaddr *)&address,
                sizeof(address)) == -1)
    {
        // https://man7.org/linux/man-pages/man3/perror.3.html
        perror("Connect error - no server available");
        return EXIT_FAILURE;
    }

    // ignore return value of printf
    printf("Connection with server (%s) established\n",
           inet_ntoa(address.sin_addr));

    ////////////////////////////////////////////////////////////////////////////
    // RECEIVE DATA
    // https://man7.org/linux/man-pages/man2/recv.2.html



    do
    {
        size = recv(create_socket, buffer, BUF - 1, 0);
        if (size == -1)
        {
            perror("recv error");
        }
        else if (size == 0)
        {
            printf("Server closed remote socket\n"); // ignore error
        }
        else
        {
            buffer[size] = '\0';
            printf("%s", buffer); // ignore error
        }

        userInput(buffer);

        if ((send(create_socket, buffer, size, 0)) == -1)
        {
            perror("send error");
            break;
        }

        ////////////////////////////////////////////////////////////////////////////
        // Main Menue
        int input1 = 0;

        if (strcmp (buffer, "SEND") == 0)
            input1 = 1;

        if (strcmp (buffer, "LIST") == 0)
            input1 = 2;

        if (strcmp (buffer, "READ") == 0)
            input1 = 3;

        if (strcmp (buffer, "DEL") == 0)
            input1 = 4;

        if (strcmp (buffer, "QUIT") == 0)
            input1 = 5;

        switch (input1)
        {
        case 1 :
            //////////////////////////////////////////////////////////////////////
            // SEND


            input_rows = 0;
            do
            {
                if (input_rows < 4)
                {
                    size = recv(create_socket, buffer, BUF - 1, 0);
                    if (size == -1)
                    {
                        perror("recv error");
                        break;
                    }
                    else if (size == 0)
                    {
                        printf("Server closed remote socket\n"); // ignore error
                        break;
                    }
                    else
                    {
                        buffer[size] = '\0';
                        printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

                    }

                }
                userInput(buffer);
                size = strlen(buffer);

                if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
                {
                    perror("send error");
                    break;
                }
                input_rows++;
                buffer[size] = '\0';
            }
            while ((buffer[0] != '.') && (strlen(buffer) != 1) );

            break;

        case 2 :
            //////////////////////////////////////////////////////////////////////
            // LIST

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }

            userInput(buffer);

            if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }

            break;

        case 3 :
            //////////////////////////////////////////////////////////////////////
            // READ
            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            userInput(buffer);

            if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            userInput(buffer);

            if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            break;

        case 4 :
            //////////////////////////////////////////////////////////////////////
            // DEL
            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            userInput(buffer);

            if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            userInput(buffer);

            if ((send(create_socket, buffer, strlen(buffer), 0)) == -1)
            {
                perror("send error");
                break;
            }

            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }

            break;

        case 5 :
            //////////////////////////////////////////////////////////////////////
            // QUIT

            break;

        default:
            //////////////////////////////////////////////////////////////////////
            // DEFAULT
            size = recv(create_socket, buffer, BUF - 1, 0);
            if (size == -1)
            {
                perror("recv error");
                break;
            }
            else if (size == 0)
            {
                printf("Server closed remote socket\n"); // ignore error
                break;
            }
            else
            {
                buffer[size] = '\0';
                printf("<< %s\n", buffer); // ignore error // hier Ok or ERR printed in client

            }
            break;
        }
    }
    while (!isQuit);

    ////////////////////////////////////////////////////////////////////////////
    // CLOSES THE DESCRIPTOR
    if (create_socket != -1)
    {
        if (shutdown(create_socket, SHUT_RDWR) == -1)
        {
            // invalid in case the server is gone already
            perror("shutdown create_socket");
        }
        if (close(create_socket) == -1)
        {
            perror("close create_socket");
        }
        create_socket = -1;
    }
    return EXIT_SUCCESS;
}



char userInput(char buffer[BUF])
{
    printf(">> ");
    if (fgets(buffer, BUF, stdin) != NULL)
    {
        int size = strlen(buffer);
        // remove new-line signs from string at the end
        if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
        {
            size -= 2;
            buffer[size] = 0;
        }
        else if (buffer[size - 1] == '\n')
        {
            --size;
            buffer[size] = 0;
        }
        isQuit = strcmp(buffer, "QUIT") == 0;
    }
    return *buffer;
}

