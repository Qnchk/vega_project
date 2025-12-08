#include "indicator.hpp"

#include <algorithm>
#include <cmath>
#include <deque>

namespace lwti {
namespace {

double typical_price(const Candle& c) {
  return (c.high + c.low + c.close) / 3.0;
}

std::size_t clamp_period(std::size_t value) { return std::max<std::size_t>(1, value); }

}  // namespace

std::string_view signal_to_string(Signal signal) {
  switch (signal) {
    case Signal::Long:
      return "long";
    case Signal::Short:
      return "short";
    case Signal::Flat:
    default:
      return "flat";
  }
}

LiquidityWeightedTrendIndicator::LiquidityWeightedTrendIndicator(IndicatorConfig config)
    : config_(config) {
  config_.trend_period = clamp_period(config_.trend_period);
  config_.momentum_lookback = clamp_period(config_.momentum_lookback);
  config_.volatility_window = clamp_period(config_.volatility_window);
  config_.threshold = std::max(0.0, config_.threshold);
  config_.volume_floor = std::max(0.1, config_.volume_floor);
}

std::vector<IndicatorPoint> LiquidityWeightedTrendIndicator::compute(
    const std::vector<Candle>& candles) const {
  if (candles.empty()) {
    return {};
  }

  const double alpha = 2.0 / (static_cast<double>(config_.trend_period) + 1.0);
  std::vector<IndicatorPoint> result;
  result.reserve(candles.size());

  std::deque<double> volume_window;
  double volume_sum = 0.0;

  std::deque<double> return_window;
  double ret_sum = 0.0;
  double ret_sq_sum = 0.0;

  std::vector<double> lw_history;
  lw_history.reserve(candles.size());

  double prev_tp = typical_price(candles.front());
  double lw_ema = typical_price(candles.front());

  for (std::size_t i = 0; i < candles.size(); ++i) {
    const Candle& c = candles[i];
    const double tp = typical_price(c);

    // Maintain rolling volume stats.
    volume_window.push_back(c.volume);
    volume_sum += c.volume;
    if (volume_window.size() > config_.trend_period) {
      volume_sum -= volume_window.front();
      volume_window.pop_front();
    }
    const double avg_volume =
        volume_window.empty() ? 0.0 : volume_sum / static_cast<double>(volume_window.size());
    double weight = config_.volume_floor;
    if (avg_volume > 0.0) {
      weight = std::max(config_.volume_floor, c.volume / avg_volume);
    }

    // Trend smoothing with volume weight.
    const double effective_alpha = std::min(1.0, alpha * weight);
    if (i == 0) {
      lw_ema = tp;
    } else {
      lw_ema = effective_alpha * tp + (1.0 - effective_alpha) * lw_ema;
    }
    lw_history.push_back(lw_ema);

    // Momentum relative to past smoothed price.
    double momentum = 0.0;
    if (i >= config_.momentum_lookback) {
      const double base = lw_history[i - config_.momentum_lookback];
      if (std::abs(base) > 1e-9) {
        momentum = (lw_ema - base) / base;
      } else {
        momentum = lw_ema - base;
      }
    }

    // Rolling volatility on simple returns of typical price.
    if (i > 0) {
      double ret = 0.0;
      if (std::abs(prev_tp) > 1e-9) {
        ret = (tp - prev_tp) / prev_tp;
      }
      return_window.push_back(ret);
      ret_sum += ret;
      ret_sq_sum += ret * ret;
      if (return_window.size() > config_.volatility_window) {
        const double removed = return_window.front();
        return_window.pop_front();
        ret_sum -= removed;
        ret_sq_sum -= removed * removed;
      }
    }
    prev_tp = tp;
    double variance = 0.0;
    if (!return_window.empty()) {
      const double mean = ret_sum / static_cast<double>(return_window.size());
      variance = ret_sq_sum / static_cast<double>(return_window.size()) - mean * mean;
      if (variance < 0.0) {
        variance = 0.0;
      }
    }
    const double volatility = std::sqrt(variance);

    // Signal gating by volatility.
    double gate = volatility * config_.threshold;
    if (gate < 1e-8) {
      gate = config_.threshold * 1e-4;
    }

    Signal signal = Signal::Flat;
    if (momentum > gate) {
      signal = Signal::Long;
    } else if (momentum < -gate) {
      signal = Signal::Short;
    }

    result.push_back(
        {i, c.timestamp, lw_ema, momentum, volatility, signal});
  }

  return result;
}

}  // namespace lwti
