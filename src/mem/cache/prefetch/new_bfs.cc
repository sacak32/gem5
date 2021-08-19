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
bool BFS::active = false;

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
        BFS::active = true;
        BFS::baseEdgeAddress = start;
        BFS::endEdgeAddress = end;
    }

    if (id == 3)
    {
        BFS::baseVisitedAddress = start;
        BFS::endVisitedAddress = end;
    }
}

void
BFS::calculatePrefetch(const PrefetchInfo &pfi,
    std::vector<AddrPriority> &addresses)
{
    // Prefetch only when active
    if (!active)
        return;
    
    // Get virtual address
    Addr addr = pfi.getAddr();

    // Check whether address is in one of the arrays
    if (baseVisitAddress <= addr && addr < endVisitAddress)
        DPRINTF(BFS, "Worklist Address: %#x\n", addr);
    if (baseVertexAddress <= addr && addr < endVertexAddress)
        DPRINTF(BFS, "Vertex Address: %#x\n", addr);
    if (baseEdgeAddress <= addr && addr < endEdgeAddress)
        DPRINTF(BFS, "Edge Address: %#x\n", addr);
    if (baseVisitedAddress <= addr && addr < endVisitedAddress)
        DPRINTF(BFS, "Visited Address: %#x\n", addr);

    
}

void 
BFS::notifyFill(const PacketPtr& pkt) 
{
    /*
    if (pkt->req->hasVaddr()) 
    {
        Addr addr = pkt->req->getVaddr();
        DPRINTF(BFS, "Fill Address: %#x\n", addr);
    } else {
        DPRINTF(BFS, "NO VALID VADDR\n");
    }*/
}

} // namespace Prefetcher
