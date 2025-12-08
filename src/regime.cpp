#include "indicators/regime.hpp"

#include <algorithm>
#include <cmath>
#include <deque>

namespace lwti {
namespace {

std::size_t clamp_period(std::size_t value) { return std::max<std::size_t>(1, value); }

}  // namespace

VolatilityRegimeIndicator::VolatilityRegimeIndicator(RegimeConfig config) : config_(config) {
  config_.window = clamp_period(config_.window);
  config_.high_vol_threshold = std::max(0.0, config_.high_vol_threshold);
}

std::vector<RegimePoint> VolatilityRegimeIndicator::compute(
    const std::vector<Candle>& candles) const {
  if (candles.empty()) return {};

  std::deque<double> returns;
  double sum = 0.0;
  double sq_sum = 0.0;

  std::vector<RegimePoint> out;
  out.reserve(candles.size());

  double prev_close = candles.front().close;
  for (std::size_t i = 0; i < candles.size(); ++i) {
    const auto& c = candles[i];
    if (i > 0) {
      const double ret = (c.close - prev_close) / prev_close;
      returns.push_back(ret);
      sum += ret;
      sq_sum += ret * ret;
      if (returns.size() > config_.window) {
        const double removed = returns.front();
        returns.pop_front();
        sum -= removed;
        sq_sum -= removed * removed;
      }
      prev_close = c.close;
    }

    double variance = 0.0;
    if (!returns.empty()) {
      const double mean = sum / static_cast<double>(returns.size());
      variance = sq_sum / static_cast<double>(returns.size()) - mean * mean;
      if (variance < 0.0) variance = 0.0;
    }
    const double vol = std::sqrt(variance);

    VolatilityRegime regime = vol > config_.high_vol_threshold ? VolatilityRegime::High
                                                               : VolatilityRegime::Low;
    Signal signal = regime == VolatilityRegime::High ? Signal::Flat : Signal::Long;

    out.push_back({i, c.timestamp, vol, regime, signal});
  }

  return out;
}

}  // namespace lwti
