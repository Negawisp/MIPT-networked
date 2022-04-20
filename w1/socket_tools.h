#pragma once

struct addrinfo;

int get_dgram_socket(addrinfo *addr, bool should_bind, addrinfo *res_addr);
int create_dgram_socket(const char *address, const char *port, addrinfo *res_addr);
bool sockaddr_equals(const sockaddr* x, const sockaddr* y);
