#ifndef CDC_CHANNEL_HPP
#define CDC_CHANNEL_HPP

#include <deque>
#include <queue>
#include <utility>
#include "booksim.hpp"
#include "credit.hpp"
#include "flitchannel.hpp"
#include "globals.hpp"

// Cross-clock-domain link model (finest simulator tick as common timebase):
// writer samples on writer_clock edges; after sync_cycles global ticks the
// datum is eligible; delivery to reader occurs on the first reader_clock edge
// >= eligible time; then optional wire_delay pipeline (same convention as Channel).

class CdcFlitChannel : public FlitChannel {
public:
  CdcFlitChannel(Module *parent, std::string const &name, int classes,
                 int w_period, int w_phase, int r_period, int r_phase,
                 int sync_cycles, int fifo_depth, int wire_delay);

  virtual void Send(Flit *flit) override;
  virtual void ReadInputs() override;
  virtual void WriteOutputs() override;

private:
  int _w_period;
  int _w_phase;
  int _r_period;
  int _r_phase;
  int _sync_cycles;
  int _fifo_depth;
  int _wire_delay;

  std::deque<Flit *> _ingress;
  std::queue<std::pair<int, Flit *>> _await_sync;
  std::deque<std::pair<int, Flit *>> _await_delivery;
  std::queue<std::pair<int, Flit *>> _wire_q;

  bool _WriterEdge(int t) const;
  bool _ReaderEdge(int t) const;
  int _Occupancy() const;
};

class CdcCreditChannel : public Channel<Credit> {
public:
  CdcCreditChannel(Module *parent, std::string const &name, int w_period,
                   int w_phase, int r_period, int r_phase, int sync_cycles,
                   int fifo_depth, int wire_delay);

  virtual void Send(Credit *c) override;
  virtual void ReadInputs() override;
  virtual void WriteOutputs() override;

private:
  int _w_period;
  int _w_phase;
  int _r_period;
  int _r_phase;
  int _sync_cycles;
  int _fifo_depth;
  int _wire_delay;

  std::deque<Credit *> _ingress;
  std::queue<std::pair<int, Credit *>> _await_sync;
  std::deque<std::pair<int, Credit *>> _await_delivery;
  std::queue<std::pair<int, Credit *>> _wire_q;

  bool _WriterEdge(int t) const;
  bool _ReaderEdge(int t) const;
  int _Occupancy() const;
};

#endif
