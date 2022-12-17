#include "http.h"

//Response and request size
const int MAX_RESPONSE_REQUEST_SIZE = 1 << 16;

int sendHTTP(int fd, char* msg) {
    return write(fd, msg, strlen(msg));
}

//Get size of file for dynamic allocation
size_t size_of_file(FILE* file) {
  if (file == NULL)
    return -1;
  
  fseek(file, 0, SEEK_END);
  size_t sz = ftell(file);
  fseek(file, 0, SEEK_SET);

  return sz + 1;
}

//Read HTML file for the HTTP header response
char* read_file(char* path) {
    FILE* file = fopen(path, "r");
    char* webPage;
    size_t n = 0;
    int c;

    //Return NULL if not found
    if (file == NULL)
        return NULL;

    size_t sizeFile = size_of_file(file);

    webPage = (char*)malloc(sizeFile * sizeof(char));

    //Alloc chars from file
    while ((c = fgetc(file)) != EOF) {
        webPage[n++] = (char)c;
    }

    //Null byte terminate
    webPage[n] = '\0'; 

    //Remember to deallocate FILE buffer
    fclose(file);
    return webPage;
}

//Initialize the socket listener
int init_listener_socket(uint16_t port) {
    //Create the LISTENING socket
    int listen_fd, opt = 1;
    struct sockaddr_in server_adr;

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "[-] Error creating listening socket.\n");
        exit(EXIT_FAILURE);
    }
    
    //Initialize all to 0 and then fill in attributes
    memset(&server_adr, 0, sizeof(server_adr));
    server_adr.sin_family      = AF_INET;
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("127.0.0.1");
    server_adr.sin_port        = htons(port);
    
    //Set socket options to avoid the bind() errors of 'ports already in use'
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) {
      perror("setsockopt");
      close(listen_fd);
      exit(EXIT_FAILURE);
    }

    //Bind the socket address
    if (bind(listen_fd, (struct sockaddr*) &server_adr, sizeof(server_adr)) < 0 ) {
        fprintf(stderr, "[-] Error calling bind()\n");
        exit(EXIT_FAILURE);
    }

    //Initialize the listener
    if (listen(listen_fd, 10)) {
        fprintf(stderr, "[-] Error being listen()\n");
        exit(EXIT_FAILURE);    
    }

    return listen_fd;
}

//Send the response with the appropriate HTTP headers
int send_response(int fd, char *header, char *content_type, char *body)
{
  char response[MAX_RESPONSE_REQUEST_SIZE];
  int response_length; // Total length of header plus body
  
  int content_length;
  content_length = strlen(body);
  sprintf(response, 
    "%s\n"
    "Content-Length: %d\n"
    "Content-Type: %s\n"
    "Connection: close\n"
    "\n"
    "%s",
  header, content_length, content_type, body);
  response_length = strlen(response);

  //Send HTTP response
  int rv = send(fd, response, response_length, 0);

  if (rv < 0) {
    perror("send");
  }

  return rv;
}

//Obtain the IP (v4/v6) address
void* get_in_addr(struct sockaddr* sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Handle the requests
int handle_http_request(int fd)
{
  //const int REQUEST_BUFFER_SIZE = 1 << 16; // 64K
  char request[MAX_RESPONSE_REQUEST_SIZE];
  char *p;
  char request_type[8]; //GET or POST
  char request_path[1024]; // /info etc.
  char request_protocol[128]; // HTTP/1.1

  //Read request
  int bytes_recvd = recv(fd, request, MAX_RESPONSE_REQUEST_SIZE - 1, 0);

  printf("%s\n", request);
  if (bytes_recvd < 0) {
    perror("recv");
    return 1;
  }

   //NULL terminate request string
  request[bytes_recvd] = '\0';

  //Get the request type and path from the first line
  sscanf(request, "%s %s %s", request_type, request_path, request_protocol);

  //Read the file
  char* read = read_file("web_page/index.html");

  //Send the response
  send_response(fd, "HTTP/1.1 200 OK", "text/html", read);

  //Free buffer
  free(read);

  return 0;
}

//Handle child processes
void sigchld_handler(int s) {
  (void)s; // quiet unused variable warning

  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  // Wait for all children that have died, discard the exit status
  while(waitpid(-1, NULL, WNOHANG) > 0);

  errno = saved_errno;
}

void start_reaper(void)
{
  struct sigaction sa;

  sa.sa_handler = sigchld_handler; // Reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; // Restart signal handler if interrupted
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
}