#pragma once

const uint32_t SYSTIME_SEND_INTERVAL_MS = 100;
const uint32_t PING_SEND_INTERVAL_MS = 1000;

const uint16_t DEFAULT_LOBBY_PORT = 10887;
const uint16_t DEFAULT_SERVER_PORT = 5397;

const uint8_t FLAG_IS_START       = 1 << 1;
const uint8_t FLAG_IS_SYSTIME     = 1 << 2;
const uint8_t FLAG_IS_SERVER_DATA = 1 << 3;
const uint8_t FLAG_IS_PLAYER_LIST = 1 << 4;
const uint8_t FLAG_IS_PING_LIST   = 1 << 5;
const uint8_t FLAG_IS_NEW_PLAYER  = 1 << 6;