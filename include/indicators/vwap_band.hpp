#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"

namespace lwti {

struct VwapBandConfig {
  std::size_t window{20};
  double band_deviation{1.5};  // standard deviations for bands
};

struct VwapBandPoint {
  std::size_t index{};
  std::string timestamp;
  double vwap{0.0};
  double upper{0.0};
  double lower{0.0};
  Signal signal{Signal::Flat};
};

class VwapBandIndicator {
 public:
  explicit VwapBandIndicator(VwapBandConfig config = {});
  std::vector<VwapBandPoint> compute(const std::vector<Candle>& candles) const;
  const VwapBandConfig& config() const { return config_; }

 private:
  VwapBandConfig config_;
};

}  // namespace lwti
