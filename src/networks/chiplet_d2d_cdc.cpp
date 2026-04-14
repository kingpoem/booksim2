#include "chiplet_d2d_cdc.hpp"

#include <algorithm>
#include <sstream>

#include "cdc_channel.hpp"
#include "chiplet_clock.hpp"
#include "module.hpp"

using namespace std;

namespace chiplet_d2d_cdc {

static int FlitSync(int Pw, int Pe, CdcAllocParams const &p) {
  int s = max(0, p.flit_sync_base);
  if (p.gray_fifo) {
    s += chiplet_clock::GraySyncExtraDelay(Pw, Pe, p.gray_stages);
  }
  return s;
}

static int CreditSync(int Pw, int Pe, CdcAllocParams const &p) {
  int s = max(0, p.credit_sync_base);
  if (p.gray_fifo) {
    s += chiplet_clock::GraySyncExtraDelay(Pe, Pw, p.gray_stages);
  }
  return s;
}

void AllocConnectX(Module *parent, string const &net_name, int Cx, int Cy,
                   vector<int> const &die_k,
                   vector<int> const &die_clock_period,
                   vector<int> const &die_clock_phase,
                   vector<FlitChannel *> &chan,
                   vector<CreditChannel *> &chan_cred,
                   deque<TimedModule *> &timed_modules, int intra_flit_channels,
                   CdcAllocParams const &p, int &d2d_slot) {
  for (int cx = 0; cx < Cx - 1; ++cx) {
    for (int cy = 0; cy < Cy; ++cy) {
      int const dw = cy * Cx + cx;
      int const de = cy * Cx + cx + 1;
      int const Pw = die_clock_period[dw];
      int const Phw = die_clock_phase[dw];
      int const Pe = die_clock_period[de];
      int const Phe = die_clock_phase[de];
      int const k = die_k[dw];
      int const fs = FlitSync(Pw, Pe, p);
      int const cs = CreditSync(Pw, Pe, p);
      for (int y = 0; y < k; ++y) {
        int const base = intra_flit_channels + 2 * d2d_slot;
        ostringstream fn0, fn1, cn0, cn1;
        fn0 << net_name << "_fchan_d2d_" << d2d_slot << "_0";
        chan[base + 0] = new CdcFlitChannel(parent, fn0.str(), p.classes, Pw,
                                            Phw, Pe, Phe, fs, p.fifo_depth,
                                            p.wire_delay);
        fn1 << net_name << "_fchan_d2d_" << d2d_slot << "_1";
        chan[base + 1] = new CdcFlitChannel(parent, fn1.str(), p.classes, Pe,
                                            Phe, Pw, Phw, fs, p.fifo_depth,
                                            p.wire_delay);
        cn0 << net_name << "_cchan_d2d_" << d2d_slot << "_0";
        chan_cred[base + 0] = new CdcCreditChannel(
            parent, cn0.str(), Pe, Phe, Pw, Phw, cs, p.fifo_depth,
            p.wire_delay);
        cn1 << net_name << "_cchan_d2d_" << d2d_slot << "_1";
        chan_cred[base + 1] = new CdcCreditChannel(
            parent, cn1.str(), Pw, Phw, Pe, Phe, cs, p.fifo_depth,
            p.wire_delay);
        timed_modules.push_back(chan[base + 0]);
        timed_modules.push_back(chan[base + 1]);
        timed_modules.push_back(chan_cred[base + 0]);
        timed_modules.push_back(chan_cred[base + 1]);
        ++d2d_slot;
      }
    }
  }
}

void AllocConnectY(Module *parent, string const &net_name, int Cx, int Cy,
                   vector<int> const &die_k,
                   vector<int> const &die_clock_period,
                   vector<int> const &die_clock_phase,
                   vector<FlitChannel *> &chan,
                   vector<CreditChannel *> &chan_cred,
                   deque<TimedModule *> &timed_modules, int intra_flit_channels,
                   CdcAllocParams const &p, int &d2d_slot) {
  for (int cy = 0; cy < Cy - 1; ++cy) {
    for (int cx = 0; cx < Cx; ++cx) {
      int const ds = cy * Cx + cx;
      int const dn = (cy + 1) * Cx + cx;
      int const Ps = die_clock_period[ds];
      int const Phs = die_clock_phase[ds];
      int const Pn = die_clock_period[dn];
      int const Phn = die_clock_phase[dn];
      int const k = die_k[ds];
      int const fs = FlitSync(Ps, Pn, p);
      int const cs = CreditSync(Ps, Pn, p);
      for (int x = 0; x < k; ++x) {
        int const base = intra_flit_channels + 2 * d2d_slot;
        ostringstream fn0, fn1, cn0, cn1;
        fn0 << net_name << "_fchan_d2d_" << d2d_slot << "_0";
        chan[base + 0] = new CdcFlitChannel(parent, fn0.str(), p.classes, Ps,
                                            Phs, Pn, Phn, fs, p.fifo_depth,
                                            p.wire_delay);
        fn1 << net_name << "_fchan_d2d_" << d2d_slot << "_1";
        chan[base + 1] = new CdcFlitChannel(parent, fn1.str(), p.classes, Pn,
                                            Phn, Ps, Phs, fs, p.fifo_depth,
                                            p.wire_delay);
        cn0 << net_name << "_cchan_d2d_" << d2d_slot << "_0";
        chan_cred[base + 0] = new CdcCreditChannel(
            parent, cn0.str(), Pn, Phn, Ps, Phs, cs, p.fifo_depth,
            p.wire_delay);
        cn1 << net_name << "_cchan_d2d_" << d2d_slot << "_1";
        chan_cred[base + 1] = new CdcCreditChannel(
            parent, cn1.str(), Ps, Phs, Pn, Phn, cs, p.fifo_depth,
            p.wire_delay);
        timed_modules.push_back(chan[base + 0]);
        timed_modules.push_back(chan[base + 1]);
        timed_modules.push_back(chan_cred[base + 0]);
        timed_modules.push_back(chan_cred[base + 1]);
        ++d2d_slot;
      }
    }
  }
}

}  // namespace chiplet_d2d_cdc
