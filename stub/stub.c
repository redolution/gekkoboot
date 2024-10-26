/*
 * Copyright (c) 2014-2023, Extrems' Corner.org
 * All rights reserved.
 */

#include <ogc/machine/processor.h>
#include <ogc/system.h>

typedef struct {
	ptrdiff_t section_offset[18];

	void *section[18];
	size_t section_size[18];

	void *bss;
	size_t bss_size;

	void (*entrypoint)(void);
} dol_header_t;

static void
memsync(void *buf, size_t size) {
	uint8_t *b = buf;
	size_t i;

	for (i = 0; i < size; i += PPC_CACHE_ALIGNMENT) {
		asm("dcbst %y0" ::"Z"(b[i]));
	}
}

static void
memzero(void *buf, size_t size) {
	uint8_t *b = buf;
	uint8_t *e = b + size;

	while (b != e) {
		*b++ = '\0';
	}
}

static void *
memmove(void *dest, const void *src, size_t size) {
	uint8_t *d = dest;
	const uint8_t *s = src;
	size_t i;

	if (d < s) {
		for (i = 0; i < size; ++i) {
			d[i] = s[i];
		}
	} else if (d > s) {
		i = size;
		while (i-- > 0) {
			d[i] = s[i];
		}
	}

	return dest;
}

// Workaround for GCC bug 5372 – [powerpc-*-eabi] -mno-eabi not working
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=5372
// Entry point can't be called main or GCC will insert a call to __eabi
__attribute__((noreturn)) void
not_main(const void *buf, size_t buflen, const void *arg, size_t arglen) {
	dol_header_t header;
	uint32_t *entrypoint;
	uint8_t args[arglen];

	memmove(&header, buf, sizeof(dol_header_t));
	memmove(args, arg, arglen);

	if (header.section[0] < buf + header.section_offset[0]) {
		for (int i = 0; i < 18; i++) {
			memmove(header.section[i],
			        buf + header.section_offset[i],
			        header.section_size[i]);
			memsync(header.section[i], header.section_size[i]);
		}
	} else {
		for (int i = 18; i > 0; i--) {
			memmove(header.section[i - 1],
			        buf + header.section_offset[i - 1],
			        header.section_size[i - 1]);
			memsync(header.section[i - 1], header.section_size[i - 1]);
		}
	}

	memzero(header.bss, header.bss_size);
	memsync(header.bss, header.bss_size);

	entrypoint = MEM_K0_TO_K1(header.entrypoint);

	if (entrypoint[1] == ARGV_MAGIC && arglen) {
		entrypoint[2] = ARGV_MAGIC;
		entrypoint[3] = (intptr_t) args;
		entrypoint[4] = arglen;
	}

	_sync();
	mthid0(mfhid0() | 0xC00);
	header.entrypoint();

	__builtin_unreachable();
}
