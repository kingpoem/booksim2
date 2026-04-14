#include "chiplet_mesh.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <utility>

#include "booksim.hpp"
#include "chiplet_d2d_cdc.hpp"
#include "globals.hpp"
#include "misc_utils.hpp"
#include "router.hpp"

using namespace std;

int ChipletMesh::_CoordsToId(int cx, int cy, int x, int y, int Cx, int k) {
  return (cy * Cx + cx) * (k * k) + y * k + x;
}

void ChipletMesh::_IdToCoords(int id, int Cx, int Cy, int k, int &cx, int &cy,
                              int &x, int &y) {
  int const dn = k * k;
  int const die = id / dn;
  cx = die % Cx;
  cy = die / Cx;
  int const loc = id % dn;
  x = loc % k;
  y = loc / k;
}

void ChipletGlobalIdToCoords(int id, int &cx, int &cy, int &x, int &y) {
  int const Cx = gChipletX;
  int const D = Cx * gChipletY;
  assert((int)gChipletDieK.size() == D);
  assert((int)gChipletRidBase.size() == D + 1);
  int d = 0;
  while (d + 1 < D && gChipletRidBase[d + 1] <= id) {
    ++d;
  }
  cx = d % Cx;
  cy = d / Cx;
  int const k = gChipletDieK[d];
  int const loc = id - gChipletRidBase[d];
  x = loc % k;
  y = loc / k;
}

int ChipletGlobalCoordsToId(int cx, int cy, int x, int y) {
  int const d = cy * gChipletX + cx;
  int const k = gChipletDieK[d];
  return gChipletRidBase[d] + y * k + x;
}

int ChipletGlobalKAtRouter(int rid) {
  int cx, cy, x, y;
  ChipletGlobalIdToCoords(rid, cx, cy, x, y);
  return gChipletDieK[cy * gChipletX + cx];
}

int ChipletMesh::_CoordsToId(int cx, int cy, int x, int y) const {
  int const d = cy * _Cx + cx;
  int const k = _die_k[d];
  return _rid_base[d] + y * k + x;
}

void ChipletMesh::_IdToCoords(int id, int &cx, int &cy, int &x, int &y) const {
  int const D = _Cx * _Cy;
  int d = 0;
  while (d + 1 < D && _rid_base[d + 1] <= id) {
    ++d;
  }
  cx = d % _Cx;
  cy = d / _Cx;
  int const k = _die_k[d];
  int const loc = id - _rid_base[d];
  x = loc % k;
  y = loc / k;
}

int ChipletMesh::_IntraLatency(int cx, int cy, int default_lat) const {
  if (_die_intra_lat.empty()) {
    return default_lat;
  }
  int v = _die_intra_lat[cy * _Cx + cx];
  return v > 0 ? v : default_lat;
}

static void chiplet_count_ports_globals(int id, bool cx_link, bool cy_link,
                                        int &inports, int &outports) {
  int cx, cy, x, y;
  ChipletGlobalIdToCoords(id, cx, cy, x, y);
  int const Cx = gChipletX;
  int const Cy = gChipletY;
  int const k = gChipletDieK[cy * Cx + cx];
  inports = outports = 0;
  if (x < k - 1) {
    ++inports;
    ++outports;
  }
  if (x > 0) {
    ++inports;
    ++outports;
  }
  if (y < k - 1) {
    ++inports;
    ++outports;
  }
  if (y > 0) {
    ++inports;
    ++outports;
  }
  if (cx_link && cx < Cx - 1 && x == k - 1) {
    ++inports;
    ++outports;
  }
  if (cx_link && cx > 0 && x == 0) {
    ++inports;
    ++outports;
  }
  if (cy_link && cy < Cy - 1 && y == k - 1) {
    ++inports;
    ++outports;
  }
  if (cy_link && cy > 0 && y == 0) {
    ++inports;
    ++outports;
  }
  ++inports;
  ++outports;
}

ChipletMesh::ChipletMesh(Configuration const &config, std::string const &name)
 : Network(config, name), _Cx(0), _Cy(0), _k_max(0), _connect_x(true),
      _connect_y(false), _intra_flit_channels(0), _d2d_interfaces(0),
      _cdc_enable(false) {
  _ComputeSize(config);
  _AllocChiplet(config);
  _BuildNet(config);
}

