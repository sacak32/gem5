
/**
 * @file
 * Describes a bfs prefetcher.
 */

#include "mem/cache/prefetch/bfs.hh"

#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

int graph500csr = 1;

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p)
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

    Addr addr = pfi.getAddr();

    if (baseVertexAddress <= addr && addr < endVertexAddress)
        DPRINTF(BFS, "Vertex addr found: %#x\n", addr);
    if (baseVisitAddress <= addr && addr < endVisitAddress)
        DPRINTF(BFS, "Visit addr found: %#x\n", addr);
    if (baseVisitedAddress <= addr && addr < endVisitedAddress)
        DPRINTF(BFS, "Visited addr found: %#x\n", addr);
    if (baseEdgeAddress <= addr && addr < endEdgeAddress)
        DPRINTF(BFS, "Edge addr found: %#x\n", addr);
}        

void BFS::notifyFill(const PacketPtr &pkt)
{
    /*
    if (pkt->cmd != MemCmd::HardPFReq && pkt->cmd != MemCmd::HardPFResp)
        return;

    DPRINTF(BFS, "Prefetch fill!!!\n");
    
    if (pkt->req->hasVaddr())
        DPRINTF(BFS, "Oh my god!! Our prefetch req has a virtual addr: %#x\n", pkt->req->getVaddr());

    PrefetchInfo pfi(pkt, pkt->req->getPaddr(), true);
    notify(pkt, pfi);
    */
}

} // namespace Prefetcher
