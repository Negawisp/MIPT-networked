#!/bin/bash
g++ -pthread client.cpp -g common/enet_tools.cpp -o ./build/client -lenet
g++ lobby.cpp -g common/enet_tools.cpp -o ./build/lobby -lenet
g++ server.cpp -g common/enet_tools.cpp -o ./build/server -lenet