#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"

namespace lwti {

// Parses CSV with header: timestamp,open,high,low,close,volume.
std::vector<Candle> read_candles_csv(const std::string& path);

}  // namespace lwti
