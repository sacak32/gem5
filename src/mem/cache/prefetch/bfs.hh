#ifndef __MEM_CACHE_PREFETCH_BFS_HH__
#define __MEM_CACHE_PREFETCH_BFS_HH__

#include "mem/cache/prefetch/queued.hh"

struct BFSPrefetcherParams;

namespace Prefetcher {

class BFS : public Queued
{
  private:
    // Array boundaries
    static Addr baseVertexAddress;
    static Addr endVertexAddress;
    static Addr baseEdgeAddress;
    static Addr endEdgeAddress;
    static Addr baseVisitAddress;
    static Addr endVisitAddress;
    static Addr baseVisitedAddress;
    static Addr endVisitedAddress;

  public:
    BFS(const BFSPrefetcherParams &p);
    ~BFS() = default;

    void calculatePrefetch(const PrefetchInfo &pfi,
                           std::vector<AddrPriority> &addresses) override;
    static void setup(uint64_t id, uint64_t start,
                      uint64_t end, uint64_t element_size);

};

} // namespace Prefetcher

#endif//__MEM_CACHE_PREFETCH_BFS_HH__
