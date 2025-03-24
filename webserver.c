#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 4096

/* Helper function to get the size of a file */
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1; // File doesn't exist or can't be accessed
}

/* Helper function to get current time in HTTP GMT format */
void get_gmt_time(char *time_buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    // Example format: Wed, 05 Mar 2025 22:04:41 GMT
    strftime(time_buffer, buffer_size, "%a, %d %b %Y %H:%M:%S GMT", gmt);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char port_str[16];

    /* STEP 1: Prompt for port number (mimics assignment style) */
    printf("Port: ");
    fflush(stdout);
    if (fgets(port_str, sizeof(port_str), stdin) == NULL) {
        fprintf(stderr, "Error reading port.\n");
        exit(EXIT_FAILURE);
    }
    int port = atoi(port_str);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number.\n");
        exit(EXIT_FAILURE);
    }

    /* STEP 2: Create TCP socket */
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Allow reuse of port if the server was recently closed */
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    server_addr.sin_port = htons(port);

    /* STEP 3: Bind the socket to the specified port */
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    /* STEP 4: Listen for incoming connections */
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    /* Print a message indicating the server is running */
    // (Optional: More user-friendly or for recruiters to see a nice message)
    printf("\nWeb server started, listening on port %d\n", port);

    /* STEP 5: Accept and handle requests in a loop */
    while (1) {
        char request[BUFFER_SIZE];
        char response[BUFFER_SIZE];
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));

        client_addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        /* Read the request (blocking call) */
        int bytes_received = recv(client_sock, request, sizeof(request) - 1, 0);
        if (bytes_received < 0) {
            perror("recv");
            close(client_sock);
            continue; // handle next connection
        }
        request[bytes_received] = '\0'; // null-terminate

        /* STEP 6: Parse out the requested page name from the GET request */
        // Example: "GET /index.html HTTP/1.1"
        char *method = strtok(request, " ");   // "GET"
        char *filename = strtok(NULL, " ");    // "/index.html"
        if (!method || !filename) {
            close(client_sock);
            continue;
        }

        // skip leading '/'
        if (filename[0] == '/')
            filename++;

        /* Print the requested page info */
        printf("\nRequested Page: %s\n", filename);

        /* If user requested exit.html, shutdown server after sending exit page */
        if (strcmp(filename, "exit.html") == 0) {
            // Attempt to open exit.html
            FILE *exit_file = fopen("exit.html", "r");
            if (!exit_file) {
                // If no exit file, just do a simple shutdown message
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n\r\n"
                         "<html><h1>The web server is shutting down...</h1></html>");
                send(client_sock, response, strlen(response), 0);
            } else {
                // Read file into buffer
                long size = get_file_size("exit.html");
                if (size < 0) size = 0;
                char date_str[128];
                get_gmt_time(date_str, sizeof(date_str));

                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Date: %s\r\n"
                         "Content-Length: %ld\r\n\r\n",
                         date_str, size);
                send(client_sock, response, strlen(response), 0);

                // Then send the file
                char file_buffer[BUFFER_SIZE];
                size_t nread;
                while ((nread = fread(file_buffer, 1, sizeof(file_buffer), exit_file)) > 0) {
                    send(client_sock, file_buffer, nread, 0);
                }
                fclose(exit_file);
            }

            printf("Status: Found and stopping the web server!\n");
            close(client_sock);
            close(server_sock);
            exit(0);
        }

        /* STEP 7: Check if file exists, otherwise 404 */
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            // 404 Not Found
            FILE *fp404 = fopen("404.html", "r");
            if (!fp404) {
                // If there's no 404.html, fallback to minimal text
                snprintf(response, sizeof(response),
                         "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/html\r\n\r\n"
                         "<html><h1>404 Requesting page not found.</h1></html>");
                send(client_sock, response, strlen(response), 0);
            } else {
                long size404 = get_file_size("404.html");
                if (size404 < 0) size404 = 0;

                char date_str[128];
                get_gmt_time(date_str, sizeof(date_str));

                snprintf(response, sizeof(response),
                         "HTTP/1.1 404 Not Found\r\n"
                         "Content-Type: text/html\r\n"
                         "Date: %s\r\n"
                         "Content-Length: %ld\r\n\r\n",
                         date_str, size404);
                send(client_sock, response, strlen(response), 0);

                // send 404.html content
                char file_buffer[BUFFER_SIZE];
                size_t nread;
                while ((nread = fread(file_buffer, 1, sizeof(file_buffer), fp404)) > 0) {
                    send(client_sock, file_buffer, nread, 0);
                }
                fclose(fp404);
            }
            printf("Status: Not found and sent 404.html\n");
        } else {
            // 200 OK
            long fsize = get_file_size(filename);
            if (fsize < 0) fsize = 0;

            char date_str[128];
            get_gmt_time(date_str, sizeof(date_str));

            snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/html\r\n"
                     "Date: %s\r\n"
                     "Content-Length: %ld\r\n\r\n",
                     date_str, fsize);
            send(client_sock, response, strlen(response), 0);

            // Send file contents
            char file_buffer[BUFFER_SIZE];
            size_t nread;
            while ((nread = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0) {
                send(client_sock, file_buffer, nread, 0);
            }
            fclose(fp);

            printf("Status: Found and serviced\n");
        }

        // Done with this request
        close(client_sock);
    }

    // Should never get here in normal flow
    close(server_sock);
    return 0;
}