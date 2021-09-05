
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
  : Queued(p), byteOrder(p.sys->getGuestByteOrder()),
    prefetchDistance(p.prefetch_distance),
    buffer(p.fifo_buffer)
{
    curVisitAddr = 0;
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
        !(baseVisitAddress <= addr && addr < endVisitAddress) &&
        !(curEdgeStart <= addr && addr < curEdgeEnd))
       return;
    
    // Prefetch with visit address
    if (baseVisitAddress <= addr && addr < endVisitAddress) {
        // Get visit data
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        DPRINTF(BFS, "Visit addr found: %#x data: %lu\n", addr, data);

        // Writes to visit queue should be ignored
        if (data == 0) return;

        /*
        // If visit address is prefetched, prefetch the vertexes.
        if (pfi.getCmd() == MemCmd::HardPFReq)
        {*/
            // prefetch vertex data from visit data
            Addr newAddr = graph500csr ? 16*data : 8*data;
            newAddr += baseVertexAddress;
            DPRINTF(BFS, "Calculated vertex addr: %#x\n", newAddr);
            addresses.push_back(AddrPriority(newAddr, 0));

            curVisitAddr = addr;
            curEdgeStart = 0;
            curEdgeEnd = 0;
        /*}
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
        }*/
        buffer->flush();
    }
    
    if (baseVertexAddress <= addr && addr < endVertexAddress) {
        // Extract edge info
        uint64_t startOffset = pfi.get<uint64_t>(byteOrder);
        uint64_t endOffset = pfi.get<uint64_t>(byteOrder,8);
        curEdgeStart = baseEdgeAddress + 8*startOffset;
        curEdgeEnd = baseEdgeAddress + 8*endOffset;
        assert(curEdgeEnd <= endEdgeAddress);
        DPRINTF(BFS, "Vertex addr found: %#x curEdgeStart: %#x curEdgeEnd: %#x"
                " startOffset: %ld endOffset: %ld\n", addr, curEdgeStart, curEdgeEnd,
                startOffset, endOffset);

        /* Now, we prefetch the whole blocks, from beginning of the edge array
           until the prefetch distance, if the array ends, we stop. */
        int i = 0;
        Addr cur = blockAddress(curEdgeStart);
        while ( i <= prefetchDistance * EDGE_DATA_SIZE / blkSize && cur < curEdgeEnd )
        {
            addresses.push_back(AddrPriority(cur,0));
            cur += blkSize;
            i++;
        }
    }
    
    // If the edge block is prefetched, prefetch the visited data.
    if (blockAddress(baseEdgeAddress) <= addr && addr < endEdgeAddress &&
        pfi.getCmd() == MemCmd::HardPFReq)  {
        DPRINTF(BFS, "Edge addr found: %#x offset: %lu\n", addr, 
               (addr - baseEdgeAddress) / EDGE_DATA_SIZE);

        for (unsigned i = 0; i < blkSize / EDGE_DATA_SIZE; i++)
        {
            Addr cur = addr + i*EDGE_DATA_SIZE;
            if (curEdgeStart <= cur && cur < curEdgeEnd)
            {
                // HERE IM ASSUMING THAT EDGE_DATA_SIZE = 8                
                // Prefetch visited addresses 
                uint64_t offset = pfi.get<uint64_t>(byteOrder, EDGE_DATA_SIZE*i);
                Addr newAddr = baseVisitedAddress + VISITED_DATA_SIZE * offset;
                addresses.push_back(AddrPriority(newAddr,0));
                buffer->enqueue(newAddr);
            }
            // If we are prefetching the last visited addr, prefetch the next visit addr
            /*
            if (cur == curEdgeEnd - EDGE_DATA_SIZE)
            {
                Addr newAddr = curVisitAddr + VISIT_DATA_SIZE;
                if (newAddr < endVisitAddress)
                    addresses.push_back(AddrPriority(newAddr,0));
            }*/
        }
    }        
    
    // if the edge address is loaded, prefetch the neccesary edge block
    if (curEdgeStart <= addr && addr < curEdgeEnd && pfi.getSize() == EDGE_DATA_SIZE &&
        pfi.getCmd() != MemCmd::HardPFReq && addr % blkSize == 0)
    {
        DPRINTF(BFS, "Edge load found: %#x offset: %lu\n", addr,
               (addr - baseEdgeAddress) / EDGE_DATA_SIZE);

        Addr newAddr = addr + blkSize * prefetchDistance / EDGE_DATA_SIZE;
        if (newAddr < curEdgeEnd)
            addresses.push_back(AddrPriority(newAddr,0));
    }

    // Print the visited address
    if (baseVisitedAddress <= addr && addr < endVisitedAddress) {
        assert(pfi.getSize() == sizeof(uint64_t));
        uint64_t data = pfi.get<uint64_t>(byteOrder);
        buffer->setData(addr, pfi.getData());
        DPRINTF(BFS, "Visited addr found: %#x offset: %lu data: %lu\n", addr,
                (addr - baseVisitedAddress) / VISITED_DATA_SIZE, data);
    }
}        

void 
BFS::notifyFill(const PacketPtr &pkt)
{
    probeNotify(pkt, false);
}

bool 
BFS::trySatisfyAccess(PacketPtr &pkt, Cycles &lat)
{
    // Prefetcher must be working to satisfy accesses
    if (!inSearch)
        return false;

    // Consider only read requests with valid virtual address
    if (!pkt->req->hasVaddr() || pkt->cmd != MemCmd::ReadReq)
        return false;
    
    Addr vaddr = pkt->req->getVaddr();
    
    // FIFO Buffer must only have visited data
    if (baseVisitedAddress > vaddr || vaddr >= endVisitedAddress)
        return false;

    // Try request 
    uint8_t* data = new uint8_t[VISITED_DATA_SIZE];
    bool satisfied = buffer->dequeue(vaddr, data, lat);

    if (!satisfied)
        return false;

    pkt->setData(data);
    delete data;
    return true; 
}

unsigned 
BFS::getFetchSize(Addr addr)
{
    if (baseVisitAddress <= addr && addr < endVisitAddress)
        return 8;
    if (baseVertexAddress <= addr && addr < endVertexAddress)
        return 16;
    /* Note: Let's hope block address of baseEdgeAddress is not in one of the 
       other boundaries */   
    if (blockAddress(baseEdgeAddress) <= addr && addr < endEdgeAddress)
        return 64;
    if (baseVisitedAddress <= addr && addr < endVisitedAddress)
        return 8;
    return 64;
}

} // namespace Prefetcher
