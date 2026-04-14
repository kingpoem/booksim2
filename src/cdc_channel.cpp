#include "booksim.hpp"

#include "cdc_channel.hpp"

#include <algorithm>
#include <cassert>

#include "chiplet_clock.hpp"
#include "flit.hpp"

using namespace std;

CdcFlitChannel::CdcFlitChannel(Module *parent, string const &name, int classes,
                               int w_period, int w_phase, int r_period,
                               int r_phase, int sync_cycles, int fifo_depth,
                               int wire_delay)
    : FlitChannel(parent, name, classes),
      _w_period(chiplet_clock::PeriodMax1(w_period)), _w_phase(w_phase),
      _r_period(chiplet_clock::PeriodMax1(r_period)), _r_phase(r_phase),
      _sync_cycles(max(0, sync_cycles)), _fifo_depth(max(1, fifo_depth)),
      _wire_delay(max(1, wire_delay)) {}

bool CdcFlitChannel::_WriterEdge(int t) const {
  return chiplet_clock::IsEdge(t, _w_period, _w_phase);
}

bool CdcFlitChannel::_ReaderEdge(int t) const {
  return chiplet_clock::IsEdge(t, _r_period, _r_phase);
}

int CdcFlitChannel::_Occupancy() const {
  return (int)_ingress.size() + (int)_await_sync.size() +
         (int)_await_delivery.size() + (int)_wire_q.size();
}

void CdcFlitChannel::Send(Flit *f) {
  if (f) {
    ++_active[f->cl];
    _ingress.push_back(f);
  } else {
    ++_idle;
  }
}

void CdcFlitChannel::ReadInputs() {
  int const T = GetSimTime();

  while (!_await_sync.empty()) {
    int const tin = _await_sync.front().first;
    if (T < tin + _sync_cycles) {
      break;
    }
    Flit *f = _await_sync.front().second;
    _await_sync.pop();
    int const t_done = tin + _sync_cycles;
    int const el = chiplet_clock::NextEdge(t_done, _r_period, _r_phase);
    _await_delivery.push_back(make_pair(el, f));
  }

  if (_WriterEdge(T) && !_ingress.empty()) {
    if (_Occupancy() >= _fifo_depth) {
      Error(string("CdcFlitChannel ") + FullName() +
            ": async FIFO overflow (increase chiplet_cdc_fifo_depth).");
    }
    Flit *f = _ingress.front();
    _ingress.pop_front();
    if (f && f->watch) {
      *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                 << "CDC writer latched flit " << f->id << "." << endl;
    }
    _await_sync.push(make_pair(T, f));
  }
}

void CdcFlitChannel::WriteOutputs() {
  int const T = GetSimTime();
  _output = 0;

  if (!_wire_q.empty()) {
    int const tr = _wire_q.front().first;
    if (T < tr) {
      return;
    }
    assert(T == tr);
    _output = _wire_q.front().second;
    _wire_q.pop();
    if (_output && _output->watch) {
      *gWatchOut << GetSimTime() << " | " << FullName() << " | "
                 << "CDC completed traversal for flit " << _output->id << "."
                 << endl;
    }
    return;
  }

  if (!_ReaderEdge(T)) {
    return;
  }
  if (_await_delivery.empty() || _await_delivery.front().first > T) {
    return;
  }
  Flit *f = _await_delivery.front().second;
  _await_delivery.pop_front();
  _wire_q.push(make_pair(T + _wire_delay - 1, f));
}

// --- Credit CDC (same timing model) ---

CdcCreditChannel::CdcCreditChannel(Module *parent, string const &name,
                                   int w_period, int w_phase, int r_period,
                                   int r_phase, int sync_cycles, int fifo_depth,
                                   int wire_delay)
    : Channel<Credit>(parent, name),
      _w_period(chiplet_clock::PeriodMax1(w_period)), _w_phase(w_phase),
      _r_period(chiplet_clock::PeriodMax1(r_period)), _r_phase(r_phase),
      _sync_cycles(max(0, sync_cycles)), _fifo_depth(max(1, fifo_depth)),
      _wire_delay(max(1, wire_delay)) {}

bool CdcCreditChannel::_WriterEdge(int t) const {
  return chiplet_clock::IsEdge(t, _w_period, _w_phase);
}

bool CdcCreditChannel::_ReaderEdge(int t) const {
  return chiplet_clock::IsEdge(t, _r_period, _r_phase);
}

int CdcCreditChannel::_Occupancy() const {
  return (int)_ingress.size() + (int)_await_sync.size() +
         (int)_await_delivery.size() + (int)_wire_q.size();
}

void CdcCreditChannel::Send(Credit *c) {
  assert(c);
  _ingress.push_back(c);
}

void CdcCreditChannel::ReadInputs() {
  int const T = GetSimTime();

  while (!_await_sync.empty()) {
    int const tin = _await_sync.front().first;
    if (T < tin + _sync_cycles) {
      break;
    }
    Credit *c = _await_sync.front().second;
    _await_sync.pop();
    int const t_done = tin + _sync_cycles;
    int const el = chiplet_clock::NextEdge(t_done, _r_period, _r_phase);
    _await_delivery.push_back(make_pair(el, c));
  }

  if (_WriterEdge(T) && !_ingress.empty()) {
    if (_Occupancy() >= _fifo_depth) {
      Error(string("CdcCreditChannel ") + FullName() +
            ": async FIFO overflow (increase chiplet_cdc_fifo_depth).");
    }
    Credit *c = _ingress.front();
    _ingress.pop_front();
    _await_sync.push(make_pair(T, c));
  }
}

void CdcCreditChannel::WriteOutputs() {
  int const T = GetSimTime();
  _output = 0;

  if (!_wire_q.empty()) {
    int const tr = _wire_q.front().first;
    if (T < tr) {
      return;
    }
    assert(T == tr);
    _output = _wire_q.front().second;
    _wire_q.pop();
    return;
  }

  if (!_ReaderEdge(T)) {
    return;
  }
  if (_await_delivery.empty() || _await_delivery.front().first > T) {
    return;
  }
  Credit *c = _await_delivery.front().second;
  _await_delivery.pop_front();
  _wire_q.push(make_pair(T + _wire_delay - 1, c));
}
