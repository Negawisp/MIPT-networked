#pragma once

#include "session.h"
#include <map>

class SessionSet {
public:
  SessionSet();
  ~SessionSet();

  Session* init(const char* syn_message, const sockaddr* s, socklen_t sock_len, int write_descriptor);
  void acknowledge(const sockaddr* s);

  bool is_syned(const sockaddr* s);
  bool is_acked(const sockaddr* s);

  void send_foreach(const char* message);
  void send_foreach_but(const sockaddr* but, const char* message);

  Session* get(const sockaddr* sockaddr);

private:
  std::map<std::string, Session*> _syned_sessions;
  std::map<std::string, Session*> _acked_sessions;

  void send(const sockaddr* receiver, const char* message);
};