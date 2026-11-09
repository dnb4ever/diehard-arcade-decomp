// Portable, bounded in-app preparation of the owner's MAME diehard set.
#ifndef P413_ROMS_H
#define P413_ROMS_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// 1 prepared; 0 absent/unreadable; -1 unsupported/corrupt set or output failure.
// src: zip path, folder with diehard.zip, loose members, or prepared cart.bin.
int dha_prepare_roms(const char* src, const char* destination, char* error, size_t error_size);
// Verify both prepared images, including SHA-256. Does not create directories.
int dha_roms_ready(const char* directory);
#ifdef __cplusplus
}
#endif
#endif
