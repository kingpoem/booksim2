#ifndef CHIPLET_MESH_HPP
#define CHIPLET_MESH_HPP

#include <vector>

#include "network.hpp"

// Layout globals (filled by ChipletMesh::_BuildNet). Used by dim_order_chiplet_mesh.
void ChipletGlobalIdToCoords(int id, int &cx, int &cy, int &x, int &y);
int ChipletGlobalCoordsToId(int cx, int cy, int x, int y);
int ChipletGlobalKAtRouter(int rid);

class ChipletMesh : public Network {
  int _Cx;
  int _Cy;
  int _k_max;
  std::vector<int> _die_k;
  std::vector<int> _rid_base;
  std::vector<int> _die_intra_lat;

  bool _connect_x;
  bool _connect_y;

  int _intra_flit_channels;
  int _d2d_interfaces;

  int _CoordsToId(int cx, int cy, int x, int y) const;
  void _IdToCoords(int id, int &cx, int &cy, int &x, int &y) const;
  int _IntraLatency(int cx, int cy, int default_lat) const;

  void _ComputeSize(Configuration const &config) override;
  void _AllocChiplet(Configuration const &config);
  void _BuildNet(Configuration const &config) override;

public:
  /** Uniform-k layout only (legacy helpers). Prefer ChipletGlobal* after build. */
  static int _CoordsToId(int cx, int cy, int x, int y, int Cx, int k);
  static void _IdToCoords(int id, int Cx, int Cy, int k, int &cx, int &cy, int &x,
 int &y);

  ChipletMesh(Configuration const &config, std::string const &name);

  static void RegisterRoutingFunctions();

  double Capacity() const override { return -1.0; }
  void InsertRandomFaults(Configuration const &config) override {}
};

#endif
