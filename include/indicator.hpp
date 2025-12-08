#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"

namespace lwti {

struct IndicatorConfig {
  std::size_t trend_period{14};
  std::size_t momentum_lookback{5};
  std::size_t volatility_window{10};
  double threshold{0.7};     // multiplier for volatility gating
  double volume_floor{1.0};  // minimum weight for low-liquidity bars
};

struct IndicatorPoint {
  std::size_t index{};
  std::string timestamp;
  double lw_ema{0.0};
  double momentum{0.0};
  double volatility{0.0};
  Signal signal{Signal::Flat};
};

class LiquidityWeightedTrendIndicator {
 public:
  explicit LiquidityWeightedTrendIndicator(IndicatorConfig config = {});
  std::vector<IndicatorPoint> compute(const std::vector<Candle>& candles) const;
  const IndicatorConfig& config() const { return config_; }

 private:
  IndicatorConfig config_;
};

}  // namespace lwti
