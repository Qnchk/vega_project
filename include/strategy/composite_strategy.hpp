#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"
#include "indicator.hpp"
#include "indicators/regime.hpp"
#include "indicators/vwap_band.hpp"

namespace lwti {

struct CompositeStrategyConfig {
  double lwti_weight{0.5};
  double vwap_weight{0.5};
  double max_position{1.0};  // fraction of equity
};

struct StrategyPoint {
  std::size_t index{};
  std::string timestamp;
  double score{0.0};
  double position{0.0};
  Signal signal{Signal::Flat};
};

class CompositeStrategy {
 public:
  explicit CompositeStrategy(CompositeStrategyConfig config = {});
  std::vector<StrategyPoint> generate(const std::vector<IndicatorPoint>& lwti_points,
                                      const std::vector<VwapBandPoint>& vwap_points,
                                      const std::vector<RegimePoint>& regimes) const;

 private:
  CompositeStrategyConfig config_;
};

}  // namespace lwti
