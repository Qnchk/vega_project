#include "csv_reader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace lwti {
namespace {

bool parse_double(const std::string& text, double& out) {
  try {
    size_t idx = 0;
    out = std::stod(text, &idx);
    return idx == text.size();
  } catch (...) {
    return false;
  }
}

std::vector<std::string> split(const std::string& line, char delim) {
  std::vector<std::string> tokens;
  std::stringstream ss(line);
  std::string item;
  while (std::getline(ss, item, delim)) {
    tokens.push_back(item);
  }
  return tokens;
}

void trim(std::string& s) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

}  // namespace

std::vector<Candle> read_candles_csv(const std::string& path) {
  std::ifstream input(path);
  std::vector<Candle> candles;
  if (!input.is_open()) {
    return candles;
  }

  std::string line;
  bool first_line = true;
  while (std::getline(input, line)) {
    if (line.empty()) {
      continue;
    }
    auto tokens = split(line, ',');
    if (tokens.size() < 6) {
      continue;
    }
    for (auto& token : tokens) {
      trim(token);
    }
    if (first_line) {
      first_line = false;
      // Skip header if present.
      if (!tokens.empty() && tokens[1] == "open") {
        continue;
      }
    }
    double open = 0.0, high = 0.0, low = 0.0, close = 0.0, volume = 0.0;
    if (!parse_double(tokens[1], open) || !parse_double(tokens[2], high) ||
        !parse_double(tokens[3], low) || !parse_double(tokens[4], close) ||
        !parse_double(tokens[5], volume)) {
      continue;
    }
    candles.push_back({tokens[0], open, high, low, close, volume});
  }

  return candles;
}

}  // namespace lwti
