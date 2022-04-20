#pragma once

#include <string>
#include <arpa/inet.h>

class Session {
public:
  static const char* SYN_MSG_START;
  static const char* SYNACK_MSG_START;
  static const char* ACK_MSG_START;
  static const char* KEEPALIVE_MSG_START;

  static bool is_syn(const char* str);
  static bool is_synack(const char* str);
  static bool is_ack(const char* str);
  static bool is_keepalive(const char* str);

  static std::string make_str_socket(const sockaddr* s);

  Session(const char* syn_message, const sockaddr* s, socklen_t sock_len, int write_descriptor);
  ~Session();

  std::string get_name() { return _name; }
  std::string get_ip()   { return _ip; }
  std::string get_port() { return _port; }

  int send(const char* message, size_t len);
  int send_synack();

private:
  sockaddr    _sock_addr;
  socklen_t   _sock_len;
  std::string _name;
  std::string _ip;
  std::string _port;

  int         _write_descriptor;
};
