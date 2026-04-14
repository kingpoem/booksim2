#ifndef CHIPLET_CLOCK_HPP
#define CHIPLET_CLOCK_HPP

#include <algorithm>

// Finest simulator tick t; period >= 1, arbitrary integer phase.
namespace chiplet_clock {

inline int PeriodMax1(int p) { return std::max(1, p); }

inline bool IsEdge(int t, int period, int phase) {
  int const P = PeriodMax1(period);
  return ((t + phase) % P) == 0;
}

// Smallest te >= t such that IsEdge(te, period, phase).
inline int NextEdge(int t, int period, int phase) {
  int const P = PeriodMax1(period);
  int const m = (t + phase) % P;
  if (m == 0) {
    return t;
  }
  return t + (P - m);
}

// Extra global ticks modeling multi-flop gray pointer synchronizer depth.
inline int GraySyncExtraDelay(int w_period, int r_period, int gray_stages) {
  if (gray_stages <= 0) {
    return 0;
  }
  int const Pw = PeriodMax1(w_period);
  int const Pr = PeriodMax1(r_period);
  return gray_stages * std::max(Pw, Pr);
}

}  // namespace chiplet_clock

#endif
