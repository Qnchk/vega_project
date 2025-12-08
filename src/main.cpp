#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "backtest/backtester.hpp"
#include "config/run_config.hpp"
#include "csv_reader.hpp"
#include "indicator.hpp"
#include "indicators/regime.hpp"
#include "indicators/vwap_band.hpp"
#include "strategy/composite_strategy.hpp"

namespace {

struct CliOptions {
  std::optional<std::string> config_path;
  std::optional<std::string> export_signals;
  std::optional<std::string> report_path;
  lwti::RunConfig fallback;
};

void print_usage(std::string_view exec) {
  std::cerr << "Usage: " << exec << " [--config <file>]"
            << " [--input <file>] [--export-signals <file>] [--report <file>]\n"
            << "Optional overrides: --trend-period N --momentum-lookback N"
            << " --volatility-window N --threshold X --volume-floor X"
            << " --vwap-window N --vwap-band-dev X --regime-window N --high-vol-threshold X"
            << " --lwti-weight X --vwap-weight X --max-position X"
            << " --risk-per-trade X --fee-bps X --slippage-bps X\n";
}

std::optional<CliOptions> parse_args(int argc, char* argv[]) {
  CliOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    auto next = [&]() -> std::optional<std::string> {
      if (i + 1 >= argc) return std::nullopt;
      return std::string(argv[++i]);
    };

    if (arg == "--config") {
      opts.config_path = next();
      if (!opts.config_path) return std::nullopt;
    } else if (arg == "--input") {
      opts.fallback.data.input_path = next().value_or("");
    } else if (arg == "--export-signals") {
      opts.export_signals = next();
      if (!opts.export_signals) return std::nullopt;
    } else if (arg == "--report") {
      opts.report_path = next();
      if (!opts.report_path) return std::nullopt;
    } else if (arg == "--trend-period") {
      opts.fallback.lwti.trend_period = std::stoul(next().value_or("0"));
    } else if (arg == "--momentum-lookback") {
      opts.fallback.lwti.momentum_lookback = std::stoul(next().value_or("0"));
    } else if (arg == "--volatility-window") {
      opts.fallback.lwti.volatility_window = std::stoul(next().value_or("0"));
    } else if (arg == "--threshold") {
      opts.fallback.lwti.threshold = std::stod(next().value_or("0"));
    } else if (arg == "--volume-floor") {
      opts.fallback.lwti.volume_floor = std::stod(next().value_or("0"));
    } else if (arg == "--vwap-window") {
      opts.fallback.vwap.window = std::stoul(next().value_or("0"));
    } else if (arg == "--vwap-band-dev") {
      opts.fallback.vwap.band_deviation = std::stod(next().value_or("0"));
    } else if (arg == "--regime-window") {
      opts.fallback.regime.window = std::stoul(next().value_or("0"));
    } else if (arg == "--high-vol-threshold") {
      opts.fallback.regime.high_vol_threshold = std::stod(next().value_or("0"));
    } else if (arg == "--lwti-weight") {
      opts.fallback.strategy.lwti_weight = std::stod(next().value_or("0"));
    } else if (arg == "--vwap-weight") {
      opts.fallback.strategy.vwap_weight = std::stod(next().value_or("0"));
    } else if (arg == "--max-position") {
      opts.fallback.strategy.max_position = std::stod(next().value_or("0"));
    } else if (arg == "--risk-per-trade") {
      opts.fallback.backtest.risk_per_trade = std::stod(next().value_or("0"));
    } else if (arg == "--fee-bps") {
      opts.fallback.backtest.fee_bps = std::stod(next().value_or("0"));
    } else if (arg == "--slippage-bps") {
      opts.fallback.backtest.slippage_bps = std::stod(next().value_or("0"));
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return std::nullopt;
    } else {
      return std::nullopt;
    }
  }

  if (!opts.config_path && opts.fallback.data.input_path.empty()) {
    return std::nullopt;
  }

  return opts;
}

