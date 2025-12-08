#include "strategy/composite_strategy.hpp"

#include <algorithm>
#include <cmath>

namespace lwti {

CompositeStrategy::CompositeStrategy(CompositeStrategyConfig config) : config_(config) {
  config_.max_position = std::clamp(config_.max_position, 0.0, 5.0);
  config_.lwti_weight = std::max(0.0, config_.lwti_weight);
  config_.vwap_weight = std::max(0.0, config_.vwap_weight);
}

std::vector<StrategyPoint> CompositeStrategy::generate(
    const std::vector<IndicatorPoint>& lwti_points,
    const std::vector<VwapBandPoint>& vwap_points,
    const std::vector<RegimePoint>& regimes) const {
  const std::size_t n =
      std::min({lwti_points.size(), vwap_points.size(), regimes.size()});
  std::vector<StrategyPoint> out;
  out.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    const auto& l = lwti_points[i];
    const auto& v = vwap_points[i];
    const auto& r = regimes[i];

    double score = 0.0;
    score += config_.lwti_weight * static_cast<double>(signal_polarity(l.signal));
    score += config_.vwap_weight * static_cast<double>(signal_polarity(v.signal));

    if (r.regime == VolatilityRegime::High) {
      score = 0.0;  // risk-off during high volatility
    }

    Signal signal = Signal::Flat;
    if (score > 1e-6) {
      signal = Signal::Long;
    } else if (score < -1e-6) {
      signal = Signal::Short;
    }

    double position = 0.0;
    if (signal == Signal::Long) {
      position = config_.max_position;
    } else if (signal == Signal::Short) {
      position = -config_.max_position;
    }

    out.push_back({i, l.timestamp, score, position, signal});
  }

  return out;
}

}  // namespace lwti
