#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch_amalgamated.hpp"

#include "backtest/backtester.hpp"
#include "indicators/regime.hpp"
#include "indicators/vwap_band.hpp"
#include "strategy/composite_strategy.hpp"

using namespace lwti;

TEST_CASE("vwap band emits short on breakout") {
  VwapBandConfig cfg;
  cfg.window = 3;
  cfg.band_deviation = 0.5;
  VwapBandIndicator ind(cfg);

  std::vector<Candle> candles{
      {"t1", 100, 101, 99, 100, 10},
      {"t2", 100, 101, 99, 100, 10},
      {"t3", 100, 101, 99, 100, 10},
      {"t4", 103, 104, 102, 103, 10},
  };

  auto out = ind.compute(candles);
  REQUIRE(out.size() == candles.size());
  CHECK(out.back().signal == Signal::Short);
}

TEST_CASE("composite strategy flattens under high volatility regime") {
  CompositeStrategy strat({.lwti_weight = 1.0, .vwap_weight = 1.0, .max_position = 1.0});

  std::vector<IndicatorPoint> lwti_points{
      {0, "t1", 0, 0, 0, Signal::Long},
      {1, "t2", 0, 0, 0, Signal::Long},
  };
  std::vector<VwapBandPoint> vwap_points{
      {0, "t1", 0, 0, 0, Signal::Long},
      {1, "t2", 0, 0, 0, Signal::Long},
  };
  std::vector<RegimePoint> regimes{
      {0, "t1", 0.0, VolatilityRegime::Low, Signal::Long},
      {1, "t2", 0.05, VolatilityRegime::High, Signal::Flat},
  };

  auto out = strat.generate(lwti_points, vwap_points, regimes);
  REQUIRE(out.size() == 2);
  CHECK(out[0].signal == Signal::Long);
  CHECK(out[0].position > 0.0);
  CHECK(out[1].signal == Signal::Flat);
  CHECK(out[1].position == 0.0);
}

TEST_CASE("backtester produces gains on rising market with long bias") {
  std::vector<Candle> candles{
      {"t1", 100, 101, 99, 100, 10},
      {"t2", 101, 102, 100, 101, 10},
      {"t3", 102, 103, 101, 102, 10},
  };

  std::vector<StrategyPoint> strategy{
      {0, "t1", 1.0, 1.0, Signal::Long},
      {1, "t2", 1.0, 1.0, Signal::Long},
      {2, "t3", 1.0, 1.0, Signal::Long},
  };

  BacktestConfig cfg;
  cfg.starting_equity = 1000.0;
  cfg.risk_per_trade = 1.0;
  cfg.fee_bps = 0.0;
  cfg.slippage_bps = 0.0;

  Backtester backtester(cfg);
  auto res = backtester.run(candles, strategy);

  CHECK(res.ending_equity > res.starting_equity);
  CHECK(res.trades >= 1);
}
