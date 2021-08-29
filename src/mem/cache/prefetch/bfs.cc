
/**
 * @file
 * Describes a bfs prefetcher.
 */

#include "mem/cache/prefetch/bfs.hh"

#include "mem/cache/base.hh"
#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

int graph500csr = 1;

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p), byteOrder(p.sys->getGuestByteOrder())
{
    curVertexAddress = 0;
    curEdgeStart = 0;
    curEdgeEnd = 0;
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
    if (!inSearch || pfi.isCacheMiss())
        return;

    Addr addr = pfi.getAddr();

    if (pfi.getCmd() != MemCmd::HardPFReq && 
        !(baseVisitAddress <= addr && addr < endVisitAddress))
       return;
    
    // Prefetch with visit address
    if (baseVisitAddress <= addr && addr < endVisitAddress) {
        // Get visit data
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        DPRINTF(BFS, "Visit addr found: %#x data: %lu\n", addr, data);

        // If visit address is prefetched, prefetch the vertexes.
        if (pfi.getCmd() == MemCmd::HardPFReq)
        {
            // prefetch vertex data from visit data
            Addr newAddr = graph500csr ? 16*data : 8*data;
            newAddr += baseVertexAddress;
            DPRINTF(BFS, "Calculated vertex addr: %#x\n", newAddr);
            addresses.push_back(AddrPriority(newAddr, 0));
            addresses.push_back(AddrPriority(newAddr+8, 0));

            curVertexAddress = newAddr;
            curEdgeStart = 0;
            curEdgeEnd = 0;
        }
        // Else prefetch next visit address
        else 
        {
            Addr newAddr = addr + 8;
            if (newAddr < endVisitAddress)
            {
                DPRINTF(BFS, "Calculated visit addr: %#x\n", newAddr);
                addresses.push_back(AddrPriority(newAddr, 0));
            }
            else
                DPRINTF(BFS, "VISIT ADDR BOUNDARY PASSED!!!\n");
        }
    }
    
    if (baseVertexAddress <= addr && addr < endVertexAddress) {
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        
        // Extract edge address from vertex data
        if (addr == curVertexAddress) {
            DPRINTF(BFS, "Start vertex addr found: %#x data: %lu\n", addr, data);
            curEdgeStart = baseEdgeAddress + 8*data;
        }
        else if (addr == curVertexAddress+8) {
            DPRINTF(BFS, "End vertex addr found: %#x data: %lu\n", addr, data);
            curEdgeEnd = baseEdgeAddress + 8*data;
        } 
        else
            DPRINTF(BFS,"Vertex addr issue!!\n");
        
        // Wait until both edge start and edge end are set.
        if (curEdgeStart == 0 || curEdgeEnd == 0)
            return;

        DPRINTF(BFS, "Edge addresses completed!\n"); 
    }

    if (baseEdgeAddress <= addr && addr < endEdgeAddress) {
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        DPRINTF(BFS, "Edge addr found: %#x data: %lu\n", addr, data);
    }        

    if (baseVisitedAddress <= addr && addr < endVisitedAddress) {
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        DPRINTF(BFS, "Visited addr found: %#x data: %lu\n", addr, data);
    }
}        

void BFS::notifyFill(const PacketPtr &pkt)
{
    probeNotify(pkt, false);
}

} // namespace Prefetcher
