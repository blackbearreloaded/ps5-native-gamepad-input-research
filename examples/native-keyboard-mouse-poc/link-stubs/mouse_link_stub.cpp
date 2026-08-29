/*
 * Link-time declarations for toolchains that do not ship a libSceMouse stub.
 * These fallback bodies must be replaced with libSceMouse imports by the
 * executable's module-linking step; they must never be packaged as app code.
 */

#include <cstdint>

extern "C" {

std::int32_t sceMouseInit() { return -1; }

std::int32_t sceMouseOpen(
    std::int32_t, std::int32_t, std::int32_t, const void*)
{
    return -1;
}

std::int32_t sceMouseRead(std::int32_t, void*, std::int32_t) { return -1; }

std::int32_t sceMouseClose(std::int32_t) { return -1; }

}  // extern "C"
