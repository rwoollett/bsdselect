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
constexpr int LISTEN_BACKLOG = 5;
constexpr int DATA_BUF_SIZE = 256;
constexpr int SERVPORT = 8080;

int acceptClientConnection(int serverHandle, bool inet); // Returns handle to connected peer
void handleClient(int peerHandle, int serverHandle, bool inet);

void handleError(int fh, const std::string &msg);
void handleErrorWithPeer(int fh, int ph, const std::string &msg);
void handleErrorWithUnlink(int fh, const std::string &msg);
void handleErrorWithPeerAndUnlink(int fh, int ph, const std::string &msg);
int setReuseAddr(int serverHandle, int val);
int setKeepAlive(int serverHandle, int val);

int main(int argc, char **argv)
{
  bool inet = false;
  if (argc > 1 && std::string(argv[1]) == "inet")
  {
    printf("Inet\n");
    inet = true;
  }

  int serverHandle = 0, acceptSocket = 0;
  struct sockaddr_un serverAddrSock;
  struct sockaddr_in serverAddrInet;

  memset(&serverAddrInet, 0, sizeof(struct sockaddr_in));
  serverAddrInet.sin_family = AF_INET;
  serverAddrInet.sin_port = htons(SERVPORT);          //PORT);
  serverAddrInet.sin_addr.s_addr = htonl(INADDR_ANY); // htonl(INADDR_LOOPBACK); //htonl(INADDR_ANY);

  memset(&serverAddrSock, 0, sizeof(struct sockaddr_un));
  serverAddrSock.sun_family = AF_UNIX;
  strncpy(serverAddrSock.sun_path, SERVER_SOCKET, sizeof(serverAddrSock.sun_path) - 1);

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
  // Bind serverHandle
  //===================================================================
  if (inet)
  {

    int iReuseOptVal = setReuseAddr(serverHandle, 1);
    int iKeepAliveOptVal = setKeepAlive(serverHandle, 0);
    printf("unix socket %s - 0.0.0.0:%d %d reuseaddr: %d, keepalive: %d\n",
           "Inet",
           SERVPORT,
           serverHandle,
           iReuseOptVal,
           iKeepAliveOptVal);

    int bErr = bind(serverHandle,
                    (const struct sockaddr *)&serverAddrInet,
                    sizeof(struct sockaddr_in));
    if (bErr < 0)
      handleError(serverHandle, "bind():" + std::to_string(bErr));

    printf("bind success, 0.0.0.0:%d\n", SERVPORT);
    int lErr = listen(serverHandle, LISTEN_BACKLOG);
    if (lErr < 0)
      handleError(serverHandle, "listen():" + std::to_string(lErr));
  }
  else
  {
    printf("unix socket %s - %d\n", SERVER_SOCKET, serverHandle);
    int bErr = bind(serverHandle,
                    (const struct sockaddr *)&serverAddrSock,
                    sizeof(struct sockaddr_un));
    if (bErr < 0)
      handleErrorWithUnlink(serverHandle, "bind():" + std::to_string(bErr));

    printf("bind success, %s\n", SERVER_SOCKET);
    int lErr = listen(serverHandle, LISTEN_BACKLOG);
    if (lErr < 0)
      handleErrorWithUnlink(serverHandle, "listen():" + std::to_string(lErr));
  }

  //===================================================================
  // Accept connections on SERVER_SOCKET
  //===================================================================
  long timeout = 1; // secs
  struct timeval selTimeOut;
  // Only one port sock in the select
  int maxDescriptor = 0;
  int servSock[1];
  servSock[0] = serverHandle;
  maxDescriptor = servSock[0];
  fd_set sockSet;
  int running = 1;

  while (running)
  {
    int peerHandle;

    // Zero socket descriptor vector and set for server sockets
    // This must be reset every time select() is called
    FD_ZERO(&sockSet);
    FD_SET(STDIN_FILENO, &sockSet);
    FD_SET(servSock[0], &sockSet);

    // Timeout spefifications
    // This must be reset every time select() is called
    selTimeOut.tv_sec = timeout; // timeout (seconds)
    selTimeOut.tv_usec = 0;      // 0 milliseconds

    // Suspend program until descriptor is ready or timeout
    int sel = select(maxDescriptor + 1, &sockSet, NULL, NULL, &selTimeOut);
    if (sel == 0)
    {
      printf("No connections for %ld secs... server still alive\n", timeout);
    }
    else
    {
      if (FD_ISSET(0, &sockSet))
      { // Check keyboard
        printf("Shutting down server\n");
        getchar();
        running = 0;
      }
      if (FD_ISSET(servSock[0], &sockSet))
      {
        printf("Request on port %d ", servSock[0]); // only the one serverSocket

        // Accept
        peerHandle = acceptClientConnection(servSock[0], inet);
        printf("accepted, %d \n", peerHandle);

        // handle client
        handleClient(peerHandle, servSock[0], inet);
      }
    }

  } /// end while

  FD_CLR(STDIN_FILENO, &sockSet);
  FD_CLR(servSock[0], &sockSet);
  //===================================================================
  // Close connections on SERVER_SOCKET
  //===================================================================
  printf("Close server handle %d\n", serverHandle);
  close(serverHandle);

  if (!inet)
    unlink(SERVER_SOCKET);

  return 0;
}

