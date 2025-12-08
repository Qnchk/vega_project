#include "core/types.hpp"

namespace lwti {

std::string_view signal_to_string(Signal signal) {
  switch (signal) {
    case Signal::Long:
      return "long";
    case Signal::Short:
      return "short";
    case Signal::Flat:
    default:
      return "flat";
  }
}

int signal_polarity(Signal signal) {
  switch (signal) {
    case Signal::Long:
      return 1;
    case Signal::Short:
      return -1;
    case Signal::Flat:
    default:
      return 0;
  }
}

}  // namespace lwti
