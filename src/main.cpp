#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "csv_reader.hpp"
#include "indicator.hpp"

namespace {

struct CliOptions {
  std::string input_path;
  std::optional<std::string> output_path;
  lwti::IndicatorConfig config;
};

void print_usage(std::string_view exec) {
  std::cerr << "Usage: " << exec
            << " --input <file> [--output <file|stdout>] [--trend-period N]"
               " [--momentum-lookback N] [--volatility-window N]"
               " [--threshold X] [--volume-floor X]\n";
}

std::optional<CliOptions> parse_args(int argc, char* argv[]) {
  CliOptions opts;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    auto next = [&]() -> std::optional<std::string> {
      if (i + 1 >= argc) return std::nullopt;
      return std::string(argv[++i]);
    };

    if (arg == "--input") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.input_path = *value;
    } else if (arg == "--output") {
      auto value = next();
      if (!value) return std::nullopt;
      if (*value != "stdout") {
        opts.output_path = *value;
      }
    } else if (arg == "--trend-period") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.config.trend_period = std::stoul(*value);
    } else if (arg == "--momentum-lookback") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.config.momentum_lookback = std::stoul(*value);
    } else if (arg == "--volatility-window") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.config.volatility_window = std::stoul(*value);
    } else if (arg == "--threshold") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.config.threshold = std::stod(*value);
    } else if (arg == "--volume-floor") {
      auto value = next();
      if (!value) return std::nullopt;
      opts.config.volume_floor = std::stod(*value);
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return std::nullopt;
    } else {
      return std::nullopt;
    }
  }
  if (opts.input_path.empty()) {
    return std::nullopt;
  }
  return opts;
}

std::ostream& prepare_output(const std::optional<std::string>& path,
                             std::ofstream& owned_stream) {
  if (path.has_value()) {
    owned_stream.open(*path);
    if (owned_stream.is_open()) {
      return owned_stream;
    }
    std::cerr << "Failed to open output file: " << *path << "\n";
  }
  return std::cout;
}

}  // namespace

int main(int argc, char* argv[]) {
  auto parsed = parse_args(argc, argv);
  if (!parsed) {
    print_usage(argv[0]);
    return 1;
  }

  const auto candles = lwti::read_candles_csv(parsed->input_path);
  if (candles.empty()) {
    std::cerr << "No candles loaded from " << parsed->input_path << "\n";
    return 1;
  }

  lwti::LiquidityWeightedTrendIndicator indicator(parsed->config);
  const auto points = indicator.compute(candles);

  std::ofstream out_file;
  std::ostream& out = prepare_output(parsed->output_path, out_file);
  out << std::fixed << std::setprecision(6);
  out << "timestamp,close,lw_ema,momentum,volatility,signal\n";
  for (std::size_t i = 0; i < points.size(); ++i) {
    const auto& c = candles[i];
    const auto& p = points[i];
    out << c.timestamp << ',' << c.close << ',' << p.lw_ema << ',' << p.momentum << ','
        << p.volatility << ',' << lwti::signal_to_string(p.signal) << '\n';
  }

  if (!points.empty()) {
    const auto& last = points.back();
    std::cout << "# last: lw_ema=" << last.lw_ema << " momentum=" << last.momentum
              << " volatility=" << last.volatility
              << " signal=" << lwti::signal_to_string(last.signal) << "\n";
  }

  return 0;
}
