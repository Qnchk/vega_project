#pragma once

#include <string>
#include <vector>

#include "core/types.hpp"
#include "strategy/composite_strategy.hpp"

namespace lwti {

struct BacktestConfig {
  double starting_equity{100000.0};
  double risk_per_trade{0.02};    // fraction of equity to allocate per signal
  double fee_bps{1.0};            // commission in basis points per trade
  double slippage_bps{1.0};       // execution slippage in basis points
};

struct Trade {
  std::string timestamp;
  Signal signal{Signal::Flat};
  double price{0.0};
  double quantity{0.0};
  double pnl{0.0};
};

struct BacktestResult {
  double starting_equity{0.0};
  double ending_equity{0.0};
  double max_drawdown{0.0};
  std::size_t trades{0};
  double win_rate{0.0};
  std::vector<Trade> trade_log;
};

class Backtester {
 public:
  explicit Backtester(BacktestConfig config = {});
  BacktestResult run(const std::vector<Candle>& candles,
                     const std::vector<StrategyPoint>& strategy) const;

 private:
  BacktestConfig config_;
};

}  // namespace lwti
