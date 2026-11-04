#pragma once

#include <string_view>

namespace dusk::disc_probe {

// One-shot diagnostics for a disc image chosen through Android's file picker
// (a content:// URI). On a Quest 1 such a disc read back wrong (hash mismatch,
// then garbage archive headers) while the same file opened by path worked
// (2026-10-27). Logs everything about how the URI opens and compares reads
// through SDL (the path the game and the verifier use) with pread() on the
// same descriptor, so one log shows where the data goes wrong. Does nothing
// for a plain path. Never changes state; safe to call more than once.
void log_content_uri(std::string_view location, const char* reason) noexcept;

}  // namespace dusk::disc_probe