//===================================================================
// Global functions
//===================================================================
int acceptClientConnection(int serverHandle, bool inet)
{
  // Accept
  struct sockaddr_in peerAddrInet;
  struct sockaddr_un peerAddrSock;

  int peerHandle = 0;
  if (inet)
  {
    socklen_t peer_addr_size = sizeof(struct sockaddr_in);
    printf("accept connections, 0.0.0.0:%d\n", SERVPORT);
    peerHandle = accept(serverHandle, (struct sockaddr *)&peerAddrInet, &peer_addr_size);
    if (peerHandle < 0)
      handleError(serverHandle, "accept():" + std::to_string(peerHandle));
  }
  else
  {
    socklen_t peer_addr_size = sizeof(struct sockaddr_un);
    printf("accept connections, %s\n", SERVER_SOCKET);
    peerHandle = accept(serverHandle, (struct sockaddr *)&peerAddrSock, &peer_addr_size);
    if (peerHandle < 0)
      handleErrorWithUnlink(serverHandle, "accept():" + std::to_string(peerHandle));
  }

  return peerHandle;
}

void handleClient(int peerHandle, int serverHandle, bool inet)
{
  char dataBuffer[DATA_BUF_SIZE];
  unsigned int clientLen;
  int SendBytes;
  int RecvBytes;
  int result = 0;
  int ret = 0;
  int flags = 0;

  /* Wait for next data packet. */
  ret = recv(peerHandle, dataBuffer, DATA_BUF_SIZE, flags);
  if (ret < 0)
  {
    if (inet)
      handleErrorWithPeer(serverHandle, peerHandle, "recv()");
    else
      handleErrorWithPeerAndUnlink(serverHandle, peerHandle, "recv()");
  }
  else if (ret == 0)
  {
    // Free peer and close handle
    printf("Connection of peer closed: [%d]\n", ret);
    close(peerHandle);
    //break;
    return;
  }

  dataBuffer[ret] = '\0';
  /* Ensure buffer is 0-terminated. */
  //buffer[BUFFER_SIZE - 1] = 0;
  //buffer[BUFFER_SIZE - 1] = 0;
  printf("Connection recv() is OK. Received %d bytes: [%s]\n", ret, dataBuffer);

  /* Handle commands. */
  if (!strncmp(dataBuffer, "END", DATA_BUF_SIZE))
  {
    close(peerHandle);
    //break;
    return;
  }

  // Do process for msg read if not the END command
  // Send message acknowledge and return to recv on peer
  // std::string message("ECHO:" + std::string(dataBuffer));
  // strcpy(dataBuffer, message.c_str());
  // printf("server sending %s %d\t", dataBuffer, message.size());

  std::string send_msg("ECHO:" + std::string(dataBuffer));
  auto result_length = strnlen(send_msg.c_str(), sizeof(dataBuffer));
  if (0 >= result_length)
  {
    send_msg = "NOTOK";
    result_length = strnlen(send_msg.c_str(), sizeof(dataBuffer));
  }
  printf("BSD send is %s %d\n", send_msg.c_str(), static_cast<int>(result_length));
  strncpy(dataBuffer, send_msg.c_str(), result_length);
  dataBuffer[result_length] = '\0';

  ret = send(peerHandle, dataBuffer, (int)result_length, (int)0);
  if (ret < 0)
  {
    if (inet)
      handleErrorWithPeer(serverHandle, peerHandle, "send()");
    else
      handleErrorWithPeerAndUnlink(serverHandle, peerHandle, "send()");
  }

  printf("Handle client - close peer\n");
  close(peerHandle);

  return;
}

