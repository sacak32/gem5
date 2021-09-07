
/**
 * @file
 * Describes a bfs prefetcher.
 */

#include "mem/cache/prefetch/bfs.hh"

#include "mem/cache/base.hh"
#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

#define VISIT_DATA_SIZE 8
#define VERTEX_DATA_SIZE 8
#define EDGE_DATA_SIZE 8
#define VISITED_DATA_SIZE 8

int graph500csr = 1;

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p), byteOrder(p.sys->getGuestByteOrder())
{
}

bool BFS::inSearch = false;
Addr BFS::baseVertexAddress = 0;
Addr BFS::endVertexAddress = 0;
Addr BFS::baseEdgeAddress = 0;
Addr BFS::endEdgeAddress = 0;
Addr BFS::baseVisitAddress = 0;
Addr BFS::endVisitAddress = 0;
Addr BFS::baseVisitedAddress = 0;
Addr BFS::endVisitedAddress = 0;

uint64_t BFS::size_of_visited_items = 0;

void BFS::setup(uint64_t id, uint64_t start, uint64_t end, uint64_t size)
{
    Addr startAddr = (Addr) start;
    Addr endAddr = (Addr) end;
    printf("Inside setup, id: %lu start: %#lx end: %#lx\n", id, startAddr, endAddr); 

    if (id == 0)
    {
        //work list

        BFS::baseVisitAddress = startAddr;
        BFS::endVisitAddress = endAddr;
    }

    if (id == 1)
    {
        //vertex list
        if (size == 4)
            graph500csr = 0;
        else
            graph500csr = 1;
        BFS::baseVertexAddress = startAddr;
        BFS::endVertexAddress = endAddr;
    }

    if (id == 2)
    {
        BFS::inSearch = true;
        BFS::baseEdgeAddress = startAddr;
        BFS::endEdgeAddress = endAddr;
    }

    if (id == 3)
    {
        BFS::baseVisitedAddress = startAddr;
        BFS::endVisitedAddress = endAddr;

        if (graph500csr)
        {
            if (BFS::endVisitedAddress != BFS::baseVisitedAddress)
                size_of_visited_items = ((BFS::endVertexAddress + 8 - BFS::baseVertexAddress) / 2) / (BFS::endVisitedAddress - BFS::baseVisitedAddress);
        }
        else
        {

            if (BFS::endVisitedAddress != BFS::baseVisitedAddress)
                size_of_visited_items = (BFS::endVertexAddress - BFS::baseVertexAddress) / (BFS::endVisitedAddress - BFS::baseVisitedAddress);
        }

        if (size_of_visited_items != 0)
            size_of_visited_items = 64 / size_of_visited_items;
        else
            size_of_visited_items = 0;

        printf("size of visited items: %ld\n", size_of_visited_items);
    }
}

void BFS::calculatePrefetch(const PrefetchInfo &pfi,
                            std::vector<AddrPriority> &addresses)

{
    if (!inSearch)
        return;

    /*if (pfi.isCacheMiss()) {
        DPRINTF(BFS, "Whoops we have a cache miss.\n");
        return;
    }*/

    if (pfi.getSize() != sizeof(uint64_t)) 
        return;

    Addr addr = pfi.getAddr();
    uint64_t data = pfi.isCacheMiss() ? 29 : pfi.get<uint64_t>(byteOrder);

    if (baseVisitAddress <= addr && addr < endVisitAddress) {
        DPRINTF(BFS, "Visit %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVisitAddress) / VISIT_DATA_SIZE, data);
    }
    
    if (baseVertexAddress <= addr && addr < endVertexAddress) {
        DPRINTF(BFS, "Vertex %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVertexAddress) / VERTEX_DATA_SIZE, data);
    }
    
    if (baseEdgeAddress <= addr && addr < endEdgeAddress) {
        DPRINTF(BFS, "Edge %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseEdgeAddress) / EDGE_DATA_SIZE, data);
    }

    if (baseVisitedAddress <= addr && addr < endVisitedAddress) {
        DPRINTF(BFS, "Visited %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVisitedAddress) / VISITED_DATA_SIZE, data);
    }
}        

void
BFS::notifyFill(const PacketPtr &pkt)
{
    // probeNotify(pkt, false);
}

} // namespace Prefetcher

