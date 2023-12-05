#pragma once

#include <stdint.h>

#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

extern const uint8_t stub[];
extern const uint8_t stub_size[];
