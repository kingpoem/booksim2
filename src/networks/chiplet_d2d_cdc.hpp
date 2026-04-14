#ifndef CHIPLET_D2D_CDC_HPP
#define CHIPLET_D2D_CDC_HPP

#include <deque>
#include <string>
#include <vector>

#include "network.hpp"

namespace chiplet_d2d_cdc {

struct CdcAllocParams {
  int classes;
  int fifo_depth;
  int flit_sync_base;
  int credit_sync_base;
  int wire_delay;
  bool gray_fifo;
  int gray_stages;
};

void AllocConnectX(Module *parent, std::string const &net_name, int Cx, int Cy,
                   std::vector<int> const &die_k,
                   std::vector<int> const &die_clock_period,
                   std::vector<int> const &die_clock_phase,
                   std::vector<FlitChannel *> &chan,
                   std::vector<CreditChannel *> &chan_cred,
                   std::deque<TimedModule *> &timed_modules,
                   int intra_flit_channels, CdcAllocParams const &p,
                   int &d2d_slot);

void AllocConnectY(Module *parent, std::string const &net_name, int Cx, int Cy,
                   std::vector<int> const &die_k,
                   std::vector<int> const &die_clock_period,
                   std::vector<int> const &die_clock_phase,
                   std::vector<FlitChannel *> &chan,
                   std::vector<CreditChannel *> &chan_cred,
                   std::deque<TimedModule *> &timed_modules,
                   int intra_flit_channels, CdcAllocParams const &p,
                   int &d2d_slot);

}  // namespace chiplet_d2d_cdc

#endif
