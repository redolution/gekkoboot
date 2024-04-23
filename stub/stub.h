#pragma once

#include <stdint.h>

#define STUB_ADDR  0x80001000
#define STUB_STACK 0x80003000

extern const uint8_t stub[];
extern const uint8_t stub_size[];
extern void stub_main(const void *buf, size_t buflen, const void *arg, size_t arglen);
