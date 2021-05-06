#include <iostream>
#include <cstring>

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#ifdef __cplusplus
}
#endif

constexpr const char *SERVER_SOCKET = "/tmp/test.sock";
constexpr int BUFFER_SIZE = 256;
constexpr int SERVPORT = 8080;

void handleError(int fh, const std::string &msg);

int main(int argc, char **argv)
{
  bool inet = false;
  if (argc > 1 && std::string(argv[1]) == "inet")
  {
    printf("Inet\n");
    inet = true;
  }
  int serverHandle = 0, acceptSocket = 0;
  struct sockaddr_un connectAddrSock;
  struct sockaddr_un tmpAddr;
  struct sockaddr_in connectAddrInet;
  struct sockaddr_in peerAddrInet;

  memset(&connectAddrInet, 0, sizeof(struct sockaddr_in));
  connectAddrInet.sin_family = AF_INET;
  connectAddrInet.sin_port = htons(SERVPORT);          //PORT);
  connectAddrInet.sin_addr.s_addr = htonl(INADDR_ANY); // htonl(INADDR_LOOPBACK); //htonl(INADDR_ANY);

  memset(&connectAddrSock, 0, sizeof(struct sockaddr_un));
  connectAddrSock.sun_family = AF_UNIX;
  strncpy(connectAddrSock.sun_path, SERVER_SOCKET, sizeof(connectAddrSock.sun_path) - 1);

  //===================================================================
  // Socket request serverHandle
  //===================================================================
  if (inet)
  {
    serverHandle = socket(PF_INET, SOCK_STREAM, 0);
  }
  else
  {
    serverHandle = socket(AF_UNIX, SOCK_STREAM, 0);
  }

  if (serverHandle < 0)
  {
    std::cerr << "BSDUnix Endpoint socket() failed\n";
    exit(1);
  }

  //===================================================================
  // Connect to SERVER_SOCKET
  //===================================================================
  if (inet)
  {
    printf("unix socket %s known - 0.0.0.0:%d %d\n", "Inet", SERVPORT, serverHandle);
    socklen_t connectAddrSize = sizeof(struct sockaddr_in);
    int cErr = connect(serverHandle, (struct sockaddr *)&connectAddrInet, connectAddrSize);
    if (cErr < 0)
      handleError(serverHandle, "connect() no server could be reason " + std::to_string(cErr));

  }
  else
  {
    printf("unix socket %s known - %d\n", SERVER_SOCKET, serverHandle);
    socklen_t connectAddrSize = sizeof(struct sockaddr_un);
    int cErr = connect(serverHandle, (struct sockaddr *)&connectAddrSock, connectAddrSize);
    if (cErr < 0)
      handleError(serverHandle, "connect() no server could be reason " + std::to_string(cErr));
  }

  // Send the message - END as in to stop server
  std::string message("werty");
  char buffer[BUFFER_SIZE];
  int ret;
  strcpy(buffer, message.c_str());
  printf("sending %s %d\t", buffer, static_cast<int>(message.size()));
  ret = send(serverHandle, buffer, (int)message.size(), (int)0);
  //ret = send(serverHandle, buffer, strlen(buffer) + 1, 0);
  if (ret < 0)
    handleError(serverHandle, "send()");

  /* Receive result. */
  ret = recv(serverHandle, buffer, BUFFER_SIZE, 0);
  if (ret == 0) // Graceful close
  {
    printf("It is a graceful close!\n");
    close(serverHandle);
    return 0;
  }
  else if (ret < 0)
  {
    handleError(serverHandle, "recv()");
  }

  /* Ensure buffer is 0-terminated. */
  buffer[ret] = '\0';
  //buffer[BUFFER_SIZE - 1] = 0;
  printf("Connection recv() is OK. Received %d bytes: [%s]\n", ret, buffer);

  //===================================================================
  // Close connection to SERVER_SOCKET
  //===================================================================

  printf("Close connection to server %d!\n", serverHandle);
  close(serverHandle);

  return 0;
}

//===================================================================
// Global functions
//===================================================================

void handleError(int fh, const std::string &msg)
{
  close(fh);
  std::cerr << "BSDUnix Connect error: " << msg << "\n";
  exit(1);
}
