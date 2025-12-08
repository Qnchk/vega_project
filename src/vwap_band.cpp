#include "indicators/vwap_band.hpp"

#include <algorithm>
#include <cmath>
#include <deque>

namespace lwti {
namespace {

std::size_t clamp_period(std::size_t value) { return std::max<std::size_t>(1, value); }

}  // namespace

VwapBandIndicator::VwapBandIndicator(VwapBandConfig config) : config_(config) {
  config_.window = clamp_period(config_.window);
  config_.band_deviation = std::max(0.1, config_.band_deviation);
}

std::vector<VwapBandPoint> VwapBandIndicator::compute(const std::vector<Candle>& candles) const {
  if (candles.empty()) return {};

  std::deque<double> pv_window;
  std::deque<double> v_window;
  std::deque<double> price_window;

  double pv_sum = 0.0;
  double v_sum = 0.0;
  double price_sum = 0.0;
  double price_sq_sum = 0.0;

  std::vector<VwapBandPoint> out;
  out.reserve(candles.size());

  for (std::size_t i = 0; i < candles.size(); ++i) {
    const auto& c = candles[i];
    const double price = c.close;
    const double pv = price * c.volume;

    pv_window.push_back(pv);
    v_window.push_back(c.volume);
    price_window.push_back(price);

    pv_sum += pv;
    v_sum += c.volume;
    price_sum += price;
    price_sq_sum += price * price;

    if (pv_window.size() > config_.window) {
      pv_sum -= pv_window.front();
      v_sum -= v_window.front();
      price_sum -= price_window.front();
      price_sq_sum -= price_window.front() * price_window.front();
      pv_window.pop_front();
      v_window.pop_front();
      price_window.pop_front();
    }

    double vwap = v_sum > 0.0 ? pv_sum / v_sum : price;
    const double mean = price_sum / static_cast<double>(price_window.size());
    double variance =
        price_sq_sum / static_cast<double>(price_window.size()) - mean * mean;
    if (variance < 0.0) variance = 0.0;
    const double stddev = std::sqrt(variance);
    const double offset = stddev * config_.band_deviation;
    const double upper = vwap + offset;
    const double lower = vwap - offset;

    Signal signal = Signal::Flat;
    if (price < lower) {
      signal = Signal::Long;
    } else if (price > upper) {
      signal = Signal::Short;
    }

    out.push_back({i, c.timestamp, vwap, upper, lower, signal});
  }

  return out;
}

}  // namespace lwti