void ChipletMesh::_ComputeSize(Configuration const &config) {
  _Cx = config.GetInt("chiplet_x");
  _Cy = config.GetInt("chiplet_y");
  int const k_default = config.GetInt("chiplet_k");
  string const conn = config.GetStr("chiplet_connect");
  _connect_x = (conn == "x" || conn == "xy");
  _connect_y = (conn == "xy");

  if (_Cx < 1 || _Cy < 1 || k_default < 1) {
    Error("chiplet_x, chiplet_y, chiplet_k must be >= 1.");
  }
  if (!_connect_x && !_connect_y) {
    Error("chiplet_connect must be \"x\" or \"xy\".");
  }

  int const D = _Cx * _Cy;
  vector<int> dk = config.GetIntArray("chiplet_die_k");
  _die_k.resize(D, k_default);
  if (dk.empty()) {
 // uniform chiplet_k
  } else if ((int)dk.size() == 1) {
    fill(_die_k.begin(), _die_k.end(), dk[0]);
  } else if ((int)dk.size() == D) {
    _die_k = dk;
  } else {
    Error("chiplet_die_k must be empty, one int, or chiplet_x*chiplet_y list.");
  }
  for (int i = 0; i < D; ++i) {
    if (_die_k[i] < 1) {
      Error("chiplet_die_k entries must be >= 1.");
    }
  }

  for (int cx = 0; cx < _Cx - 1; ++cx) {
    for (int cy = 0; cy < _Cy; ++cy) {
      int const d = cy * _Cx + cx;
      if (_connect_x && _die_k[d] != _die_k[d + 1]) {
        Error("chiplet_die_k: adjacent dies in +X must share the same k "
 "(D2D boundary).");
      }
    }
  }
  for (int cy = 0; cy < _Cy - 1; ++cy) {
    for (int cx = 0; cx < _Cx; ++cx) {
      int const d = cy * _Cx + cx;
      if (_connect_y && _die_k[d] != _die_k[d + _Cx]) {
        Error("chiplet_die_k: adjacent dies in +Y must share the same k "
              "(D2D boundary).");
      }
    }
  }

  _rid_base.resize(D + 1);
  _rid_base[0] = 0;
  for (int i = 0; i < D; ++i) {
    _rid_base[i + 1] = _rid_base[i] + _die_k[i] * _die_k[i];
  }
  _nodes = _rid_base[D];
  _size = _nodes;
  _k_max = *max_element(_die_k.begin(), _die_k.end());

  _intra_flit_channels = 0;
  for (int i = 0; i < D; ++i) {
    int const k = _die_k[i];
    _intra_flit_channels += 4 * k * max(0, k - 1);
  }

  _d2d_interfaces = 0;
  if (_connect_x) {
    for (int cx = 0; cx < _Cx - 1; ++cx) {
      for (int cy = 0; cy < _Cy; ++cy) {
        _d2d_interfaces += _die_k[cy * _Cx + cx];
      }
    }
  }
  if (_connect_y) {
    for (int cy = 0; cy < _Cy - 1; ++cy) {
      for (int cx = 0; cx < _Cx; ++cx) {
        _d2d_interfaces += _die_k[cy * _Cx + cx];
      }
    }
  }

  vector<int> dil = config.GetIntArray("chiplet_die_intra_latency");
  if (!dil.empty()) {
    if ((int)dil.size() != D) {
      Error("chiplet_die_intra_latency must be empty or length chiplet_x*"
 "chiplet_y (use 0 to fall back to chiplet_intra_latency).");
    }
    _die_intra_lat = dil;
  }

  _cdc_enable = (config.GetInt("chiplet_cdc_enable") != 0);

  _die_clock_period.resize(D, 1);
  _die_clock_phase.resize(D, 0);
  vector<int> dcp = config.GetIntArray("chiplet_die_clock_period");
  if (dcp.empty()) {
  } else if ((int)dcp.size() == 1) {
    fill(_die_clock_period.begin(), _die_clock_period.end(), max(1, dcp[0]));
  } else if ((int)dcp.size() == D) {
    for (int i = 0; i < D; ++i) {
      _die_clock_period[i] = max(1, dcp[i]);
    }
  } else {
    Error("chiplet_die_clock_period must be empty, one int, or "
 "chiplet_x*chiplet_y list.");
  }

  vector<int> dcph = config.GetIntArray("chiplet_die_clock_phase");
  if (dcph.empty()) {
  } else if ((int)dcph.size() == 1) {
    fill(_die_clock_phase.begin(), _die_clock_phase.end(), dcph[0]);
  } else if ((int)dcph.size() == D) {
    _die_clock_phase = dcph;
  } else {
    Error("chiplet_die_clock_phase must be empty, one int, or "
          "chiplet_x*chiplet_y list.");
  }

  bool any_non_default = false;
  for (int i = 0; i < D; ++i) {
    if (_die_clock_period[i] != 1 || _die_clock_phase[i] != 0) {
      any_non_default = true;
      break;
    }
  }
  if (any_non_default && !_cdc_enable) {
    Error("chiplet: non-default die clocks require chiplet_cdc_enable = 1.");
  }

  // D2D: two uni-directional flit+credit links per interface (same as mesh hop).
  _channels = _intra_flit_channels + 2 * _d2d_interfaces;
}

