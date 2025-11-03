// $Id$

/*
 Copyright (c) 2007-2015, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sstream>

#include "globals.hpp"
#include "booksim.hpp"
#include "buffer.hpp"

Buffer::Buffer( const Configuration& config, int outputs, 
		Module *parent, const string& name, int router_id ) :
Module( parent, name ), _occupancy(0)
{
  // 使用路由器特定配置（如果提供了 router_id，优先使用路由器特定值，否则回退到全局值）
  int num_vcs;
  if(router_id >= 0) {
    num_vcs = config.GetRouterInt( router_id, "num_vcs" );
  } else {
    num_vcs = config.GetInt( "num_vcs" );
  }

  int buf_size;
  if(router_id >= 0) {
    buf_size = config.GetRouterInt( router_id, "buf_size" );
  } else {
    buf_size = config.GetInt("buf_size");
  }
  
  if(buf_size < 0) {
    int vc_buf_size;
    if(router_id >= 0) {
      vc_buf_size = config.GetRouterInt( router_id, "vc_buf_size" );
    } else {
      vc_buf_size = config.GetInt( "vc_buf_size" );
    }
    _size = num_vcs * vc_buf_size;
  } else {
    _size = buf_size;
  }

  _vc.resize(num_vcs);

  for(int i = 0; i < num_vcs; ++i) {
    ostringstream vc_name;
    vc_name << "vc_" << i;
    _vc[i] = new VC(config, outputs, this, vc_name.str( ) );
  }

#ifdef TRACK_BUFFERS
  int classes;
  if(router_id >= 0) {
    classes = config.GetRouterInt( router_id, "classes" );
  } else {
    classes = config.GetInt("classes");
  }
  _class_occupancy.resize(classes, 0);
#endif
}

Buffer::~Buffer()
{
  for(vector<VC*>::iterator i = _vc.begin(); i != _vc.end(); ++i) {
    delete *i;
  }
}

void Buffer::AddFlit( int vc, Flit *f )
{
  if(_occupancy >= _size) {
    Error("Flit buffer overflow.");
  }
  ++_occupancy;
  _vc[vc]->AddFlit(f);
#ifdef TRACK_BUFFERS
  ++_class_occupancy[f->cl];
#endif
}

void Buffer::Display( ostream & os ) const
{
  for(vector<VC*>::const_iterator i = _vc.begin(); i != _vc.end(); ++i) {
    (*i)->Display(os);
  }
}
