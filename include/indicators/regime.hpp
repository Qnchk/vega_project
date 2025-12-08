#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"

namespace lwti {

enum class VolatilityRegime { Low, High };

struct RegimeConfig {
  std::size_t window{30};
  double high_vol_threshold{0.02};  // daily return std threshold
};

struct RegimePoint {
  std::size_t index{};
  std::string timestamp;
  double realized_vol{0.0};
  VolatilityRegime regime{VolatilityRegime::Low};
  Signal signal{Signal::Flat};  // flat when high volatility, else neutral long
};

class VolatilityRegimeIndicator {
 public:
  explicit VolatilityRegimeIndicator(RegimeConfig config = {});
  std::vector<RegimePoint> compute(const std::vector<Candle>& candles) const;
  const RegimeConfig& config() const { return config_; }

 private:
  RegimeConfig config_;
};

}  // namespace lwti