void ChipletMesh::_AllocChiplet(Configuration const &config) {
  assert(_size == _nodes);
  _routers.resize(_size);
  gNodes = _nodes;

  _inject.resize(_nodes);
  _inject_cred.resize(_nodes);
  for (int s = 0; s < _nodes; ++s) {
    ostringstream name;
    name << Name() << "_fchan_ingress" << s;
    _inject[s] = new FlitChannel(this, name.str(), _classes);
    _inject[s]->SetSource(NULL, s);
    _timed_modules.push_back(_inject[s]);
    name.str("");
    name << Name() << "_cchan_ingress" << s;
    _inject_cred[s] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_inject_cred[s]);
  }
  _eject.resize(_nodes);
  _eject_cred.resize(_nodes);
  for (int d = 0; d < _nodes; ++d) {
    ostringstream name;
    name << Name() << "_fchan_egress" << d;
    _eject[d] = new FlitChannel(this, name.str(), _classes);
    _eject[d]->SetSink(NULL, d);
    _timed_modules.push_back(_eject[d]);
    name.str("");
    name << Name() << "_cchan_egress" << d;
    _eject_cred[d] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_eject_cred[d]);
  }

  _chan.resize(_channels);
  _chan_cred.resize(_channels);

  for (int c = 0; c < _intra_flit_channels; ++c) {
    ostringstream name;
    name << Name() << "_fchan_" << c;
    _chan[c] = new FlitChannel(this, name.str(), _classes);
    _timed_modules.push_back(_chan[c]);
    name.str("");
    name << Name() << "_cchan_" << c;
    _chan_cred[c] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_chan_cred[c]);
  }

  if (_cdc_enable) {
    chiplet_d2d_cdc::CdcAllocParams cap;
    cap.classes = _classes;
    cap.fifo_depth = config.GetInt("chiplet_cdc_fifo_depth");
    cap.flit_sync_base = max(0, config.GetInt("chiplet_cdc_sync_cycles"));
    cap.credit_sync_base =
        max(0, config.GetInt("chiplet_cdc_credit_sync_cycles"));
    cap.wire_delay = max(1, config.GetInt("chiplet_d2d_latency"));
    cap.gray_fifo = (config.GetInt("chiplet_cdc_gray_fifo") != 0);
    cap.gray_stages = max(0, config.GetInt("chiplet_cdc_gray_stages"));
    int d2d_i = 0;
    if (_connect_x) {
      chiplet_d2d_cdc::AllocConnectX(
          this, Name(), _Cx, _Cy, _die_k, _die_clock_period, _die_clock_phase,
          _chan, _chan_cred, _timed_modules, _intra_flit_channels, cap, d2d_i);
    }
    if (_connect_y) {
      chiplet_d2d_cdc::AllocConnectY(
          this, Name(), _Cx, _Cy, _die_k, _die_clock_period, _die_clock_phase,
          _chan, _chan_cred, _timed_modules, _intra_flit_channels, cap, d2d_i);
    }
    assert(d2d_i == _d2d_interfaces);
  } else {
    int const d2d_stride = 2;
    for (int i = 0; i < _d2d_interfaces; ++i) {
      int const base = _intra_flit_channels + d2d_stride * i;
      for (int j = 0; j < 2; ++j) {
        int const c = base + j;
        ostringstream name;
        name << Name() << "_fchan_d2d_" << i << "_" << j;
        _chan[c] = new FlitChannel(this, name.str(), _classes);
        _timed_modules.push_back(_chan[c]);
        name.str("");
        name << Name() << "_cchan_d2d_" << i << "_" << j;
        _chan_cred[c] = new CreditChannel(this, name.str());
        _timed_modules.push_back(_chan_cred[c]);
      }
    }
  }
}

