#include "mem/cache/prefetch/bfs.hh"

#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p)
{
}

Addr BFS::baseVertexAddress = 0;
Addr BFS::endVertexAddress = 0;
Addr BFS::baseEdgeAddress = 0;
Addr BFS::endEdgeAddress = 0;
Addr BFS::baseVisitAddress = 0;
Addr BFS::endVisitAddress = 0;
Addr BFS::baseVisitedAddress = 0;
Addr BFS::endVisitedAddress = 0;

void BFS::setup(uint64_t id, uint64_t start,
                uint64_t end, uint64_t element_size)
{
    if (id == 0)
    {
        BFS::baseVisitAddress = (Addr)start;
        BFS::endVisitAddress = (Addr)end;
    }

    if (id == 1)
    {
        BFS::baseVertexAddress = start;
        BFS::endVertexAddress = end;
    }

    if (id == 2)
    {
        BFS::baseEdgeAddress = start;
        BFS::endEdgeAddress = end;
    }

    if (id == 3)
    {
        BFS::baseVisitedAddress = start;
        BFS::endVisitedAddress = end;
    }

    printf("Setup for id: %lu\n", id);
}

void
BFS::calculatePrefetch(const PrefetchInfo &pfi,
    std::vector<AddrPriority> &addresses)
{
}

} // namespace Prefetcher
