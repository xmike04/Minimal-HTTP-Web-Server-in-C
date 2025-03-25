/*
  wbserver.c
 
 Simple C-based HTTP server that listens for GET requests
 and serves requested .html files from the server's local
 directory. Returns a 404 page if the file is not found, and
 shuts down gracefully when exit.html is requested.

 Usage:
   ./wbserver
 (Then user is prompted for "Port: ")

Example:
   ./wbserver
  Port: 8080

  Author: Michael Diaz
  Date:   Apr 24 2025
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <time.h>
 #include <sys/stat.h>
 
 #define BUFFER_SIZE 4096
 
 // Function to get current GMT date in HTTP  friendly format
 void get_gmt_date(char *date_buffer, size_t max_len) {
     time_t rawtime;
     struct tm *timeinfo;
     time(&rawtime);
     timeinfo = gmtime(&rawtime);


     strftime(date_buffer, max_len, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
 }
 
 // Sends the file contents + a built HTTP header for status_line
 void send_http_response(int client_sock, 
                         const char *status_line, 
                         const char *filename, 
                         const char *date_str)
 {
     // Attempt to open the file
     FILE *fp = fopen(filename, "rb");
     if (!fp) {
         // If file can't be opened, return
         return;
     }
 
     // Finds file size
     fseek(fp, 0, SEEK_END);
     long file_size = ftell(fp);
     rewind(fp);
 
     // Build the header string
     char header[512];
     snprintf(header, sizeof(header),
         "%s\r\n"
         "Content-Type: text/html\r\n"
         "Date: %s\r\n"
         "Content-Length: %ld\r\n"
         "\r\n",
         status_line, date_str, file_size
     );
 
     // Send the header
     send(client_sock, header, strlen(header), 0);
 
     // Sendsfile contents
     char file_buffer[BUFFER_SIZE];


     size_t bytes_read;
     while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), fp)) > 0) {
         send(client_sock, file_buffer, bytes_read, 0);
     }
 
     fclose(fp);
 }
 
 int main(void)
 {
     // |1: Prints program name
     printf("./wbserver\n");
 
     char port_str[64];
     memset(port_str, 0, sizeof(port_str));
 
     printf("Port: ");
     fflush(stdout);
 
     if (fgets(port_str, sizeof(port_str), stdin) == NULL) {
         fprintf(stderr, "Error reading port.\n");
         exit(EXIT_FAILURE);
     }
 
     // Converts port string to int
     int port = atoi(port_str);
     if (port <= 0) {
         fprintf(stderr, "Invalid port number.\n");
         exit(EXIT_FAILURE);
     }
 
     // Creates TCP socket
     int server_sock = socket(AF_INET, SOCK_STREAM, 0);
     if (server_sock < 0) {
         perror("socket creation failed");
         exit(EXIT_FAILURE);
     }
 
     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
 
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     server_addr.sin_addr.s_addr = INADDR_ANY;
 
     if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
         perror("bind failed");
         close(server_sock);
         exit(EXIT_FAILURE);
     }
 
     // Listenss for incoming connections
     if (listen(server_sock, 10) < 0) {
         perror("listen failed");
         close(server_sock);
         exit(EXIT_FAILURE);
     }
 
     //  keeps accepting connections until exit.html is requested requestedd
     int should_run = 1;
 
     while (should_run) 
     {
         struct sockaddr_in client_addr;
         socklen_t client_len = sizeof(client_addr);
         int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
         if (client_sock < 0) {
             perror("accept failed");
             close(server_sock);
             exit(EXIT_FAILURE);
         }
 
         // Read the incoming HTTP request
         char request[BUFFER_SIZE];
         memset(request, 0, sizeof(request));
 
         ssize_t bytes_received = recv(client_sock, request, sizeof(request) - 1, 0);
         if (bytes_received > 0) 
         {
             char *method = strtok(request, " ");
             char *path   = strtok(NULL, " ");
 
             // If no path or path is "/", default to index.html
             if (!path || strcmp(path, "/") == 0) {
                 path = "index.html";
             } else {
                 // remove leading '/'
                 if (path[0] == '/') path++;
             }
 
             // printss which page was requested
             printf("Requested Page: %s\n", path);
 
             // Prepare date string
             char date_buffer[128];
             get_gmt_date(date_buffer, sizeof(date_buffer));
 
             // If path is "exit.html", then shut down
             if (strcmp(path, "exit.html") == 0) {
                 // Attempts to open exit.html to see if exists
                 FILE *test_exit = fopen("exit.html", "rb");
                 if (test_exit) {
                     fclose(test_exit);
                     // Serves exit.html
                     send_http_response(client_sock, "HTTP/1.1 200 OK", "exit.html", date_buffer);
                     printf("Status: Found and stopping the web server!\n\n");
                 } else {
                     printf("Status: exit.html not found, but stopping anyway.\n\n");
                 }
                 close(client_sock);
                 should_run = 0;
                 break;
             }
 
             // Attempt to open reqeted file
             FILE *fp = fopen(path, "rb");
             if (fp) {
                 fclose(fp);
                 // 200 OK
                 send_http_response(client_sock, "HTTP/1.1 200 OK", path, date_buffer);
                 printf("Status: Found and serviced\n\n");
             } else {
                 // 404
                 FILE *fp404 = fopen("404.html", "rb");
                 if (fp404) {
                     fclose(fp404);
                     send_http_response(client_sock, "HTTP/1.1 404 Not Found", "404.html", date_buffer);
                     printf("Status: Not found and sent 404.html\n\n");
                 } else {
                     // If 404.html don't exist
                     printf("Status: Not found, 404.html also missing\n\n");
                 }
             }
         }
         close(client_sock);
     }
 
     // Cleanup
     close(server_sock);
     return 0;
 }