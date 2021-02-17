#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define UPDC32(octet, crc) (crc_32_tab[((crc >> 24 ) ^ (octet)) & 0xff] ^ ((crc) << 8))
#define FINAL 0xffffffff

uint32_t crc32buf(const char* buf, size_t len);
uint32_t crc32str(const char* buf);
uint32_t find_old_crc(uint32_t current_crc, char removed);
uint32_t compute_removed_crc(const char* s, char c);
uint32_t compute_removed_str(const char* s, const char* s2);
uint32_t compute_removed_str2(uint32_t oldhash, const char* s2);
uint32_t endian(uint32_t num);


