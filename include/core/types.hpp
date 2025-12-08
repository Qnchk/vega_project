#pragma once

#include <string>
#include <string_view>

namespace lwti {

struct Candle {
  std::string timestamp;
  double open{0.0};
  double high{0.0};
  double low{0.0};
  double close{0.0};
  double volume{0.0};
};

enum class Signal { Long, Short, Flat };

std::string_view signal_to_string(Signal signal);
int signal_polarity(Signal signal);

}  // namespace lwti
