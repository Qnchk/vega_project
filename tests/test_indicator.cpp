#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include <vector>

#include "indicator.hpp"

using namespace lwti;

TEST_CASE("indicator returns same size and initial flat signals") {
  LiquidityWeightedTrendIndicator indicator;
  std::vector<Candle> candles{
      {"t1", 100, 101, 99, 100, 1000},
      {"t2", 101, 102, 100, 101, 1000},
      {"t3", 102, 103, 101, 102, 1000},
  };

  auto result = indicator.compute(candles);
  REQUIRE(result.size() == candles.size());
  CHECK(result.front().signal == Signal::Flat);
}

TEST_CASE("uptrend generates long bias") {
  IndicatorConfig cfg;
  cfg.trend_period = 3;
  cfg.momentum_lookback = 2;
  cfg.volatility_window = 3;
  cfg.threshold = 0.0;

  LiquidityWeightedTrendIndicator indicator(cfg);
  std::vector<Candle> candles{
      {"t1", 100, 101, 99, 100, 1000}, {"t2", 101, 102, 100, 101, 1200},
      {"t3", 102, 103, 101, 102, 1400}, {"t4", 103, 104, 102, 103, 1500},
      {"t5", 104, 105, 103, 104, 1600}, {"t6", 105, 106, 104, 105, 1700}};

  auto result = indicator.compute(candles);
  REQUIRE(result.size() == candles.size());
  CHECK(result.back().signal == Signal::Long);
  CHECK(result.back().momentum > 0.0);
}

TEST_CASE("downtrend generates short bias") {
  IndicatorConfig cfg;
  cfg.trend_period = 3;
  cfg.momentum_lookback = 2;
  cfg.volatility_window = 3;
  cfg.threshold = 0.0;

  LiquidityWeightedTrendIndicator indicator(cfg);
  std::vector<Candle> candles{
      {"t1", 105, 106, 104, 105, 1700}, {"t2", 104, 105, 103, 104, 1600},
      {"t3", 103, 104, 102, 103, 1500}, {"t4", 102, 103, 101, 102, 1400},
      {"t5", 101, 102, 100, 101, 1300}, {"t6", 100, 101, 99, 100, 1200}};

  auto result = indicator.compute(candles);
  REQUIRE(result.size() == candles.size());
  CHECK(result.back().signal == Signal::Short);
  CHECK(result.back().momentum < 0.0);
}

TEST_CASE("high volume speeds trend adjustment") {
  IndicatorConfig cfg;
  cfg.trend_period = 3;
  cfg.momentum_lookback = 1;
  cfg.volatility_window = 3;
  cfg.threshold = 0.0;

  LiquidityWeightedTrendIndicator indicator(cfg);
  std::vector<Candle> low_vol{
      {"t1", 100, 100, 100, 100, 1},
      {"t2", 110, 110, 110, 110, 1},
      {"t3", 110, 110, 110, 110, 1},
  };
  std::vector<Candle> high_vol = low_vol;
  high_vol[1].volume = 50.0;  // spike volume on second bar

  auto base = indicator.compute(low_vol);
  auto boosted = indicator.compute(high_vol);

  REQUIRE(base.size() == boosted.size());
  // With larger volume, EMA should react more to the second bar.
  CHECK(boosted[1].lw_ema > base[1].lw_ema);
}
