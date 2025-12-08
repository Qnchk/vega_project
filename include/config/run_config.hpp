#pragma once

#include <optional>
#include <string>

#include "backtest/backtester.hpp"
#include "indicator.hpp"
#include "indicators/regime.hpp"
#include "indicators/vwap_band.hpp"
#include "strategy/composite_strategy.hpp"

namespace lwti {

struct DataConfig {
  std::string input_path;
};

struct RunConfig {
  DataConfig data;
  IndicatorConfig lwti{};
  VwapBandConfig vwap{};
  RegimeConfig regime{};
  CompositeStrategyConfig strategy{};
  BacktestConfig backtest{};
};

std::optional<RunConfig> load_run_config(const std::string& path);

}  // namespace lwti
