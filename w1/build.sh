#!/bin/bash
g++ -std=c++11 -pthread client.cpp -g session/session_set.cpp session/session.cpp socket_tools.cpp -o ./build/client
g++ -std=c++11 server.cpp -g session/session_set.cpp session/session.cpp socket_tools.cpp -o ./build/server