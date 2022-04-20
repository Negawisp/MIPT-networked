#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <map>

#include "session/session_set.h"
#include "socket_tools.h"

int main(int argc, const char **argv)
{
  const char *port = "2022";
  SessionSet sessions;

  time_t keepalive_timestamp = time(NULL);

  int sfd = create_dgram_socket(nullptr, port, nullptr);

  if (sfd == -1)
    return 1;
  printf("listening!\n");

  while (true)
  {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(sfd + 1, &readSet, NULL, NULL, &timeout);

    if (FD_ISSET(sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr addr;
      memset(&addr, 0, sizeof(sockaddr));
      socklen_t addrlen = 30;

      ssize_t numBytes = recvfrom(sfd, buffer, buf_size - 1, 0, &addr, &addrlen);
      if (numBytes > 0) {
        // Message is from not-acked client
        if (!sessions.is_acked(&addr)) {
          if (Session::is_syn(buffer)) {
            Session* session = sessions.init(buffer, &addr, addrlen, sfd);
            int res = session->send_synack();
            if (res == -1) {
              printf("Error %n\n", &errno);
            }
          }
          else if (Session::is_ack(buffer)) {
            sessions.acknowledge(&addr);
          }
          
          continue;
        }

        // message is from an acked client
        // message is a keep-alive datagram
        if (Session::is_keepalive(buffer))
          continue;

        // message is presumably a chat-text
        Session* sender = sessions.get(&addr);
        std::string msg;
        msg
          .assign(sender->get_name())
          .append("[")
          .append(sender->get_ip().c_str())
          .append(":")
          .append(sender->get_port().c_str())
          .append("] says: ")
          .append(buffer);

        sessions.send_foreach_but(&addr, msg.c_str());
        printf("%s\n", msg.c_str());
      }
    }

    // sending keepalive packets
    time_t now = time(NULL);
    if (now > keepalive_timestamp) {
      sessions.send_foreach(Session::KEEPALIVE_MSG_START);

      tm now_tm = *localtime(&now);
      now_tm.tm_sec += 1;
      keepalive_timestamp = mktime(&now_tm);
    }
  }
  return 0;
}