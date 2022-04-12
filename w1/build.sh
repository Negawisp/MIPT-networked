#!/bin/bash
g++ client.cpp -g socket_tools.cpp -o client
g++ server.cpp -g socket_tools.cpp -o server