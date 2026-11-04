#include "disc_probe.hpp"

#include <borealis/log.hpp>
#include <fmt/format.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_error.h>

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(__ANDROID__) || defined(ANDROID)
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace dusk::disc_probe {
namespace {

constexpr borealis::Log ProbeLog{"dusk::disc_probe"};
constexpr size_t kChunk = 64 * 1024;

uint64_t fnv1a(const uint8_t* data, size_t size) {
    uint64_t hash = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= 0x100000001b3ull;
    }
    return hash;
}

std::string hex_bytes(const uint8_t* data, size_t size) {
    std::string out;
    for (size_t i = 0; i < size; ++i) {
        out += fmt::format("{:02x}", data[i]);
    }
    return out;
}

std::string printable(const uint8_t* data, size_t size) {
    std::string out;
    for (size_t i = 0; i < size; ++i) {
        out += (data[i] >= 0x20 && data[i] < 0x7f) ? static_cast<char>(data[i]) : '.';
    }
    return out;
}

// SDL seek + read, looping like aurora's sdlStreamReadAt.
int64_t sdl_read_at(SDL_IOStream* io, uint64_t offset, uint8_t* out, size_t len) {
    if (SDL_SeekIO(io, static_cast<Sint64>(offset), SDL_IO_SEEK_SET) < 0) {
        return -1;
    }
    size_t total = 0;
    while (total < len) {
        const size_t read = SDL_ReadIO(io, out + total, len - total);
        if (read == 0) {
            break;
        }
        total += read;
    }
    return static_cast<int64_t>(total);
}

}  // namespace

void log_content_uri(std::string_view location, const char* reason) noexcept {
#if defined(__ANDROID__) || defined(ANDROID)
    if (!location.starts_with("content://")) {
        return;
    }
    const std::string path{location};
    ProbeLog.info("[disc-probe] ({}) uri={}", reason, path);

    SDL_ClearError();
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
    if (io == nullptr) {
        ProbeLog.error("[disc-probe] SDL_IOFromFile failed: {}", SDL_GetError());
        return;
    }
    const Sint64 sdlSize = SDL_GetIOSize(io);
    ProbeLog.info("[disc-probe] SDL size={} (a full GameCube disc is 1459978240)", sdlSize);

    auto* fp = static_cast<FILE*>(
        SDL_GetPointerProperty(SDL_GetIOProperties(io), SDL_PROP_IOSTREAM_STDIO_FILE_POINTER, nullptr));
    const int fd = fp != nullptr ? fileno(fp) : -1;
    if (fd < 0) {
        ProbeLog.warn("[disc-probe] no stdio FILE behind the stream (fp={}), pread comparison skipped",
                      static_cast<const void*>(fp));
    } else {
        std::array<char, 512> target{};
        const std::string link = fmt::format("/proc/self/fd/{}", fd);
        const ssize_t linkLen = readlink(link.c_str(), target.data(), target.size() - 1);
        struct stat st{};
        const int statResult = fstat(fd, &st);
        const char* type = S_ISREG(st.st_mode)  ? "regular file" :
                           S_ISFIFO(st.st_mode) ? "pipe" :
                           S_ISSOCK(st.st_mode) ? "socket" :
                           S_ISCHR(st.st_mode)  ? "char device" :
                                                  "other";
        const off_t pos = lseek(fd, 0, SEEK_CUR);
        ProbeLog.info("[disc-probe] fd={} -> '{}' fstat={} size={} type={} lseek(cur)={}{}", fd,
                      linkLen > 0 ? std::string(target.data(), static_cast<size_t>(linkLen)) : "?",
                      statResult, static_cast<long long>(st.st_size), type, static_cast<long long>(pos),
                      pos < 0 ? fmt::format(" (errno {}: {})", errno, std::strerror(errno)) : "");
    }

    // A second, independent open: does it see the same bytes?
    SDL_IOStream* io2 = SDL_IOFromFile(path.c_str(), "rb");

    std::vector<uint8_t> viaSdl(kChunk), viaPread(kChunk), viaSecond(kChunk);
    std::vector<uint64_t> offsets{0, 0x400, 0x10000, 0x100000};
    if (sdlSize > 0) {
        const auto size = static_cast<uint64_t>(sdlSize);
        for (uint64_t off : {size / 4, size / 2, (size / 4) * 3, size > kChunk ? size - kChunk : 0}) {
            offsets.push_back(off & ~uint64_t{0xfff});
        }
    }
    int mismatches = 0;
    for (const uint64_t off : offsets) {
        const int64_t a = sdl_read_at(io, off, viaSdl.data(), kChunk);
        int64_t b = -2;
        if (fd >= 0) {
            b = pread(fd, viaPread.data(), kChunk, static_cast<off_t>(off));
        }
        const int64_t c = io2 != nullptr ? sdl_read_at(io2, off, viaSecond.data(), kChunk) : -2;
        const uint64_t ha = a > 0 ? fnv1a(viaSdl.data(), static_cast<size_t>(a)) : 0;
        const uint64_t hb = b > 0 ? fnv1a(viaPread.data(), static_cast<size_t>(b)) : 0;
        const uint64_t hc = c > 0 ? fnv1a(viaSecond.data(), static_cast<size_t>(c)) : 0;
        const bool same = (b < 0 || (a == b && ha == hb)) && (c < 0 || (a == c && ha == hc));
        mismatches += same ? 0 : 1;
        ProbeLog.info("[disc-probe] off=0x{:x} sdl={}B {:016x} pread={}B {:016x} second-open={}B {:016x} {}", off, a,
                      ha, b, hb, c, hc, same ? "same" : "DIFFERENT");
    }

    // Sequential reads without seeking, the way a whole-disc hash reads.
    if (fd >= 0 && SDL_SeekIO(io, 0, SDL_IO_SEEK_SET) == 0) {
        for (int i = 0; i < 16; ++i) {
            const size_t a = SDL_ReadIO(io, viaSdl.data(), kChunk);
            const ssize_t b = pread(fd, viaPread.data(), kChunk, static_cast<off_t>(i) * kChunk);
            const bool same = static_cast<ssize_t>(a) == b && std::memcmp(viaSdl.data(), viaPread.data(), a) == 0;
            if (!same) {
                ++mismatches;
                ProbeLog.info("[disc-probe] sequential chunk {} sdl={}B pread={}B DIFFERENT", i, a, b);
            }
        }
        ProbeLog.info("[disc-probe] sequential first 1 MiB checked against pread");
    }

    if (sdl_read_at(io, 0, viaSdl.data(), 0x20) == 0x20) {
        ProbeLog.info("[disc-probe] header {} '{}' (expect game id GZ2E01)", hex_bytes(viaSdl.data(), 0x20),
                      printable(viaSdl.data(), 0x20));
    }
    ProbeLog.info("[disc-probe] done: {} mismatch(es)", mismatches);

    if (io2 != nullptr) {
        SDL_CloseIO(io2);
    }
    SDL_CloseIO(io);
#else
    (void)location;
    (void)reason;
#endif
}

}  // namespace dusk::disc_probe
