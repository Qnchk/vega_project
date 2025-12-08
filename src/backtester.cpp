#include "backtest/backtester.hpp"

#include <algorithm>
#include <cmath>

namespace lwti {

Backtester::Backtester(BacktestConfig config) : config_(config) {
  config_.starting_equity = std::max(1000.0, config_.starting_equity);
  config_.risk_per_trade = std::clamp(config_.risk_per_trade, 0.0, 1.0);
  config_.fee_bps = std::max(0.0, config_.fee_bps);
  config_.slippage_bps = std::max(0.0, config_.slippage_bps);
}

BacktestResult Backtester::run(const std::vector<Candle>& candles,
                               const std::vector<StrategyPoint>& strategy) const {
  const std::size_t n = std::min(candles.size(), strategy.size());
  if (n < 2) {
    return {config_.starting_equity, config_.starting_equity, 0.0, 0, 0.0, {}};
  }

  double equity = config_.starting_equity;
  double peak = equity;
  double max_drawdown = 0.0;
  double position_qty = 0.0;  // number of units
  double trade_entry_equity = equity;
  Signal trade_signal = Signal::Flat;
  std::vector<Trade> log;
  std::size_t wins = 0;

  double prev_close = candles.front().close;

  for (std::size_t i = 1; i < n; ++i) {
    const double price = candles[i].close;
    const double price_change = price - prev_close;
    equity += position_qty * price_change;
    prev_close = price;

    peak = std::max(peak, equity);
    if (peak > 0.0) {
      max_drawdown = std::max(max_drawdown, (peak - equity) / peak);
    }

    const auto& s = strategy[i];
    double target_value = equity * config_.risk_per_trade * s.position;
    double target_qty = price != 0.0 ? target_value / price : 0.0;

    if (std::isnan(target_qty) || std::isinf(target_qty)) {
      target_qty = 0.0;
    }

    const double delta_qty = target_qty - position_qty;
    if (std::abs(delta_qty) > 1e-9) {
      const double trade_notional = std::abs(delta_qty) * price;
      const double cost =
          trade_notional * (config_.fee_bps + config_.slippage_bps) / 10000.0;
      equity -= cost;
    }

    const bool closing = position_qty != 0.0 &&
                         (target_qty == 0.0 || (position_qty * target_qty < 0.0));
    if (closing) {
      const double trade_pnl = equity - trade_entry_equity;
      log.push_back({candles[i].timestamp, trade_signal, price, position_qty, trade_pnl});
      if (trade_pnl > 0.0) {
        ++wins;
      }
    }

    const bool opening = target_qty != 0.0 &&
                         (position_qty == 0.0 || (position_qty * target_qty < 0.0));
    if (opening) {
      trade_entry_equity = equity;
      trade_signal = s.signal;
    }

    position_qty = target_qty;
  }

  if (position_qty != 0.0) {
    const double trade_pnl = equity - trade_entry_equity;
    log.push_back({candles[n - 1].timestamp, trade_signal, candles[n - 1].close,
                   position_qty, trade_pnl});
    if (trade_pnl > 0.0) {
      ++wins;
    }
  }

  const std::size_t trades = log.size();
  const double win_rate = trades > 0 ? static_cast<double>(wins) / trades : 0.0;

  return {config_.starting_equity, equity, max_drawdown, trades, win_rate, log};
}

}  // namespace lwti