std::ostream& prepare_output(const std::optional<std::string>& path,
                             std::ofstream& owned_stream) {
  if (path.has_value() && *path != "stdout") {
    owned_stream.open(*path);
    if (owned_stream.is_open()) {
      return owned_stream;
    }
    std::cerr << "Failed to open output file: " << *path << "\n";
  }
  return std::cout;
}

void write_signals(const std::vector<lwti::Candle>& candles,
                   const std::vector<lwti::IndicatorPoint>& lwti_points,
                   const std::vector<lwti::VwapBandPoint>& vwap_points,
                   const std::vector<lwti::RegimePoint>& regime_points,
                   const std::vector<lwti::StrategyPoint>& strat_points,
                   const std::optional<std::string>& path) {
  if (!path) {
    return;
  }
  std::ofstream out_file;
  std::ostream& out = prepare_output(path, out_file);
  const std::size_t n =
      std::min({candles.size(), lwti_points.size(), vwap_points.size(), strat_points.size(),
                regime_points.size()});

  out << std::fixed << std::setprecision(6);
  out << "timestamp,close,lwti_momentum,lwti_signal,vwap,upper,lower,vwap_signal,"
         "regime_vol,strategy_score,strategy_signal\n";
  for (std::size_t i = 0; i < n; ++i) {
    out << candles[i].timestamp << ',' << candles[i].close << ',' << lwti_points[i].momentum
        << ',' << lwti::signal_to_string(lwti_points[i].signal) << ','
        << vwap_points[i].vwap << ',' << vwap_points[i].upper << ',' << vwap_points[i].lower
        << ',' << lwti::signal_to_string(vwap_points[i].signal) << ','
        << regime_points[i].realized_vol << ',' << strat_points[i].score << ','
        << lwti::signal_to_string(strat_points[i].signal) << '\n';
  }
}

void write_report(const lwti::BacktestResult& result, const std::optional<std::string>& path) {
  if (!path) {
    return;
  }
  std::ofstream out_file;
  std::ostream& out = prepare_output(path, out_file);
  out << std::fixed << std::setprecision(4);
  out << "starting_equity=" << result.starting_equity << "\n";
  out << "ending_equity=" << result.ending_equity << "\n";
  const double ret = result.starting_equity > 0.0
                         ? (result.ending_equity - result.starting_equity) /
                               result.starting_equity
                         : 0.0;
  out << "return_pct=" << ret * 100.0 << "\n";
  out << "max_drawdown_pct=" << result.max_drawdown * 100.0 << "\n";
  out << "trades=" << result.trades << "\n";
  out << "win_rate_pct=" << result.win_rate * 100.0 << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
  auto parsed = parse_args(argc, argv);
  if (!parsed) {
    print_usage(argv[0]);
    return 1;
  }

  std::optional<lwti::RunConfig> cfg;
  if (parsed->config_path) {
    cfg = lwti::load_run_config(*parsed->config_path);
    if (!cfg) {
      return 1;
    }
  } else {
    cfg = parsed->fallback;
  }

  if (cfg->data.input_path.empty()) {
    std::cerr << "Input path is required via --config or --input\n";
    return 1;
  }

  const auto candles = lwti::read_candles_csv(cfg->data.input_path);
  if (candles.empty()) {
    std::cerr << "No candles loaded from " << cfg->data.input_path << "\n";
    return 1;
  }

  const auto lwti_points =
      lwti::LiquidityWeightedTrendIndicator(cfg->lwti).compute(candles);
  const auto vwap_points = lwti::VwapBandIndicator(cfg->vwap).compute(candles);
  const auto regime_points = lwti::VolatilityRegimeIndicator(cfg->regime).compute(candles);
  const auto strat_points =
      lwti::CompositeStrategy(cfg->strategy).generate(lwti_points, vwap_points, regime_points);
  const auto backtest = lwti::Backtester(cfg->backtest).run(candles, strat_points);

  write_signals(candles, lwti_points, vwap_points, regime_points, strat_points,
                parsed->export_signals);
  write_report(backtest, parsed->report_path);

  std::cout << "# Backtest ending equity: " << backtest.ending_equity
            << " | trades=" << backtest.trades
            << " | win_rate=" << backtest.win_rate * 100.0 << "%\n";
  return 0;
}
