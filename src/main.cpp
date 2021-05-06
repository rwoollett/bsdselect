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

//constexpr const char *SERVER_SOCKET = "/home/rwlltt/dockers/bsdtest/test.sock";
constexpr const char *SERVER_SOCKET = "/tmp/test.sock";
constexpr int LISTEN_BACKLOG = 5;
constexpr int DATA_BUF_SIZE = 256;
constexpr int SERVPORT = 8080;
void handleError(int fh, const std::string &msg);
void handleErrorWithPeer(int fh, int ph, const std::string &msg);
void handleErrorWithUnlink(int fh, const std::string &msg);
void handleErrorWithPeerAndUnlink(int fh, int ph, const std::string &msg);

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
  struct sockaddr_un peerAddrSock;
  struct sockaddr_in serverAddrInet;
  struct sockaddr_in peerAddrInet;

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
    printf("unix socket %s - 0.0.0.0:%d %d\n", "Inet", SERVPORT, serverHandle);
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
  for (;;)
  {
    int peerHandle;

    // Accept
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

    printf("accepted, %d \n", peerHandle);

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
      break;
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
      break;
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
    printf("BSDUnix send is %s %d\n", send_msg.c_str(), static_cast<int>(result_length));
    strncpy(dataBuffer, send_msg.c_str(), result_length);
    dataBuffer[result_length] = '\0';

    ret = send(serverHandle, dataBuffer, (int)result_length, (int)0);
    if (ret < 0)
    {
      if (inet)
        handleErrorWithPeer(serverHandle, peerHandle, "send()");
      else
        handleErrorWithPeerAndUnlink(serverHandle, peerHandle, "send()");
    }

  } /// end for (;;)

  //===================================================================
  // Close connections on SERVER_SOCKET
  //===================================================================
  close(serverHandle);

  if (!inet)
    unlink(SERVER_SOCKET);

  return 0;
}

//===================================================================
// Global functions
//===================================================================
void handleErrorWithUnlink(int fh, const std::string &msg)
{
  close(fh);
  unlink(SERVER_SOCKET);
  std::cerr << "BSDUnix Endpoint error: " << msg << "\n";
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
  std::cerr << "BSDInet Endpoint error: " << msg << "\n";
  exit(1);
}

void handleErrorWithPeer(int fh, int ph, const std::string &msg)
{
  close(ph);
  close(fh);
  std::cerr << "BSDInet Endpoint error: " << msg << "\n";
  exit(1);
}
