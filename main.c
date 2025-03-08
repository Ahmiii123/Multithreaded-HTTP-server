                                                                                                                                                                                   server.c
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAX_BUFFER 1024

// Function to log the request details
void log_request(const char* client_ip, const char* path, const char* status_code) {
    FILE* log_file = fopen("server_log.txt", "a");
    if (log_file) {
        time_t current_time;
        struct tm* time_info;
        char time_buffer[80];

        time(&current_time);
        time_info = localtime(&current_time);
        strftime(time_buffer, sizeof(time_buffer), "%a %b %d %H:%M:%S %Y", time_info);

        // Get last modified time of the requested file
        struct stat file_stat;
        char modified_time[80] = "N/A";
        if (stat(path + 1, &file_stat) == 0) {
            strftime(modified_time, sizeof(modified_time), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_mtime));
        }

        fprintf(log_file, "%s/%s %s Last modified at: %s %s\n",
            client_ip,
            time_buffer,
            modified_time,
            path,
            status_code);

        fclose(log_file);
    }
}

// Function to handle client requests
void* handle_request(void* newSocketPtr) {
    int newSocket = *((int*)newSocketPtr);
    free(newSocketPtr); 

    char buffer[MAX_BUFFER];
    int bytes_read;
    char* response;
    char method[10], path[100];
    int keep_alive = 1;

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    getpeername(newSocket, (struct sockaddr*)&client, &client_len);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);

   
    while (keep_alive) {
        
        memset(buffer, 0, sizeof(buffer));

        // Read the HTTP request from the client
        bytes_read = read(newSocket, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            printf("Error reading request\n");
            break;
        }

        buffer[bytes_read] = '\0';  

        sscanf(buffer, "%s %s", method, path);

        // Log the request
        printf("Received %s request for %s\n", method, path); 
        if (strcmp(method, "GET") == 0) {
            FILE* file = fopen(path + 1, "r");  
            if (file) {
                char file_content[MAX_BUFFER];
                size_t read_size;
                read_size = fread(file_content, 1, sizeof(file_content) - 1, file);
                fclose(file);
                file_content[read_size] = '\0';

                response = malloc(200 + strlen(file_content));
                sprintf(response, "HTTP/1.0 200 OK\nContent-Type: text/plain\n\n%s", file_content);

               
                log_request(client_ip, path, "200 OK");
            }
            else {
                response = "HTTP/1.0 404 Not Found\nContent-Type: text/plain\n\nFile Not Found";

               
                log_request(client_ip, path, "404 Not Found");
            }
        }
        // Handle POST request
        else if (strcmp(method, "POST") == 0) {
            
            char* data = strstr(buffer, "\r\n\r\n");
            if (data) {
                data += 4;  
                FILE* file = fopen(path + 1, "w");  
                if (file) {
                    fprintf(file, "%s", data);  
                    fclose(file);
                    response = "HTTP/1.0 200 OK\nContent-Type: text/plain\n\nData Received";

                    // Log the request
                    log_request(client_ip, path, "200 OK");
                }
                else {
                    response = "HTTP/1.0 500 Internal Server Error\nContent-Type: text/plain\n\nUnable to write file";

                    
                    log_request(client_ip, path, "500 Internal Server Error");
                }
            }
        }
        // Handle DELETE request
        else if (strcmp(method, "DELETE") == 0) {
            if (remove(path + 1) == 0) {
                response = "HTTP/1.0 200 OK\nContent-Type: text/plain\n\nFile Deleted";

                
                log_request(client_ip, path, "200 OK");
            }
            else {
                response = "HTTP/1.0 404 Not Found\nContent-Type: text/plain\n\nFile Not Found";

                
                log_request(client_ip, path, "404 Not Found");
            }
        }
        // Handle unsupported methods
        else {
            response = "HTTP/1.0 405 Method Not Allowed\nContent-Type: text/plain\n\nMethod Not Allowed";

           
            log_request(client_ip, path, "405 Method Not Allowed");
        }

       
        write(newSocket, response, strlen(response));

        
        if (strstr(buffer, "Connection: keep-alive") == NULL) {
            keep_alive = 0; 
        }
    }

  
    close(newSocket);
    return NULL;
}

int main(int argc, char* argv[]) {
    int serverSocket, * newSocket;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);
    int portNum;

    
    if (argc < 2) {
        printf("Error! Input format error!\n");
        printf("Input format: ./server 2545\n");
        exit(-1);
    }

    portNum = atoi(argv[1]);

    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        printf("Error creating socket!\n");
        exit(-1);
    }

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(portNum);

    // Bind the socket to the address and port
    if (bind(serverSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("Error binding socket!\n");
        close(serverSocket);
        exit(-1);
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 10) < 0) {
        printf("Error listening on socket!\n");
        close(serverSocket);
        exit(-1);
    }

    printf("Server is listening on port %d...\n", portNum);

    
    while (1) {
        // Accept a new client connection
        newSocket = malloc(sizeof(int)); 
        *newSocket = accept(serverSocket, (struct sockaddr*)&client, &client_len);
        if (*newSocket < 0) {
            printf("Error accepting connection!\n");
            free(newSocket);
            continue;
        }

        printf("Connection established with client: %s\n", inet_ntoa(client.sin_addr));

        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_request, (void*)newSocket) != 0) {
            printf("Error creating thread\n");
            free(newSocket);
        }

        
        pthread_detach(thread_id);
    }

    close(serverSocket);
    return 0;
}
