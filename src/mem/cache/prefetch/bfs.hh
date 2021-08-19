/*
 * Copyright (c) 2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Ron Dreslinski
 */

/**
 * @file
 * Describes a tagged prefetcher.
 */

#ifndef __MEM_CACHE_PREFETCH_BFS_HH__
#define __MEM_CACHE_PREFETCH_BFS_HH__

#include "mem/cache/prefetch/queued.hh"

struct BFSPrefetcherParams;

namespace Prefetcher {

class BFS : public Queued
{
public:
  static Addr baseVertexAddress;
  static Addr endVertexAddress;
  static Addr baseEdgeAddress;
  static Addr endEdgeAddress;
  static Addr baseVisitAddress;
  static Addr endVisitAddress;
  static Addr baseVisitedAddress;
  static Addr endVisitedAddress;

  static Addr baseVisitedAddress2;
  static Addr endVisitedAddress2;

  static Addr baseVisitedAddress3;
  static Addr endVisitedAddress3;

  static bool inSearch;

  static uint64_t size_of_visited_items;
  static uint64_t size_of_visited_items2;
  static uint64_t size_of_visited_items3;

  Addr *visitOffsetToLoad;
  Addr *vertexOffsetToLoad;
  Addr *edgeOffsetToLoad;
  Addr *visitedOffsetToLoad;
  Addr *edgePacketsToLoad;
  Addr *edgeEnds;

  uint64_t avg_time_issue_load = 0;
  uint64_t avg_time_vertex_visit = 0;

  int fetch_prev = 1;

  bool subfetch = false;
  Addr current_subfetch_base_addr = 0;
  Addr current_subfetch_end_addr = 0;
  Addr current_subfetch_next_visit = 0;

  uint64_t prev_time_visit_vertex = 0;

  uint64_t *time_between_loads;

  int currentVertexOffset;
  int currentEdgeOffset;
  int currentPacketOffset;

  uint64_t functionalDataValue(Addr vaddr, PacketPtr pkt);

  uint64_t functionalPDataValue(Addr paddr, PacketPtr pkt);

  BFS(const BFSPrefetcherParams &p);
  ~BFS() = default;

  void
  calculatePrefetch(const PacketPtr &pkt,
                    std::vector<AddrPriority> &addresses);
                    
  static void setup(uint64_t id, uint64_t start,
                      uint64_t end, uint64_t element_size);
  
  void notifyFill(const PacketPtr &pkt) override;

  void prefetchAddr(PacketPtr pkt, Addr vaddr, std::vector<AddrPriority> &addresses, uint64_t action, uint64_t value);
};

} // namespace Prefetcher

#endif // __MEM_CACHE_PREFETCH_BFS_HH__
