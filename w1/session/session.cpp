#include "session.h"
#include "../socket_tools.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

const char* Session::SYN_MSG_START       = "Hello, Server! I am a client and my name is ";
const char* Session::SYNACK_MSG_START    = "Hello, Client! I am a server and I hear you well, ";
const char* Session::ACK_MSG_START       = "Roger that! Now let's talk";
const char* Session::KEEPALIVE_MSG_START = "Still alive";

Session::Session(const char* syn_msg, const sockaddr* s, socklen_t sock_len, int write_descriptor) :
  _sock_addr(*s),
  _sock_len(sock_len),
  _name(syn_msg + strlen(SYN_MSG_START)),
  _write_descriptor(write_descriptor)
{
  // Get ip and socket from sockaddr_in
  struct sockaddr_in *sin = (struct sockaddr_in *)s;
  char ip_buff[INET_ADDRSTRLEN] = {};
  char port_buff[INET_ADDRSTRLEN] = {};
  
  int res = getnameinfo(s, sock_len, ip_buff, sizeof(ip_buff), port_buff, sizeof(port_buff), 0);

  _ip.assign(ip_buff);
  _port.assign(port_buff);
}

Session::~Session() {
}

static bool str_starts_with(const char *pre, const char *str) {
  char cp;
  char cs;

  if (!*pre)
    return true;

  while ((cp = *pre++) && (cs = *str++)) {
    if (cp != cs)
      return false;
  }

  return cs;
}

bool Session::is_syn(const char* str) {
  return str_starts_with(Session::SYN_MSG_START, str);
}

bool Session::is_synack(const char* str) {
  return str_starts_with(Session::SYNACK_MSG_START, str);
}

bool Session::is_ack(const char* str) {
  return str_starts_with(Session::ACK_MSG_START, str);
}


bool Session::is_keepalive(const char* str) {
  return str_starts_with(Session::KEEPALIVE_MSG_START, str);
}

std::string Session::make_str_socket(const sockaddr* s) {
  char buff[2*INET_ADDRSTRLEN] = {};
  
  int res = getnameinfo(s, INET_ADDRSTRLEN, buff, INET_ADDRSTRLEN, buff + INET_ADDRSTRLEN, INET_ADDRSTRLEN, 0);
  if (res != 0) {
    printf("Error %d at file %s, line %d", errno, __FILE__, __LINE__);
    return std::string("Error");
  }
  
  strcat(buff, buff+INET_ADDRSTRLEN);
  return std::string(buff);
}

int Session::send(const char* message, size_t len) {
  return sendto(_write_descriptor, message, len, 0, &_sock_addr, _sock_len);
}

int Session::send_synack() {
  std::string msg(SYNACK_MSG_START);
  msg += _name;
  return send(msg.c_str(), msg.length());
}