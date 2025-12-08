#include "config/run_config.hpp"

#include <fstream>
#include <iostream>

#include "nlohmann/json.hpp"

namespace lwti {

namespace {

template <typename T>
void set_if_exists(const nlohmann::json& j, const char* key, T& field) {
  if (j.contains(key)) {
    field = j.at(key).get<T>();
  }
}

}  // namespace

std::optional<RunConfig> load_run_config(const std::string& path) {
  std::ifstream input(path);
  if (!input.is_open()) {
    std::cerr << "Cannot open config: " << path << "\n";
    return std::nullopt;
  }

  nlohmann::json j;
  try {
    input >> j;
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse JSON: " << e.what() << "\n";
    return std::nullopt;
  }

  RunConfig cfg;

  if (j.contains("data")) {
    const auto& jd = j["data"];
    set_if_exists(jd, "input_path", cfg.data.input_path);
  }

  if (j.contains("lwti")) {
    const auto& jl = j["lwti"];
    set_if_exists(jl, "trend_period", cfg.lwti.trend_period);
    set_if_exists(jl, "momentum_lookback", cfg.lwti.momentum_lookback);
    set_if_exists(jl, "volatility_window", cfg.lwti.volatility_window);
    set_if_exists(jl, "threshold", cfg.lwti.threshold);
    set_if_exists(jl, "volume_floor", cfg.lwti.volume_floor);
  }

  if (j.contains("vwap")) {
    const auto& jw = j["vwap"];
    set_if_exists(jw, "window", cfg.vwap.window);
    set_if_exists(jw, "band_deviation", cfg.vwap.band_deviation);
  }

  if (j.contains("regime")) {
    const auto& jr = j["regime"];
    set_if_exists(jr, "window", cfg.regime.window);
    set_if_exists(jr, "high_vol_threshold", cfg.regime.high_vol_threshold);
  }

  if (j.contains("strategy")) {
    const auto& js = j["strategy"];
    set_if_exists(js, "lwti_weight", cfg.strategy.lwti_weight);
    set_if_exists(js, "vwap_weight", cfg.strategy.vwap_weight);
    set_if_exists(js, "max_position", cfg.strategy.max_position);
  }

  if (j.contains("backtest")) {
    const auto& jb = j["backtest"];
    set_if_exists(jb, "starting_equity", cfg.backtest.starting_equity);
    set_if_exists(jb, "risk_per_trade", cfg.backtest.risk_per_trade);
    set_if_exists(jb, "fee_bps", cfg.backtest.fee_bps);
    set_if_exists(jb, "slippage_bps", cfg.backtest.slippage_bps);
  }

  return cfg;
}

}  // namespace lwti
