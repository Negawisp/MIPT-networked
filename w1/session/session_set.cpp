#include "session_set.h"
#include <cstring>

SessionSet::SessionSet() {

}

SessionSet::~SessionSet() {
  for (const auto& pair : _syned_sessions) {
    Session* s = pair.second;
    delete(s);
  }

  for (const auto& pair : _acked_sessions) {
    Session* s = pair.second;
    delete(s);
  }
}


Session* SessionSet::init(const char* syn_message, const sockaddr* s, socklen_t sock_len, int write_descriptor) {
  Session* session = new Session(syn_message, s, sock_len, write_descriptor);
  _syned_sessions[Session::make_str_socket(s)] = session;
  return session;
}

void SessionSet::acknowledge(const sockaddr* s) {
  std::string str_sock = Session::make_str_socket(s);
  auto search = _syned_sessions.find(str_sock);
  if (search != _syned_sessions.end()) {
    _acked_sessions[str_sock] = search->second;
    _syned_sessions.erase(str_sock);
  }
}


bool SessionSet::is_syned(const sockaddr* s) {
  auto search = _syned_sessions.find(Session::make_str_socket(s));
  return search != _syned_sessions.end();
}

bool SessionSet::is_acked(const sockaddr* s) {
  auto search = _acked_sessions.find(Session::make_str_socket(s));
  return search != _acked_sessions.end();
}


void SessionSet::send_foreach(const char* message) {
  size_t message_size = strlen(message);
  for (const auto& pair : _acked_sessions) {
    pair.second->send(message, message_size);
  }
}

void SessionSet::send_foreach_but(const sockaddr* but, const char* message) {
  std::string str_sock = Session::make_str_socket(but);
  
  size_t message_size = strlen(message);
  for (const auto& pair : _acked_sessions) {
    if (!pair.first.compare(str_sock)) continue;
    else pair.second->send(message, message_size);
  }
}

Session* SessionSet::get(const sockaddr* sockaddr) {
  auto search = _acked_sessions.find(Session::make_str_socket(sockaddr));
  return search != _acked_sessions.end() ? search->second : NULL;
}