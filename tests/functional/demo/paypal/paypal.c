/*
 * under construction
 * this sample shouild be used with "zerovm with sockets"
 */
#define ZVM_SOCKETS

#include "include/zvmlib.h"
#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#define STDOUT_NO 1
#define BUFSIZE 0x10000
#define REQUEST "GET /index.html HTTP/1.1\r\nHost: google.com:80\r\n\r\n"

#define ID printf("#%d\n", __LINE__)
#define FAILIF(c, r, ...) if(c) { printf(__VA_ARGS__); exit(r); }

int main(int argc , char **argv)
{
  char buffer[BUFSIZE];
  struct sockaddr_in addr = {0};
  int result;
  int s;

  /* create socket */
  s = zvm_socket(PF_INET, SOCK_STREAM, 0);
  FAILIF(s < 0, 1, "socket: %d\n", s);

  /* populate a sockaddr_in structure with the remote IP and port */
  addr.sin_family = AF_INET;
  addr.sin_port = 0x5000; // 80 in network order. htons(PORT)
  addr.sin_addr.s_addr = 0x7146C2AD; // 173.194.70.113. htonl(INADDR_ANY)

  /* connect the socket to the far end */
  result = zvm_connect(s, (void*)&addr, sizeof addr);
  FAILIF(result < 0, 2, "connect: %d\n", result);

  /* send the request string */
  result = zvm_send(s, REQUEST, sizeof REQUEST, 0);
  FAILIF(result < sizeof REQUEST, 3, "write: %d\n", result);

  /* read the data */
  while((result = zvm_recv(s, buffer, BUFSIZE, 0)) > 0)
  {
    FAILIF(result < 0, 4, "read: %d\n", result);
    result = zvm_pwrite(STDOUT_NO, buffer, result, 0);
    FAILIF(result < 0, 5, "write: %d\n", result);
  }

  /* close the socket */
  result = zvm_close(s);
  FAILIF(result < 0, 6, "close: %d\n", result);
  return(0);
}

