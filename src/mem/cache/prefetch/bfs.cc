
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

Addr BFS::baseVertexAddress = 0;
Addr BFS::endVertexAddress = 0;
Addr BFS::baseEdgeAddress = 0;
Addr BFS::endEdgeAddress = 0;
Addr BFS::baseVisitAddress = 0;
Addr BFS::endVisitAddress = 0;
Addr BFS::baseVisitedAddress = 0;
Addr BFS::endVisitedAddress = 0;

unsigned BFS::visitDataSize = 0;
unsigned BFS::vertexDataSize = 0;
unsigned BFS::edgeDataSize = 0;
unsigned BFS::visitedDataSize = 0;

bool BFS::inSearch = false;

void BFS::setup(uint64_t id, uint64_t start, uint64_t end, uint64_t size)
{
    Addr startAddr = (Addr) start;
    Addr endAddr = (Addr) end;
    printf("Inside setup, id: %lu start: %#lx end: %#lx size:%lu\n", id, startAddr, endAddr, size); 

    if (id == 0)
    {
        BFS::baseVisitAddress = startAddr;
        BFS::endVisitAddress = endAddr;
        BFS::visitDataSize = size;
    }

    if (id == 1)
    {
        BFS::baseVertexAddress = startAddr;
        BFS::endVertexAddress = endAddr;
        BFS::vertexDataSize = size;
    }

    if (id == 2)
    {
        BFS::inSearch = true;
        BFS::baseEdgeAddress = startAddr;
        BFS::endEdgeAddress = endAddr;
        BFS::edgeDataSize = size;
    }

    if (id == 3)
    {
        BFS::baseVisitedAddress = startAddr;
        BFS::endVisitedAddress = endAddr;
        BFS::visitedDataSize = size;
    }
}

void BFS::calculatePrefetch(const PrefetchInfo &pfi,
                            std::vector<AddrPriority> &addresses)

{
    if (!inSearch)
        return;

    Addr addr = pfi.getAddr();

    // Get prefetched data
    uint64_t data = 29;
    if (!pfi.isCacheMiss()) {
        switch(pfi.getSize()) {
            case sizeof(uint8_t):
                data = pfi.get<uint8_t>(byteOrder);
                break;
            case sizeof(uint16_t):
                data = pfi.get<uint16_t>(byteOrder);
                break;
            case sizeof(uint32_t):
                data = pfi.get<uint32_t>(byteOrder);
                break;
            case sizeof(uint64_t):
                data = pfi.get<uint64_t>(byteOrder);
                break;
            default:
                return;
        }
    }

    if (baseVisitAddress <= addr && addr < endVisitAddress) {
        DPRINTF(BFS, "Visit %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVisitAddress) / visitDataSize, data);
    }
    
    if (baseVertexAddress <= addr && addr < endVertexAddress) {
        DPRINTF(BFS, "Vertex %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVertexAddress) / vertexDataSize, data);
    }
    
    if (baseEdgeAddress <= addr && addr < endEdgeAddress) {
        DPRINTF(BFS, "Edge %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseEdgeAddress) / edgeDataSize, data);
    }

    if (baseVisitedAddress <= addr && addr < endVisitedAddress) {
        DPRINTF(BFS, "Visited %s addr: %#x offset: %lu data: %lu\n",
                pfi.getCmd().toString(), addr,
                (addr - baseVisitedAddress) / visitedDataSize, data);
    }
}        

void
BFS::notifyFill(const PacketPtr &pkt)
{
    // probeNotify(pkt, false);
}

} // namespace Prefetcher

