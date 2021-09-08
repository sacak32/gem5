
/**
 * @file
 * Describes a bfs prefetcher.
 */

#include "mem/cache/prefetch/bfs.hh"

#include "mem/cache/base.hh"
#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p), byteOrder(p.sys->getGuestByteOrder()),
    prefetchDistance(p.prefetch_distance),
    visitedBuffer(p.visited_buffer), edgeBuffer(p.edge_buffer)
{
    curVertexAddr = 0;
    curEdgeStart = 0;
    curEdgeEnd = 0;
    
    subfetch = false;
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
bool BFS::graph500 = false;

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
        
        BFS::graph500 = size == 8;
    }
}

void BFS::calculatePrefetch(const PrefetchInfo &pfi,
                            std::vector<AddrPriority> &addresses)

{
    if (!inSearch)
        return;

    Addr addr = pfi.getAddr();

    /* We usually only consider requests with a data, but edge load requests
       are an exception */
    if (pfi.isCacheMiss() && 
        !(pfi.getCmd() == MemCmd::ReadReq &&
          curEdgeStart <= addr && addr < curEdgeEnd))
       return;

    if (pfi.getCmd() != MemCmd::HardPFReq && 
        !(baseVertexAddress <= addr && addr < endVertexAddress) &&
        !(curEdgeStart <= addr && addr < curEdgeEnd))
       return;
    
    // Get prefetched data
    uint64_t data = 0;
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
                break;
        }
    }

    if (baseVertexAddress <= addr && addr < endVertexAddress &&
        pfi.getSize() == vertexDataSize) {
        DPRINTF(BFS, "Vertex load addr: %#x offset: %lu data: %#x\n", addr, 
                (addr - baseVertexAddress) / vertexDataSize, (Addr)data);
        
        /* If we are currently prefetching for a vertex, 
           cancel it and start capturing new pattern */
        if (subfetch) {
            subfetch = false;

            curVertexAddr = addr;
            curEdgeStart = data;
            curEdgeEnd = 0;

            if (visitedBuffer) 
                visitedBuffer->flush();
            if (edgeBuffer)
                edgeBuffer->flush();

            return;
        }

        if (addr == curVertexAddr + vertexDataSize) {
            curEdgeEnd = data;
            subfetch = true;
        } else if (addr == curVertexAddr - vertexDataSize) {
            curEdgeEnd = curEdgeStart;
            curEdgeStart = data;
            subfetch = true;
        } else {
            curVertexAddr = addr;
            curEdgeStart = data;
            return;
        }

        // graph 500 keeps indexes instead of direct pointers
        if (graph500) {
            curEdgeStart = baseEdgeAddress + edgeDataSize * curEdgeStart;
            curEdgeEnd = baseEdgeAddress + edgeDataSize * curEdgeEnd;
        }
        
        // lets not prefetch first 2 vertexes
        // curEdgeStart += 2*edgeDataSize;

        if (curEdgeStart >= curEdgeEnd)
            return;
        
        /* Now, we prefetch the whole blocks, from beginning of the edge array
           until the prefetch distance, if the array ends, we stop. */
        int i = curEdgeStart == blockAddress(curEdgeStart) ? 1 : 0;
        Addr cur = blockAddress(curEdgeStart);
        while ( i <= prefetchDistance * edgeDataSize / blkSize && cur < curEdgeEnd )
        {
            addresses.push_back(AddrPriority(cur,0));

            if (edgeBuffer)
                edgeBuffer->enqueue(cur);

            cur += blkSize;
            i++;
        }
    }
    
    // If the edge block is prefetched, prefetch the visited data.
    if (blockAddress(baseEdgeAddress) <= addr && addr < endEdgeAddress &&
        pfi.getCmd() == MemCmd::HardPFReq)  {
        DPRINTF(BFS, "Edge addr found: %#x offset: %lu\n", addr, 
               (addr - baseEdgeAddress) / edgeDataSize);
        
        if (edgeBuffer) {
            // Set edge buffer data first.
            edgeBuffer->setData(addr, pfi.getData());
    
            // While edge buffer has valid head entries, dequeue them and prefetch visited data 
            uint8_t* data = new uint8_t[blkSize];
            Addr addr;
            while (edgeBuffer->dequeue(addr, data))
            {
                for (unsigned i = 0; i < blkSize / edgeDataSize; i++)
                {
                    Addr cur = addr + i*edgeDataSize;
                    if (curEdgeStart <= cur && cur < curEdgeEnd)
                    {
                        // HERE IM ASSUMING THAT edge data < 2^32
                        // Prefetch visited addresses 
                        uint32_t offset = byteOrder == ByteOrder::big ? 
                                          betoh(*(uint32_t*)(data+edgeDataSize*i)) :
                                          letoh(*(uint32_t*)(data+edgeDataSize*i));
    
                        Addr newAddr = baseVisitedAddress + visitedDataSize * offset;
                        addresses.push_back(AddrPriority(newAddr,0));

                        if (visitedBuffer)
                            visitedBuffer->enqueue(newAddr);
                    }
                }
            }
            delete data;
        } else {
            for (unsigned i = 0; i < blkSize / edgeDataSize; i++)
            {
                Addr cur = addr + i*edgeDataSize;
                if (curEdgeStart <= cur && cur < curEdgeEnd)
                {
                    // HERE IM ASSUMING THAT edge data < 2^32
                    // Prefetch visited addresses 
                    uint32_t offset = pfi.get<uint32_t>(byteOrder, edgeDataSize*i);
                    Addr newAddr = baseVisitedAddress + visitedDataSize * offset;
                    addresses.push_back(AddrPriority(newAddr,0));
                }
            }
        }
    }        
    
    // if the edge address is loaded, prefetch the neccesary edge block
    if (curEdgeStart <= addr && addr < curEdgeEnd && pfi.getSize() == edgeDataSize &&
        pfi.getCmd() == MemCmd::ReadReq && addr % blkSize == 0)
    {
        DPRINTF(BFS, "Edge load found: %#x offset: %lu\n", addr,
               (addr - baseEdgeAddress) / edgeDataSize);

        Addr newAddr = addr + prefetchDistance * edgeDataSize; 
        if (newAddr < curEdgeEnd) {
            addresses.push_back(AddrPriority(newAddr,0));

            if (edgeBuffer)
                edgeBuffer->enqueue(newAddr);
        }
    }

    // Print the visited address
    if (baseVisitedAddress <= addr && addr < endVisitedAddress &&
        pfi.getCmd() == MemCmd::HardPFReq) {

        if (visitedBuffer)
            visitedBuffer->setData(addr, pfi.getData());

        DPRINTF(BFS, "Visited addr found: %#x offset: %lu data: %lu\n", addr,
                (addr - baseVisitedAddress) / visitedDataSize, data);
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
    if (!visitedBuffer || !inSearch)
        return false;

    // Consider only read requests with valid virtual address
    if (!pkt->req->hasVaddr() || pkt->cmd != MemCmd::ReadReq)
        return false;
    
    Addr vaddr = pkt->req->getVaddr();
   
    // We are only checking visited buffer
    if (baseVisitedAddress > vaddr || vaddr >= endVisitedAddress)
        return false;

    // Try request 
    uint8_t* data = new uint8_t[visitedDataSize];
    bool satisfied = visitedBuffer->dequeue(vaddr, data, lat);

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
        return visitDataSize;
    if (baseVertexAddress <= addr && addr < endVertexAddress)
        return vertexDataSize;
    /* Note: Let's hope block address of baseEdgeAddress is not in one of the 
       other boundaries */   
    if (blockAddress(baseEdgeAddress) <= addr && addr < endEdgeAddress)
        return blkSize;
    if (baseVisitedAddress <= addr && addr < endVisitedAddress)
        return visitedDataSize;
    return blkSize;
}

bool
BFS::notAllocateOnCache(Addr addr)
{
    // If we use fifo buffers, do not allocate visited prefetches on cache
    return visitedBuffer && (baseVisitedAddress <= addr && addr < endVisitedAddress);
}

} // namespace Prefetcher
