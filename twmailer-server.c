#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>


///////////////////////////////////////////////////////////////////////////////

#define BUF 1024
//#define PORT 6543

///////////////////////////////////////////////////////////////////////////////

int abortRequested = 0;
int create_socket = -1;
int new_socket = -1;


///////////////////////////////////////////////////////////////////////////////

void *clientCommunication(void *data, char* mailSpoolDirectory);
void signalHandler(int sig);
void createDir(char* newDir);
char messeagesFind(char buffer[BUF], char* mailSpoolDirectory);
//char openMessage(char buffer[BUF]);
char* receiveMessage(char buffer[BUF], int *current_socket);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
    socklen_t addrlen;
    struct sockaddr_in address, cliaddress;
    int reuseValue = 1;
    int PORT;
    char* mailSpoolDirectory;

    //./twmailer-server <port> <mail-spool-directoryname>
    //z.B.: ./twmailer-server 1234 ../users
    if (argc != 3)
    {
        printf("Usage: %s <port> <mail-spool-directoryname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else
    {
        PORT = atoi(argv[1]);
        mailSpoolDirectory = argv[2];
    }


    ////////////////////////////////////////////////////////////////////////////
    // SIGNAL HANDLER
    // SIGINT (Interrup: ctrl+c)
    // https://man7.org/linux/man-pages/man2/signal.2.html
    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("signal can not be registered");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // CREATE A SOCKET
    // https://man7.org/linux/man-pages/man2/socket.2.html
    // https://man7.org/linux/man-pages/man7/ip.7.html
    // https://man7.org/linux/man-pages/man7/tcp.7.html
    // IPv4, TCP (connection oriented), IP (same as client)
    if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket error"); // errno set by socket()
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // SET SOCKET OPTIONS
    // https://man7.org/linux/man-pages/man2/setsockopt.2.html
    // https://man7.org/linux/man-pages/man7/socket.7.html
    // socket, level, optname, optvalue, optlen
    if (setsockopt(create_socket,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &reuseValue,
                   sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reuseAddr");
        return EXIT_FAILURE;
    }

    if (setsockopt(create_socket,
                   SOL_SOCKET,
                   SO_REUSEPORT,
                   &reuseValue,
                   sizeof(reuseValue)) == -1)
    {
        perror("set socket options - reusePort");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // INIT ADDRESS
    // Attention: network byte order => big endian
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    ////////////////////////////////////////////////////////////////////////////
    // ASSIGN AN ADDRESS WITH PORT TO SOCKET
    if (bind(create_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        perror("bind error");
        return EXIT_FAILURE;
    }

    ////////////////////////////////////////////////////////////////////////////
    // ALLOW CONNECTION ESTABLISHING
    // Socket, Backlog (= count of waiting connections allowed)
    if (listen(create_socket, 5) == -1)
    {
        perror("listen error");
        return EXIT_FAILURE;
    }

    while (!abortRequested)
    {
        /////////////////////////////////////////////////////////////////////////
        // ignore errors here... because only information message
        // https://linux.die.net/man/3/printf
        printf("Waiting for connections...\n");

        /////////////////////////////////////////////////////////////////////////
        // ACCEPTS CONNECTION SETUP
        // blocking, might have an accept-error on ctrl+c
        addrlen = sizeof(struct sockaddr_in);
        if ((new_socket = accept(create_socket,
                                 (struct sockaddr *)&cliaddress,
                                 &addrlen)) == -1)
        {
            if (abortRequested)
            {
                perror("accept error after aborted");
            }
            else
            {
                perror("accept error");
            }
            break;
        }

        /////////////////////////////////////////////////////////////////////////
        // START CLIENT
        // ignore printf error handling
        printf("Client connected from %s:%d...\n",
               inet_ntoa(cliaddress.sin_addr),
               ntohs(cliaddress.sin_port));
        clientCommunication(&new_socket, mailSpoolDirectory); // returnValue can be ignored
        new_socket = -1;
    }

    // frees the descriptor
    if (create_socket != -1)
    {
        if (shutdown(create_socket, SHUT_RDWR) == -1)
        {
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


// =========================================================================================================================


void *clientCommunication(void *data, char* mailSpoolDirectory)
{
    char buffer2[BUF];
    char* buffer = buffer2;
    int size;
    int *current_socket = (int *)data;
    //int check;
    int userFound;
    //int message_counter2 = 0;
    //DIR* dir;
    DIR *directory;
    struct dirent *entry;
    DIR *directory2;
    struct dirent *entry2;
    char senderDir[1000];
    char receiverDir[1000];

    ////////////////////////////////////////////////////////////////////////////
    // SEND welcome message
    strcpy(buffer, "Welcome to myserver!\r\n");
    if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
    {
        perror("send failed");
        return NULL;
    }

    do
    {
        strcpy(buffer, "Please enter your command from the Menu below\r\nSEND\r\nLIST\r\nREAD\r\nDEL\r\nQUIT\r\n");
        if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
        {
            perror("send failed");
            return NULL;
        }

        /////////////////////////////////////////////////////////////////////////
        // RECEIVE
        buffer = receiveMessage(buffer2, current_socket);
        if (buffer == NULL) break;


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

//      if (strcmp (buffer, "QUIT") == 0)
//         input1 = 5;

        switch (input1)

        {
        case 1 :
            ////////////////////////////////////////////////////////////////////////////
            // SEND

            if (send(*current_socket, "Enter your name:", 17, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }
            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;


            // buffer now should contain sender name
            strcpy(senderDir,mailSpoolDirectory);   //../users
            strcat(senderDir,"/");                  //../users/
            strcat(senderDir,buffer);               //../users/nemo
            createDir(senderDir);


            if (send(*current_socket, "Enter the receiver's name:", 27, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;


            // buffer now should contain receiver's name
            strcpy(receiverDir,mailSpoolDirectory);
            strcat(receiverDir,"/");
            strcat(receiverDir,buffer);
            createDir(receiverDir);

            printf("Enter the subject of the message:");


            if (send(*current_socket, "Enter the subject of the message:", 34, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }



            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;



            // buffer now should contain the subject of the message
            FILE *f1Pointer;
            strcat(senderDir,"/");
            strcat(senderDir,buffer);
            strcat(senderDir,".txt");

            f1Pointer = fopen (senderDir, "w");
            FILE *f2Pointer;
            strcat(receiverDir,"/");        //../users/nemo/
            strcat(receiverDir,buffer);     //../users/nemo/blub

            strcat(receiverDir,".txt");     //../users/nemo/blub.txt

            f2Pointer = fopen (receiverDir, "w");




            // Does not work yet, only one line possible. Not terminated by .\n
            if (send(*current_socket, "Enter your message: (End message by typing '.' and then press enter)", 69, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            do
            {

                buffer = receiveMessage(buffer2, current_socket);
                //if (buffer == NULL) break;

                fprintf(f1Pointer,"%s\n",buffer);
                fprintf(f2Pointer,"%s\n",buffer);

            }
            while ((buffer[0] != '.') && (strlen(buffer) != 1));

            fclose (f1Pointer);
            fclose (f2Pointer);

            break;


        case 2 :
            ////////////////////////////////////////////////////////////////////////////
            // LIST

            if (send(*current_socket, "Enter the user's name:", 23, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }


            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;

            directory = opendir(mailSpoolDirectory);

            if (directory == NULL)
            {
                printf("%s\n", "Error opening directory ");
            }
            userFound = 0;

            while ((entry = readdir(directory)) != NULL)
            {
                if (strcmp(buffer,entry->d_name) == 0)
                {
                    userFound = 1;

                    messeagesFind(buffer, mailSpoolDirectory);  // finding the folder of user and print the results

                    if (buffer[0] == 0)
                    {
                        if (send(*current_socket, "0 (no message)", 15, 0) == -1)
                        {
                            perror("send answer failed");
                            //return NULL;
                            break;
                        }
                    }
                    else if (send(*current_socket, buffer, strlen(buffer) - 4, 0) == -1)
                    {
                        perror("send failed");
                        return NULL;
                    }
                }
            }
            if (userFound == 0)
            {
                if (send(*current_socket, "0 (user unknown)", 17, 0) == -1)
                {
                    perror("send answer failed");
                    //return NULL;
                    break;
                }
            }


            if (closedir(directory) == -1)
            {
                printf("%s\n", "Error opening directory ");
            }
            break;

        case 3 :
            ////////////////////////////////////////////////////////////////////////////
            // READ
            if (send(*current_socket, "Enter the user's name:", 23, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;

            directory2 = opendir(mailSpoolDirectory);

            if (directory2 == NULL)
            {
                printf("%s\n", "Error opening directory ");
            }
            userFound = 0;
            while ((entry2 = readdir(directory2)) != NULL)
            {
                if (strcmp(buffer,entry2->d_name) == 0)
                {
                    userFound = 1;
                    char userInputs[100];            // array to save the input from user
                    strcpy (userInputs, mailSpoolDirectory);
                    strcat(userInputs,"/");
                    strcat (userInputs, buffer);

                    //DIR *directory;
                    //struct dirent *entry;
                    directory = opendir(userInputs);

                    if (send(*current_socket, "Enter the subject that you want to read (without '.txt'):", 57, 0) == -1)
                    {
                        perror("send answer failed");
                        return NULL;
                    }
                    buffer = receiveMessage(buffer2, current_socket);
                    if (buffer == NULL) break;

                    char inputFileName[80];
                    strcpy (inputFileName, buffer);
                    strcat (inputFileName, ".txt");
                    strcat (userInputs,"/");
                    strcat (userInputs, inputFileName);

                    //search for the file and print error if not found

                    if (directory == NULL)
                    {
                        printf("%s\n", "Error opening directory ");
                    }

                    int count = 0;
                    while ((entry = readdir(directory)) != NULL)
                    {
                        if (strcmp(entry->d_name, inputFileName) == 0)
                        {
                            count++;
                            FILE *fPointer;
                            fPointer = fopen (userInputs, "r");
                            char singleLine[150];

                            if (fPointer != NULL)
                            {
                                strcpy (buffer,"OK\n");
                                while (!feof (fPointer))
                                {
                                    if (fgets(singleLine, 150, fPointer) == NULL) // eventuell Leerzeile in Datei
                                    {
                                        printf("Failed to read integer.\n");
                                    }
                                    //fgets(singleLine, 150, fPointer);
                                    strcat (buffer, "<< ");
                                    strcat (buffer, singleLine);
                                }
                            }
                            else
                            {
                                printf("Error\n");
                            }

                            fclose (fPointer);
                        }
                    }

                    if (closedir(directory) == -1)
                    {
                        printf("%s\n", "Error closing directory ");
                    }

                    if (count == 1)
                    {
                        if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
                        {
                            perror("send failed");
                            return NULL;
                        }
                    }

                    if (count != 1)
                    {
                        if (send(*current_socket, "ERR", 4, 0) == -1)
                        {
                            perror("send answer failed");
                            return NULL;
                        }
                    }

                }
            }
            if (closedir(directory2) == -1)
            {
                printf("%s\n", "Error closing directory ");
            }
            if (userFound == 0)
            {
                if (send(*current_socket, "ERR", 4, 0) == -1)
                {
                    perror("send answer failed");
                    return NULL;
                }
            }
            break;

        case 4 :
            ////////////////////////////////////////////////////////////////////////////
            // DEL
            if (send(*current_socket, "Enter the user's name:", 23, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }

            buffer = receiveMessage(buffer2, current_socket);
            if (buffer == NULL) break;

            directory2 = opendir(mailSpoolDirectory);

            if (directory2 == NULL)
            {
                printf("%s\n", "Error opening directory ");
            }
            userFound = 0;
            while ((entry2 = readdir(directory2)) != NULL)
            {
                if (strcmp(buffer,entry2->d_name) == 0)
                {
                    userFound = 1;

                    char userInputs[100];            // array to save the input from user
                    strcpy (userInputs, mailSpoolDirectory);
                    strcat (userInputs, "/");
                    strcat (userInputs, buffer);
                    if (send(*current_socket, "Enter the subject that you want to delete (without '.txt'):", 59, 0) == -1)
                    {
                        perror("send answer failed");
                        return NULL;
                    }
                    size = recv(*current_socket, buffer, BUF - 1, 0);
                    if (size == -1)
                    {
                        if (abortRequested)
                        {
                            perror("recv error after aborted");
                        }
                        else
                        {
                            perror("recv error");
                        }
                        break;
                    }
                    if (size == 0)
                    {
                        printf("Client closed remote socket\n"); // ignore error
                        break;
                    }

                    // remove ugly debug message, because of the sent newline of client
                    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
                    {
                        size -= 2;
                    }
                    else if (buffer[size - 1] == '\n')
                    {
                        --size;
                    }

                    buffer[size] = '\0';
                    printf("Message received: %s\n", buffer); // ignore error

                    strcat (userInputs,"/");
                    strcat (userInputs, buffer);
                    strcat (userInputs, ".txt");

                    if(remove(userInputs) == 0)
                    {
                        if (send(*current_socket, "OK", 3, 0) == -1)
                        {
                            perror("send answer failed");
                            return NULL;
                        }
                    }
                    else
                    {
                        if (send(*current_socket, "ERR", 4, 0) == -1)
                        {
                            perror("send answer failed");
                            return NULL;
                        }
                    }

                    if (send(*current_socket, buffer, strlen(buffer), 0) == -1)
                    {
                        perror("send failed");
                        return NULL;
                    }
                }
            }
            if (userFound == 0)
            {
                if (send(*current_socket, "User unknown", 13, 0) == -1)
                {
                    perror("send answer failed");
                    return NULL;
                }
            }

            break;

//      case 5 :
//      ////////////////////////////////////////////////////////////////////////////
//      // QUIT
//
//         break;

        default:
            ////////////////////////////////////////////////////////////////////////////
            // DEFAULT
            if (send(*current_socket, "WRONG INPUT", 12, 0) == -1)
            {
                perror("send answer failed");
                return NULL;
            }
            break;
        }

    }
    while (strcmp(buffer, "QUIT") != 0);   // && !abortRequested);
    /////////////////////////////////////////////////////////////////////////
    // CLOSES THE DESCRIPTOR
    // closes/frees the descriptor if not already
    if (*current_socket != -1)
    {
        if (shutdown(*current_socket, SHUT_RDWR) == -1)
        {
            perror("shutdown new_socket");
        }
        if (close(*current_socket) == -1)
        {
            perror("close new_socket");
        }
        *current_socket = -1;
    }

    return NULL;
}

void signalHandler(int sig)
{
    if (sig == SIGINT)
    {
        printf("abort Requested... "); // ignore error
        abortRequested = 1;
        /////////////////////////////////////////////////////////////////////////
        // With shutdown() one can initiate normal TCP close sequence ignoring
        // the reference count.
        // https://beej.us/guide/bgnet/html/#close-and-shutdownget-outta-my-face
        // https://linux.die.net/man/3/shutdown
        if (new_socket != -1)
        {
            if (shutdown(new_socket, SHUT_RDWR) == -1)
            {
                perror("shutdown new_socket");
            }
            if (close(new_socket) == -1)
            {
                perror("close new_socket");
            }
            new_socket = -1;
        }

        if (create_socket != -1)
        {
            if (shutdown(create_socket, SHUT_RDWR) == -1)
            {
                perror("shutdown create_socket");
            }
            if (close(create_socket) == -1)
            {
                perror("close create_socket");
            }
            create_socket = -1;
        }
    }
    else
    {
        exit(sig);
    }
}

/*The function opens the directory (char* mailSpoolDirectory) where the messages are stored and searches for the folder (name)
that the user has entered in the application. The name of the user folder is stored in char buffer[BUF]. */

char messeagesFind(char buffer[BUF], char* mailSpoolDirectory)
{
    // opening the directory and send the content
    DIR *directory;
    struct dirent *entry;
    char userFolder[1000];            // array to save the path and folder of user
    //int counter = 0;
    strcpy (userFolder, mailSpoolDirectory);
    strcat (userFolder,"/");
    strcat (userFolder, buffer);

    directory = opendir(userFolder);

    if (directory == NULL)
    {
        printf("%s\n", "Error opening directory ");
    }
    strcpy (buffer, "\0");

    while ((entry = readdir(directory)) != NULL)
    {
        //counter++;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {

        }
        else
        {
            strcat(buffer, entry->d_name);
            strcat(buffer, "\n<< ");
        }
    }

    if (closedir(directory) == -1)
    {
        printf("%s\n", "Error opening directory ");
    }
    return *buffer;
}


//returns the buffer (the user's input) without line breaking characters
char* receiveMessage(char buffer[BUF], int *current_socket)
{
    int size;


    size = recv(*current_socket, buffer, BUF - 1, 0);
    if (size == -1)
    {
        if (abortRequested)
        {
            perror("recv error after aborted");
        }
        else
        {
            perror("recv error");
        }
    }
    if (size == 0)
    {
        printf("Client closed remote socket\n"); // ignore error
    }

    // remove ugly debug message, because of the sent newline of client
    if (buffer[size - 2] == '\r' && buffer[size - 1] == '\n')
    {
        size -= 2;
    }
    else if (buffer[size - 1] == '\n')
    {
        --size;
    }

    buffer[size] = '\0';
    printf("Message received: %s\n", buffer); // ignore error

    return buffer;


}
/*The function creates a new directory for a user to store his/her messages
(after checking that the directory for the user does not already exist).*/

void createDir(char* newDir)
{
    int check;
    DIR* dir;


    dir = opendir(newDir);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
    }
    else if (ENOENT == errno)
    {
        /* Directory does not exist. */
        check = mkdir(newDir,0755);                    //0777 = permissions: read, write, & execute for owner, group and others

        // check if directory is created or not
        if (!check)
            printf("Directory created\n");
        else
        {
            printf("Unable to create directory\n");
            exit(1);
        }
    }
    else
    {
        /* opendir() failed for some other reason. */
    }
    return;
}