void ChipletMesh::_BuildNet(Configuration const &config) {
  gChipletX = _Cx;
  gChipletY = _Cy;
  gChipletDieK = _die_k;
  gChipletRidBase = _rid_base;
  gChipletK = _k_max;
  gChipletConnectX = _connect_x ? 1 : 0;
  gChipletConnectY = _connect_y ? 1 : 0;
  gNodes = _nodes;
  gK = _k_max;
  gN = 2;

  bool const use_noc_latency = (config.GetInt("use_noc_latency") == 1);
  int const intra_lat =
      config.GetInt("chiplet_intra_latency") > 0
          ? config.GetInt("chiplet_intra_latency")
          : (use_noc_latency ? 1 : 1);
  int const d2d_lat = max(1, config.GetInt("chiplet_d2d_latency"));
  int const d2d_stride = 2;

  map<pair<int, int>, pair<int, int>> horiz_edge;
  map<pair<int, int>, pair<int, int>> vert_edge;

  int mesh_idx = 0;

  for (int cx = 0; cx < _Cx; ++cx) {
    for (int cy = 0; cy < _Cy; ++cy) {
      int const k = _die_k[cy * _Cx + cx];
      int const lat = _IntraLatency(cx, cy, intra_lat);
      for (int y = 0; y < k; ++y) {
        for (int x = 0; x < k - 1; ++x) {
          int const u = _CoordsToId(cx, cy, x, y);
          int const v = _CoordsToId(cx, cy, x + 1, y);
          int const c_e = mesh_idx++;
          int const c_w = mesh_idx++;
          assert(c_w < _intra_flit_channels);
          horiz_edge[make_pair(u, v)] = make_pair(c_e, c_w);
          _chan[c_e]->SetLatency(lat);
          _chan[c_w]->SetLatency(lat);
          _chan_cred[c_e]->SetLatency(lat);
          _chan_cred[c_w]->SetLatency(lat);
        }
      }
    }
  }

  for (int cx = 0; cx < _Cx; ++cx) {
    for (int cy = 0; cy < _Cy; ++cy) {
      int const k = _die_k[cy * _Cx + cx];
      int const lat = _IntraLatency(cx, cy, intra_lat);
      for (int y = 0; y < k - 1; ++y) {
        for (int x = 0; x < k; ++x) {
          int const u = _CoordsToId(cx, cy, x, y);
          int const v = _CoordsToId(cx, cy, x, y + 1);
          int const c_s = mesh_idx++;
          int const c_n = mesh_idx++;
          assert(c_n < _intra_flit_channels);
          vert_edge[make_pair(u, v)] = make_pair(c_s, c_n);
          _chan[c_s]->SetLatency(lat);
          _chan[c_n]->SetLatency(lat);
          _chan_cred[c_s]->SetLatency(lat);
          _chan_cred[c_n]->SetLatency(lat);
        }
      }
    }
  }

  assert(mesh_idx == _intra_flit_channels);

  map<int, int> d2d_x_base;
  map<int, int> d2d_y_base;
  int d2d_slot = 0;

  if (_connect_x) {
    for (int cx = 0; cx < _Cx - 1; ++cx) {
      for (int cy = 0; cy < _Cy; ++cy) {
        int const k = _die_k[cy * _Cx + cx];
        for (int y = 0; y < k; ++y) {
          int const west = _CoordsToId(cx, cy, k - 1, y);
          int const east = _CoordsToId(cx + 1, cy, 0, y);
          int const base = _intra_flit_channels + d2d_stride * d2d_slot;
          if (!_cdc_enable) {
            for (int j = 0; j < 2; ++j) {
              _chan[base + j]->SetLatency(d2d_lat);
              _chan_cred[base + j]->SetLatency(d2d_lat);
            }
          }
          d2d_x_base[west] = base;
          d2d_x_base[east] = base;
          ++d2d_slot;
        }
      }
    }
  }
  if (_connect_y) {
    for (int cy = 0; cy < _Cy - 1; ++cy) {
      for (int cx = 0; cx < _Cx; ++cx) {
        int const k = _die_k[cy * _Cx + cx];
        for (int x = 0; x < k; ++x) {
          int const south = _CoordsToId(cx, cy, x, k - 1);
          int const north = _CoordsToId(cx, cy + 1, x, 0);
          int const base = _intra_flit_channels + d2d_stride * d2d_slot;
          if (!_cdc_enable) {
            for (int j = 0; j < 2; ++j) {
              _chan[base + j]->SetLatency(d2d_lat);
              _chan_cred[base + j]->SetLatency(d2d_lat);
            }
          }
          d2d_y_base[south] = base;
          d2d_y_base[north] = base;
          ++d2d_slot;
        }
      }
    }
  }
  assert(d2d_slot == _d2d_interfaces);

  for (int rid = 0; rid < _size; ++rid) {
    ostringstream router_name;
    router_name << "router_" << rid;
    int inports, outports;
    chiplet_count_ports_globals(rid, _connect_x, _connect_y, inports,
 outports);
    _routers[rid] = Router::NewRouter(config, this, router_name.str(), rid,
                                      inports, outports);
    _timed_modules.push_back(_routers[rid]);

    int cx, cy, x, y;
    _IdToCoords(rid, cx, cy, x, y);
    int const kloc = _die_k[cy * _Cx + cx];

    if (x < kloc - 1) {
      int const v = _CoordsToId(cx, cy, x + 1, y);
      pair<int, int> const ce = horiz_edge[make_pair(rid, v)];
      _routers[rid]->AddInputChannel(_chan[ce.second], _chan_cred[ce.second]);
    }
    if (x > 0) {
      int const w = _CoordsToId(cx, cy, x - 1, y);
      pair<int, int> const ce = horiz_edge[make_pair(w, rid)];
      _routers[rid]->AddInputChannel(_chan[ce.first], _chan_cred[ce.first]);
    }
    if (x < kloc - 1) {
      int const v = _CoordsToId(cx, cy, x + 1, y);
      pair<int, int> const ce = horiz_edge[make_pair(rid, v)];
      _routers[rid]->AddOutputChannel(_chan[ce.first], _chan_cred[ce.first]);
    }
    if (x > 0) {
      int const w = _CoordsToId(cx, cy, x - 1, y);
      pair<int, int> const ce = horiz_edge[make_pair(w, rid)];
      _routers[rid]->AddOutputChannel(_chan[ce.second], _chan_cred[ce.second]);
    }

    if (y < kloc - 1) {
      int const v = _CoordsToId(cx, cy, x, y + 1);
      pair<int, int> const ce = vert_edge[make_pair(rid, v)];
      _routers[rid]->AddInputChannel(_chan[ce.second], _chan_cred[ce.second]);
    }
    if (y > 0) {
      int const w = _CoordsToId(cx, cy, x, y - 1);
      pair<int, int> const ce = vert_edge[make_pair(w, rid)];
      _routers[rid]->AddInputChannel(_chan[ce.first], _chan_cred[ce.first]);
    }
    if (y < kloc - 1) {
      int const v = _CoordsToId(cx, cy, x, y + 1);
      pair<int, int> const ce = vert_edge[make_pair(rid, v)];
      _routers[rid]->AddOutputChannel(_chan[ce.first], _chan_cred[ce.first]);
    }
    if (y > 0) {
      int const w = _CoordsToId(cx, cy, x, y - 1);
      pair<int, int> const ce = vert_edge[make_pair(w, rid)];
      _routers[rid]->AddOutputChannel(_chan[ce.second], _chan_cred[ce.second]);
    }

    if (_connect_x && cx < _Cx - 1 && x == kloc - 1) {
      int const base = d2d_x_base.at(rid);
      _routers[rid]->AddInputChannel(_chan[base + 1], _chan_cred[base + 1]);
      _routers[rid]->AddOutputChannel(_chan[base + 0], _chan_cred[base + 0]);
    }
    if (_connect_x && cx > 0 && x == 0) {
      int const base = d2d_x_base.at(rid);
      _routers[rid]->AddInputChannel(_chan[base + 0], _chan_cred[base + 0]);
      _routers[rid]->AddOutputChannel(_chan[base + 1], _chan_cred[base + 1]);
    }

    if (_connect_y && cy < _Cy - 1 && y == kloc - 1) {
      int const base = d2d_y_base.at(rid);
      _routers[rid]->AddInputChannel(_chan[base + 1], _chan_cred[base + 1]);
      _routers[rid]->AddOutputChannel(_chan[base + 0], _chan_cred[base + 0]);
    }
    if (_connect_y && cy > 0 && y == 0) {
      int const base = d2d_y_base.at(rid);
      _routers[rid]->AddInputChannel(_chan[base + 0], _chan_cred[base + 0]);
      _routers[rid]->AddOutputChannel(_chan[base + 1], _chan_cred[base + 1]);
    }

    _routers[rid]->AddInputChannel(_inject[rid], _inject_cred[rid]);
    _routers[rid]->AddOutputChannel(_eject[rid], _eject_cred[rid]);
    _inject[rid]->SetLatency(1);
    _eject[rid]->SetLatency(1);
  }
}

void ChipletMesh::RegisterRoutingFunctions() {}
