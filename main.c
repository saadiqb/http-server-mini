/*
HTTP:
 - tcp/ip sockets
 - servers listen on -> port 80
 - transmit resources, information identifiable by a URL
 - after response delivered, server closes connection  

format for Req and Res messages:
  <init line>
  Header1: value1  (ends with CRLF)
  Header2: value2
  <optional message body data>

init Req line:
    GET /path/to/file/index.html HTTP/1.0

init Res line:
    HTTP/1.0 200 OK

header lines:
    header keywords provided: values settable

message body:
    response: resource is returned
    request: data or files sent

example:
    www.me.com/index.html
    1. open a socket to me.com port 80. Send a GET request
    2. server responds through same socket
    3. after response, server closes socket 

POST:
    path replaced with a resource handling path


Sockets on server side:
    1. The steps involved in establishing a socket on the server side are as follows:
    2. Create a socket with the socket() system call
    3. Bind the socket to an address using the bind() system call. For a server socket on the Internet, an address consists of a port number on the host machine.
    4. Listen for connections with the listen() system call
    5. Accept a connection with the accept() system call. This call typically blocks until a client connects with the server.
    6. Send and receive data

Sockets address domain:
    unix domain, shared a common FS : a character string 
    internet domain: IP address : Port

Sockets types:
    stream sockets   -> continuous stream of characters -> TCP
    datagram sockets -> read entire messages at once    -> UDP

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_LENGTH 2056      //256
#define HTTP_VERSION "HTTP/1.1"

char *httpOp;
char *pathX;
char *httpV;

void deleteFirstCharacter(char *str) {
    if (str == NULL || str[0] == '\0') {
        // Handle empty string or null pointer
        return;
    }

    // Shift all characters one position to the left
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = str[i + 1];
    }
}


void error(char *msg)
{
    perror(msg);
    exit(1);
}

void returnNotFound(int sockfd)
{
    char * notFound =  "HTTP/1.1 404 Not Found\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 0\r\n"
             "Connection: close\r\n"
             "\r\n";

    write(sockfd, notFound, strlen(notFound));
}


void doGET(char * filePath, int sockfd)
{
    printf("GET received\n");

    // Define a buffer size for the response header
    const int response_header_size = 256;
    char response_header[response_header_size];

    // Allocate enough space for the HTML body
    char *htmlBody = malloc(10000 * sizeof(char));
    if (htmlBody == NULL) {
        perror("malloc failed for htmlBody");
        return;
    }
    htmlBody[0] = '\0';  // Initialize the buffer to be empty

    // Assuming pathX is defined and points to the file path
    char *filepath = filePath;
    deleteFirstCharacter(filepath);

    printf("File path is: %s\n", filepath);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        returnNotFound(sockfd);
        error("couldn't read/open file path");
        free(htmlBody);
        return;
    }

    char line[1000];
    while (fgets(line, sizeof(line), file))
    {
        strcat(htmlBody, line);
    }

    fclose(file);

    int bodyLen = strlen(htmlBody);
    //printf("Body is: %s", htmlBody);

    // Format the response header with the body length
    snprintf(response_header, response_header_size,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %d\r\n"
             "\r\n",
             bodyLen);

    // Send the response header and body
    write(sockfd, response_header, strlen(response_header));
    write(sockfd, htmlBody, bodyLen);

    // Clean up allocated memory
    free(htmlBody);
}

void doPOST(int sockfd)
{
    printf("POST recieved\n");

}

void InitiateHTTP(int _fd)
{
    //char path[256];
    int n;
    char buffer[BUFFER_LENGTH];
    memset(buffer, 0, BUFFER_LENGTH);
    
    n = recv(_fd, buffer, BUFFER_LENGTH - 1, 0);
    if (n < 0) error("err -> reading from socket\n");
    //printf("dump: %s\n", buffer);

    char line[BUFFER_LENGTH];
    
    // get 1st line
    //fgets(line, sizeof(line), buffer);
    sscanf(buffer, "%[^\n]", line);
    //size_t len = strlen(line);
    //printf("extracted line ->  %s\n", line);

    char* token;
    
    // 1st parameter
    token = strtok(line, " \t");
    if (token == NULL)
    {
        error("no operation\n");
    }
    httpOp = token;

    // 2nd parameter
    token = strtok(NULL, " \t");
    if (token == NULL)
    {
        error("err path empty\n");
    }
    //printf("parse path ->  %s\n", token);
    pathX = token;

    // 3rd parameter
    token = strtok(NULL, " \t");
    if (token == NULL)
    {
        error("http version empty\n");
    }
    if (!strcmp(token, HTTP_VERSION))
    {
        error("http version not correct");
    }
    //printf("http version ->  %s\n", token);
    httpV = token;

    // Operation
    if (strcmp(httpOp, "GET") == 0)
    {
        printf("x->%s\n", pathX);
        if (strcmp(pathX,"/index.html") == 0)
        {
            doGET(pathX, _fd);
        }
        else
        {
            char * def  = "/404.html"; 
            doGET(def, _fd);
        }
        
        
        // if (strcmp(pathX, "/") == 0 || strcmp(pathX, "/404.html") == 0)
        // {
        //     doGET("/404.html", _fd);
        // }
        // else if (strcmp(pathX, "index.html") == 0)
        // {
        //     doGET("/index.html", _fd);
        // }
        // else
        // {
        //     doGET("/404.html", _fd);
        // }
        doGET(pathX, _fd);
    }
    else if (strcmp(httpOp, "POST") == 0)
    {
        doPOST(_fd);
    }
    else
    {
        error("err parsing operation\n");
    }

    close(_fd);  
}

int main(int argc, char *argv[])
{
    if (argc < 2) error("ERROR, no port given\n");
    printf("Listening on PORT: %d\n", atoi(argv[1]));

    int sockfd, clientfd, portno, clilen, pid;
    struct sockaddr_in serv_addr, cli_addr;

    // creates a new socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("err -> open socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));  
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    // port number in host byte order to network byte order
    serv_addr.sin_port = htons(portno);
    // set ip address of server : constant
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket -> address
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("err -> binding");
    
    // listen on socket for connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        // accept connection
        clientfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
        if (clientfd < 0) error("err -> accept");
        pid = fork();
        if (pid < 0) error("err -> fork");
        if (pid == 0)
        {
            close(sockfd);
            InitiateHTTP(clientfd);
            exit(0);
        }
        else
        {
            close(clientfd);
        }
    }
    
    return 0;
}