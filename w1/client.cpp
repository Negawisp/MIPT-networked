#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "session/session.h"
#include "socket_tools.h"

std::string read_name() {
  std::string s_name;
  printf("Please, enter client name: ");
  std::getline(std::cin, s_name);
  return s_name;
}

struct connection_data {
  int sfd;
  addrinfo addr_info;
};

connection_data connect_to_server(const char* server_ip, const char* server_port, const char* user_name) {
  ssize_t sock_write_res = -1;
  connection_data ret {};

  // Create write socket
  ret.sfd = create_dgram_socket(server_ip, server_port, &ret.addr_info);
  if (ret.sfd == -1) {
    printf("Cannot create write socket\n");
    return ret;
  }

  // Send syn to server
  std::string syn_message(Session::SYN_MSG_START);
  syn_message = syn_message.append(user_name);
  sock_write_res = sendto(ret.sfd, syn_message.c_str(), syn_message.size(), 0, ret.addr_info.ai_addr, ret.addr_info.ai_addrlen);
  if (sock_write_res == -1)
    std::cout << strerror(errno) << std::endl;

  // Wait for synack
  while (true) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(ret.sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(ret.sfd + 1, &readSet, NULL, NULL, &timeout);

    if (FD_ISSET(ret.sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr addr;
      memset(&addr, 0, sizeof(sockaddr));
      socklen_t addrlen = 30;

      ssize_t numBytes = recvfrom(ret.sfd, buffer, buf_size - 1, 0, &addr, &addrlen);
      if (numBytes > 0)
      {
        // Message is not from server 
        if (!sockaddr_equals(&addr, ret.addr_info.ai_addr))
          continue;

        // Message is not synack
        if (!Session::is_synack(buffer))
          continue;

        // Message is synack
        printf("%s\n", buffer);
        break;
      }
    }
  }

  // Send ack to server
  std::string ack_message(Session::ACK_MSG_START);
  ack_message = ack_message.append(user_name);
  sock_write_res = sendto(ret.sfd, ack_message.c_str(), ack_message.size(), 0, ret.addr_info.ai_addr, ret.addr_info.ai_addrlen);
  if (sock_write_res == -1)
    std::cout << strerror(errno) << std::endl;

  return ret;
} 

ssize_t send(const connection_data* con_data, std::string msg) {
  ssize_t res = sendto(con_data->sfd, msg.c_str(), msg.size(), 0, con_data->addr_info.ai_addr, con_data->addr_info.ai_addrlen);
  if (res == -1)
    std::cout << "Error " << errno << ": " << strerror(errno) << std::endl;
  return res;
}

int main(int argc, const char **argv)
{
  const char* server_ip = "127.0.0.1";
  const char* server_port = "2022";

  std::string name = read_name();
  connection_data con_data = connect_to_server(server_ip, server_port, name.c_str());

  time_t keepalive_timestamp = time(NULL);

  bool is_quit = false;
  auto input_reading_and_sending = std::thread([&]{
      std::string s;
      while(!is_quit) {
        // reading input
        printf(">");
        std::getline(std::cin, s, '\n');

        if (s == "quit") {
            is_quit = true;
        }
         
        // sending to server
        send(&con_data, s);

        // ssize_t res = sendto(con_data.sfd, s.c_str(), s.size(), 0, con_data.addr_info.ai_addr, con_data.addr_info.ai_addrlen);
        // if (res == -1)
        //   std::cout << "Error " << errno << ": " << strerror(errno) << std::endl;
      }
      is_quit = true;
  });


  auto current_string = std::string();
  while (true) {
    // Checking if quit
    if (is_quit) {
      break;
    }

    // Receiving message from server
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(con_data.sfd, &readSet);

    timeval timeout = { 0, 100000 }; // 100 ms
    select(con_data.sfd + 1, &readSet, NULL, NULL, &timeout);

    if (FD_ISSET(con_data.sfd, &readSet))
    {
      constexpr size_t buf_size = 1000;
      static char buffer[buf_size];
      memset(buffer, 0, buf_size);

      sockaddr addr;
      memset(&addr, 0, sizeof(sockaddr));
      socklen_t addrlen = 30;

      ssize_t numBytes = recvfrom(con_data.sfd, buffer, buf_size - 1, 0, &addr, &addrlen);
      if (numBytes > 0)
      {
        // message is not from server 
        if (!sockaddr_equals(&addr, con_data.addr_info.ai_addr))
          continue;

        // message is a keep-alive datagram
        if (Session::is_keepalive(buffer))
          continue;
        
        // message is presumably a chat-text from another user
        printf("%s\n", buffer);
      }
    }

    
    // sending keepalive packets
    time_t now = time(NULL);
    if (now > keepalive_timestamp) {
      send(&con_data, Session::KEEPALIVE_MSG_START);

      tm now_tm = *localtime(&now);
      now_tm.tm_sec += 1;
      keepalive_timestamp = mktime(&now_tm);
    }
  }

  input_reading_and_sending.join();
  return 0;
}
