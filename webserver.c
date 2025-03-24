/*
 Filename: wbserver.c
 Description: A simple C-based web server that responds to HTTP GET requests
              from a browser. Serves HTML files from the local directory,
              returns a custom 404 page if not found, and shuts down on exit.html.

 Course: CSCE 3530 - Introduction to Computer Networks (Spring 2025)

 Usage:
   ./wbserver
   Port: <port_number>

 Then in browser:
   http://cell06-cse.eng.unt.edu:<port_number>/<filename>.html

 Example outputs (per assignment PDF):
   Requested Page: index.html
   Status: Found and serviced

   Requested Page: exit.html
   Status: Found and stopping the web server! 
   */


 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <time.h>
 #include <sys/stat.h>
 #include <sys/socket.h>
 #include <fcntl.h>
 
 #define BACKLOG 10         // How many pending connections queue will hold
 #define REQ_BUFFER_SIZE 2048
 #define RES_BUFFER_SIZE 8192
 
 // Helper function to get current GMT time in HTTP format
 void get_http_date(char* date_str, size_t max_len) {
     time_t rawtime;
     struct tm* timeinfo;
     time(&rawtime);
     // Convert to GMT/UTC
     timeinfo = gmtime(&rawtime);
     // Format: e.g. "Wed, 05 Mar 2025 22:04:41 GMT"
     strftime(date_str, max_len, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
 }
 
 // Helper function: read file content into a buffer, return file size
 // Return -1 if file not found or unreadable.
 ssize_t read_file_into_buffer(const char* filename, char* buffer, size_t max_len) {
     FILE* fp = fopen(filename, "rb");
     if (!fp) {
         return -1; // not found
     }
 
     // fseek to find file size
     fseek(fp, 0, SEEK_END);
     long fsize = ftell(fp);
     rewind(fp);
 
     if (fsize > (long)max_len) {
         fclose(fp);
         return -1; // Too large for our buffer
     }
 
     size_t read_bytes = fread(buffer, 1, fsize, fp);
     fclose(fp);
     return (ssize_t)read_bytes;
 }
 
 // Build and send an HTTP response to the client
 void send_http_response(int client_sock, const char* header_status, 
                         const char* filename, int is_found) {
     char response_buffer[RES_BUFFER_SIZE];
     char file_buffer[RES_BUFFER_SIZE];
     memset(response_buffer, 0, sizeof(response_buffer));
     memset(file_buffer, 0, sizeof(file_buffer));
 
     // If we are serving the file
     ssize_t file_size = 0;
     if (is_found) {
         // Read the file content
         file_size = read_file_into_buffer(filename, file_buffer, RES_BUFFER_SIZE - 1);
         if (file_size < 0) {
             // fallback: treat as not found
             is_found = 0;
         }
     }
 
     // If not found, read from 404.html
     if (!is_found) {
         file_size = read_file_into_buffer("404.html", file_buffer, RES_BUFFER_SIZE - 1);
         // Safety check (though the assignment ensures 404.html is available)
         if (file_size < 0) {
             file_size = 0;
         }
     }
 
     // Prepare the date header
     char date_str[128];
     get_http_date(date_str, sizeof(date_str));
 
     // Status line
     // If file is found => 200 OK, else => 404 Not Found
     char status_line[64];
     if (is_found) {
         snprintf(status_line, sizeof(status_line), "HTTP/1.1 200 OK");
     } else {
         snprintf(status_line, sizeof(status_line), "HTTP/1.1 404 Not Found");
     }
 
     // Build the HTTP response header
     // Content-Type is always text/html as per assignment
     // We also add Date, Content-Length
     // Then a blank line, then the file data
     int header_len = snprintf(response_buffer, sizeof(response_buffer),
         "%s\r\n"
         "Content-Type: text/html\r\n"
         "Date: %s\r\n"
         "Content-Length: %ld\r\n"
         "\r\n",  // blank line before the file content
         status_line, date_str, (long)file_size
     );
 
     // Copy the file content after the header
     memcpy(response_buffer + header_len, file_buffer, file_size);
     // total size of our final response
     ssize_t total_len = header_len + file_size;
 
     // Send the response to the client
     write(client_sock, response_buffer, total_len);
 }
 
 int main() {
     // The assignment says:
     // ./wbserver
     // Port: <port_number>
     // We'll prompt the user for the port in the same style.
 
     char port_str[16];
     printf("./wbserver\n");
     printf("Port: ");
     fflush(stdout);
     fgets(port_str, sizeof(port_str), stdin);
     // Remove trailing newline
     port_str[strcspn(port_str, "\n")] = '\0';
     if (strlen(port_str) == 0) {
         fprintf(stderr, "No port provided.\n");
         return 1;
     }
 
     int port = atoi(port_str);
     if (port <= 0) {
         fprintf(stderr, "Invalid port: %s\n", port_str);
         return 1;
     }
 
     // Create a TCP socket
     int server_sock = socket(AF_INET, SOCK_STREAM, 0);
     if (server_sock < 0) {
         perror("socket() failed");
         return 1;
     }
 
     // Allow reuse of address
     int optval = 1;
     setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
 
     // Bind to the port
     struct sockaddr_in server_addr;
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET; 
     server_addr.sin_addr.s_addr = INADDR_ANY; 
     server_addr.sin_port = htons(port);
 
     if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
         perror("bind() failed");
         close(server_sock);
         return 1;
     }
 
     // Listen
     if (listen(server_sock, BACKLOG) < 0) {
         perror("listen() failed");
         close(server_sock);
         return 1;
     }
 
     // We must keep accepting connections and serving requests
     // until user requests exit.html
     int keep_running = 1; // to track if we should keep server running
 
     while (keep_running) {
         // Accept a new connection
         struct sockaddr_in client_addr;
         socklen_t client_len = sizeof(client_addr);
 
         int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
         if (client_sock < 0) {
             perror("accept() failed");
             continue; // try next
         }
 
         // Read the request
         char request_buffer[REQ_BUFFER_SIZE];
         memset(request_buffer, 0, sizeof(request_buffer));
         ssize_t bytes_received = recv(client_sock, request_buffer, sizeof(request_buffer) - 1, 0);
 
         if (bytes_received <= 0) {
             // No data or error
             close(client_sock);
             continue;
         }
 
         // We only handle GET method from the assignment instructions
         // The line looks like: "GET /index.html HTTP/1.1"
         // We'll parse out the requested filename
         char method[8], path[256], version[16];
         memset(method, 0, sizeof(method));
         memset(path, 0, sizeof(path));
         memset(version, 0, sizeof(version));
 
         // e.g. parse with sscanf
         // We'll parse only the first line of the request
         sscanf(request_buffer, "%s %s %s", method, path, version);
 
         // The path will have a leading '/'
         // We'll remove that slash
         char filename[256];
         if (path[0] == '/') {
             strncpy(filename, path + 1, sizeof(filename) - 1);
         } else {
             strncpy(filename, path, sizeof(filename) - 1);
         }
 
         // If the user typed something like: GET / HTTP/1.1
         // Then filename is "" => means index.html by default
         if (strlen(filename) == 0) {
             strncpy(filename, "index.html", sizeof(filename) - 1);
         }
 
         // Print out the server console logs as the assignment wants
         printf("\nRequested Page: %s\n", filename);
 
         // Check if it's exit.html => we shutdown after responding
         if (strcmp(filename, "exit.html") == 0) {
             // Serve exit.html if it exists, else it's not found
             // Then break
             // We'll do the code below
         }
 
         // Check if file actually exists
         int found = 0;
         struct stat st;
         if (stat(filename, &st) == 0) {
             // The file is found
             found = 1;
         }
 
         // If file is exit.html => once we serve it, we stop the web server
         if (strcmp(filename, "exit.html") == 0 && found) {
             // Print the console log
             printf("Status: Found and stopping the web server!\n");
             // Serve exit.html
             send_http_response(client_sock, "HTTP/1.1 200 OK", filename, 1);
             close(client_sock);
 
             keep_running = 0;
         } else if (found) {
             printf("Status: Found and serviced\n");
             // Serve the file
             send_http_response(client_sock, "HTTP/1.1 200 OK", filename, 1);
             close(client_sock);
         } else {
             // not found
             // If the user requested test.html but we don't have it => 404
             printf("Status: Not found and sent 404.html\n");
             send_http_response(client_sock, "HTTP/1.1 404 Not Found", filename, 0);
             close(client_sock);
         }
     }
 
     // Once we break out of the while, we close the server
     close(server_sock);
     return 0;
 }