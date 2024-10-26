#pragma once

#include <stdint.h>

#define STUB_ADDR 0x8000'1000
#define STUB_STACK 0x8000'3000

extern const uint8_t stub[];
extern const uint8_t stub_size[];