void handleErrorWithUnlink(int fh, const std::string &msg)
{
  close(fh);
  unlink(SERVER_SOCKET);
  //std::cerr << "BSDUnix Endpoint error: " << msg << "\n";
  perror(std::string("BSDUnix Endpoint error: " + msg).c_str());

  exit(1);
}

void handleErrorWithPeerAndUnlink(int fh, int ph, const std::string &msg)
{
  close(ph);
  close(fh);
  unlink(SERVER_SOCKET);
  std::cerr << "BSDUnix Endpoint error: " << msg << "\n";
  exit(1);
}

void handleError(int fh, const std::string &msg)
{
  close(fh);
  //std::cerr << "BSDInet Endpoint error: " << msg << "\n";
  perror(std::string("BSDInet Endpoint error: " + msg).c_str());
  exit(1);
}

void handleErrorWithPeer(int fh, int ph, const std::string &msg)
{
  close(ph);
  close(fh);
  //std::cerr << "BSDInet Endpoint error: " << msg << "\n";
  perror(std::string("BSDInet Endpoint error: " + msg).c_str());
  exit(1);
}

int setReuseAddr(int serverHandle, int val)
{
  int iResult = 0;
  int iOptVal = 0;
  socklen_t optLen = sizeof(iOptVal);
  iResult = getsockopt(serverHandle, SOL_SOCKET, SO_REUSEADDR, &iOptVal, &optLen);
  if (iResult < 0)
  {
    perror("getsockopt for SO_REUSEADDR failed\n");
  }
  printf("Reuse addr was: %d\n", iOptVal);
  if (val == 1 && iOptVal == 0)
  {
    iOptVal = 1;
    iResult = setsockopt(serverHandle, SOL_SOCKET, SO_REUSEADDR, &iOptVal, sizeof(iOptVal));
    if (iResult < 0)
    {
      perror("setsockopt for SO_REUSEADDR failed\n");
    }
  }
  iResult = getsockopt(serverHandle, SOL_SOCKET, SO_REUSEADDR, &iOptVal, &optLen);
  return iOptVal;
}

int setKeepAlive(int serverHandle, int val)
{
  int iResult = 0;
  int bOptVal = 0;
  socklen_t optLen = sizeof(bOptVal);
  iResult = setsockopt(serverHandle, SOL_SOCKET, SO_KEEPALIVE, &bOptVal, sizeof(bOptVal));
  if (iResult < 0)
  {
    perror("setsockopt for SO_KEEPALIVE failed\n");
  }
  printf("Keepalive addr was: %d %d\n", val , bOptVal);
  if (val == 1 && bOptVal == 0)
  {
    bOptVal = 1;
    iResult = setsockopt(serverHandle, SOL_SOCKET, SO_KEEPALIVE, &bOptVal, sizeof(bOptVal));
    if (iResult < 0)
    {
      perror("setsockopt for SO_REUSEADDR failed\n");
    }
  }
  iResult = getsockopt(serverHandle, SOL_SOCKET, SO_KEEPALIVE, &bOptVal, &optLen);
  return bOptVal;
}
