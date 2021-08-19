
/**
 * @file
 * Describes a bfs prefetcher.
 */

#include "mem/cache/prefetch/bfs.hh"

#include "debug/BFS.hh"
#include "params/BFSPrefetcher.hh"

#include "cpu/simple/timing.hh"
#include "mem/cache/cache.hh"
#include "sim/system.hh"

#define XOFF(k) (vertices[2 * (k)])
#define XENDOFF(k) (vertices[1 + 2 * (k)])

#define functional 0

//#define graph500csr 1

int graph500csr = 1;

#define LISTSIZE 50

#define SUBPREFETCHWIDTH 7

#define EWMA1WIDTH 3
#define EWMA1MUL 7

#define EWMA2WIDTH 5
#define EWMA2MUL 31

namespace Prefetcher {

BFS::BFS(const BFSPrefetcherParams &p)
  : Queued(p)
{
    vertexOffsetToLoad = new Addr[LISTSIZE];
    visitOffsetToLoad = new Addr[LISTSIZE];
    edgeOffsetToLoad = new Addr[LISTSIZE];
    visitedOffsetToLoad = new Addr[LISTSIZE];
    edgePacketsToLoad = new Addr[LISTSIZE];
    time_between_loads = new uint64_t[LISTSIZE];

    currentVertexOffset = 0;
    currentPacketOffset = 0;
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

Addr BFS::baseVisitedAddress2 = 0;
Addr BFS::endVisitedAddress2 = 0;

Addr BFS::baseVisitedAddress3 = 0;
Addr BFS::endVisitedAddress3 = 0;

uint64_t BFS::size_of_visited_items = 0;
uint64_t BFS::size_of_visited_items2 = 0;
uint64_t BFS::size_of_visited_items3 = 0;

void BFS::prefetchAddr(PacketPtr pkt, Addr vaddr, std::vector<AddrPriority> &addresses, uint64_t action, uint64_t value)
{
    addresses.push_back(AddrPriority(vaddr, 0));

    Addr paddr = 0;
    tlb->translateFunctional(cache->system->threads[pkt->req->contextId()], vaddr, paddr);
    DPRINTF(BFS, "Calculated vaddr: %#x paddr: %#x\n", vaddr, paddr);
    if (paddr)
    {
        switch (action)
        {
        case 0:
            visitOffsetToLoad[value] = paddr;
            break;
        case 1:
            edgePacketsToLoad[value] = paddr;
            break;
        case 2:
            if (baseVisitAddress != 0)
                visitOffsetToLoad[value] = paddr;
            else
            {
                edgeOffsetToLoad[value] = paddr;
                edgePacketsToLoad[value] = paddr;
            }
            break;
        case 3:
            vertexOffsetToLoad[value] = paddr;
            break;
        case 4:
            edgeOffsetToLoad[value] = paddr;
            break;
        case 5:
            edgePacketsToLoad[value] = paddr;
            break;
        }
    }
    else
        printf("no physical addr!\n");
}

void BFS::setup(uint64_t id, uint64_t start, uint64_t end, uint64_t size)
{
    printf("Inside setup, id: %lu start: %#x end: %#x\n", id, start, end);

    if (id == 0)
    {
        //work list

        BFS::baseVisitAddress = (Addr)start;
        BFS::endVisitAddress = (Addr)end;
    }

    if (id == 1)
    {
        //vertex list
        if (size == 4)
            graph500csr = 0;
        else
            graph500csr = 1;
        BFS::baseVertexAddress = start;
        BFS::endVertexAddress = end;
    }

    if (id == 2)
    {
        BFS::inSearch = true;
        BFS::baseEdgeAddress = start;
        BFS::endEdgeAddress = end;
    }

    if (id == 3)
    {

        BFS::baseVisitedAddress = (Addr)start;

        BFS::endVisitedAddress = (Addr)end;

        //printf("visited %ld - %ld\n", BFS::baseVisitedAddress, BFS::endVisitedAddress);

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
    }

    if (id == 4)
    {
        //BfsVisitedList2(start,end,0,0);
    }
}

void BFS::calculatePrefetch(const PacketPtr &pkt,
                            std::vector<AddrPriority> &addresses)

{
    if (!inSearch)
        return;

    Addr addr = 0;
    if (pkt->req->hasVaddr())
    {
        addr = (pkt->req)->getVaddr();
    }

    Addr paddr = pkt->getAddr();
    
    if (addr == 0)
        DPRINTF(BFS, "No virtual addr found, physical addr: %#x\n", paddr);
    if (baseVertexAddress <= addr && addr < endVertexAddress)
        DPRINTF(BFS, "Vertex addr found: %#x\n", addr);
    if (baseVisitAddress <= addr && addr < endVisitAddress)
        DPRINTF(BFS, "Visit addr found: %#x\n", addr);
    if (baseVisitedAddress <= addr && addr < endVisitedAddress)
        DPRINTF(BFS, "Visited addr found: %#x\n", addr);
    if (baseEdgeAddress <= addr && addr < endEdgeAddress)
        DPRINTF(BFS, "Edge addr found: %#x\n", addr);
        
    // if(addr && !(old_mid == 0 || (old_mid == pkt->req->getAsid()))) {return;}

    if (pkt->isRead() && ((addr - baseVertexAddress) % 64) == 0 && baseVisitAddress == 0 && addr >= baseVertexAddress && addr < endVertexAddress)
    {
        prefetchAddr(pkt, addr + 32 * 8, addresses, -1, -1);
    }

    if (pkt->isRead() && subfetch && baseVisitAddress != 0 && addr >= baseVertexAddress && addr < endVertexAddress)
    {
        if (current_subfetch_base_addr == 0)
        {
            //got Start vertex

            current_subfetch_base_addr = (baseEdgeAddress + functionalDataValue(addr, pkt) * 8);

            if (current_subfetch_end_addr != 0 && current_subfetch_base_addr > current_subfetch_end_addr)
            {
                current_subfetch_base_addr = current_subfetch_end_addr;
                current_subfetch_end_addr = (baseEdgeAddress + functionalDataValue(addr, pkt) * 8);
            }
        }
        else
        {
            current_subfetch_end_addr = (baseEdgeAddress + functionalDataValue(addr, pkt) * 8);
            if (current_subfetch_base_addr > current_subfetch_end_addr)
            {
                current_subfetch_end_addr = current_subfetch_base_addr;
                current_subfetch_base_addr = (baseEdgeAddress + functionalDataValue(addr, pkt) * 8);
            }
        }
    }

    if (pkt->isRead() && subfetch && current_subfetch_base_addr > 0 && current_subfetch_end_addr > 0 && addr >= current_subfetch_base_addr && addr < current_subfetch_end_addr)
    {

        if (current_subfetch_end_addr - addr <= (64 * SUBPREFETCHWIDTH))
        {
            //load next vertex
            Addr toFetch = 0;
            if (baseVisitAddress != 0)
            {
                toFetch = current_subfetch_next_visit;

                if (toFetch > endVisitAddress || toFetch < baseVisitAddress)
                    return;
            }
            else
                toFetch = current_subfetch_end_addr;

            prefetchAddr(pkt, toFetch, addresses, 0, currentVertexOffset);
            time_between_loads[currentVertexOffset] = curTick();
            currentVertexOffset++;

            currentVertexOffset = currentVertexOffset % LISTSIZE;

            subfetch = false;
        }

        else if (((addr - current_subfetch_base_addr) % (64)) == 0)
        {

            Addr toFetch = addr + (64 * SUBPREFETCHWIDTH * 2);
            if (toFetch > endEdgeAddress || toFetch < baseEdgeAddress)
            {
            }
            else
            {

                prefetchAddr(pkt, toFetch, addresses, 1, currentPacketOffset);
                currentPacketOffset++;

                currentPacketOffset = currentPacketOffset % LISTSIZE;
            }
        }
    }

    if (addr && pkt->isRead() && ((addr >= baseVisitAddress && addr < endVisitAddress) || (baseVisitAddress == 0 && (((addr - baseEdgeAddress) % 64) == 0) && addr >= baseEdgeAddress && addr < endEdgeAddress)))
    {

        int64_t new_time = 0;
        if (prev_time_visit_vertex != 0)
        {
            if (avg_time_vertex_visit == 0)
            {
                avg_time_vertex_visit = curTick() - prev_time_visit_vertex;
            }
            else
            {
                new_time = curTick() - prev_time_visit_vertex;
                avg_time_vertex_visit = (new_time >> EWMA1WIDTH) + ((EWMA1MUL * avg_time_vertex_visit) >> EWMA1WIDTH);
            }
        }

        prev_time_visit_vertex = curTick();

        int fetchNext = 1;

        uint64_t avg2 = avg_time_issue_load;

        if (avg_time_vertex_visit != 0 && baseVisitAddress != 0)
        {
            fetchNext += (2 * (avg2)) / (avg_time_vertex_visit);

            if ((avg2) < avg_time_vertex_visit && baseVisitAddress != 0)
            {

                fetch_prev = 1;

                subfetch = true;

                current_subfetch_base_addr = 0;
                current_subfetch_end_addr = 0;

                current_subfetch_next_visit = addr + 8;

                return;
            }
            subfetch = false;
            if (fetch_prev + 1 < fetchNext)
                fetchNext = fetch_prev + 1;
        }
        else
            fetchNext = 32;

        for (int x = fetch_prev; x <= fetchNext; x++)
        {

            Addr toFetch = addr + x * 8;
            if (baseVisitAddress != 0 && (toFetch > endVisitAddress || toFetch < baseVisitAddress))
                return;
            if (baseVisitAddress == 0 && (toFetch > endEdgeAddress || toFetch < baseEdgeAddress))
                return;

            prefetchAddr(pkt, toFetch, addresses, 2, currentVertexOffset);
            time_between_loads[currentVertexOffset] = curTick();
            currentVertexOffset++;

            currentVertexOffset = currentVertexOffset % LISTSIZE;
        }
        fetch_prev = fetchNext;
    }
    if (addr == 0)
    {
        bool matched = false;

        Addr paddrMin = paddr - (paddr % 64);
        Addr paddrMax = paddrMin + 64; //TODO: change to blkSize

        if (!matched)
        {
            for (int x = 0; x < LISTSIZE; x++)
            {
                if (visitOffsetToLoad[x] >= paddr && visitOffsetToLoad[x] < paddrMax)
                {
                    matched = true;
                    uint64_t value_req = (visitOffsetToLoad[x] - paddr) / 8;

                    uint64_t offset = pkt->getPtr<uint64_t>()[value_req];
                    Addr toFetch;
                    if (graph500csr)
                        toFetch = offset * 8 * 2 + baseVertexAddress;
                    else
                        toFetch = offset * 8 + baseVertexAddress;

                    if (toFetch > endVertexAddress || toFetch < baseVertexAddress)
                    {

                        if (fetch_prev > 1)
                            fetch_prev--;
                        continue;
                    }

                    visitOffsetToLoad[x] = 0;

                    prefetchAddr(pkt, toFetch, addresses, 3, x);
                }
            }
        }

        if (!matched)
        {
            for (int x = 0; x < LISTSIZE; x++)
            {
                if (vertexOffsetToLoad[x] >= paddr && vertexOffsetToLoad[x] < paddrMax)
                {

                    matched = true;
                    uint64_t value_req = (vertexOffsetToLoad[x] - paddr) >> 3;

                    //then we loaded in a vertex list value.

                    int numberOfPackets = 2;
                    uint64_t numberOfValues = 10;
                    uint64_t offset = pkt->getPtr<uint64_t>()[value_req];
                    if (value_req < 7)
                    {

                        numberOfValues = ((functionalPDataValue(vertexOffsetToLoad[x] + 8, pkt) - offset) + 7); //above broken for boost for some reason.
                        numberOfPackets = numberOfValues / 8;
                        if (numberOfPackets > SUBPREFETCHWIDTH * 3)
                        {
                            numberOfPackets = SUBPREFETCHWIDTH * 3;
                            numberOfValues = SUBPREFETCHWIDTH * 3 * 8;
                        }
                    }

                    Addr toFetch = baseEdgeAddress + offset * 8;

                    if (toFetch > endEdgeAddress || toFetch < baseEdgeAddress)
                        break;

                    prefetchAddr(pkt, toFetch, addresses, 4, x);
                    //printf("issued prefetch of edge!\n");
                    vertexOffsetToLoad[x] = 0;

                    for (int x = 0; x < numberOfPackets; x++)
                    {

                        if (toFetch + 64 * x > endEdgeAddress || toFetch + 64 * x < baseEdgeAddress)
                            return;

                        prefetchAddr(pkt, toFetch + 64 * x, addresses, 5, currentPacketOffset);

                        currentPacketOffset++;
                        currentPacketOffset = currentPacketOffset % LISTSIZE;
                    }
                }
            }
        }

        for (int z = 0; z < LISTSIZE; z++)
        {
            if (edgeOffsetToLoad[z] >= paddr && edgeOffsetToLoad[z] < paddrMax)
            {

                if (avg_time_issue_load == 0)
                    avg_time_issue_load = (curTick() - time_between_loads[z]);
                else
                {
                    uint64_t new_time = (curTick() - time_between_loads[z]);
                    avg_time_issue_load = (new_time >> EWMA2WIDTH) + ((avg_time_issue_load * EWMA2MUL) >> EWMA2WIDTH);
                }

                edgeOffsetToLoad[z] = 0;
            }
        }

        if (!matched)
        {
            for (int x = 0; x < LISTSIZE; x++)
            {
                if (paddr != 0 && edgePacketsToLoad[x] >= paddr && edgePacketsToLoad[x] < paddrMax)
                {
                    matched = true;

                    //then load in all of the visited packets.
                    for (int y = 0; y < 8; y++)
                    {

                        uint64_t visitedOffset = functionalPDataValue(paddr + y * 8, pkt); //vertex value

                        if (baseVisitedAddress + ((size_of_visited_items * visitedOffset) >> 3) < endVisitedAddress)
                        {
                            Addr toFetch = baseVisitedAddress + ((size_of_visited_items * visitedOffset) >> 3);

                            prefetchAddr(pkt, toFetch, addresses, 6, 1);
                        }

                        if (baseVisitedAddress2 + ((size_of_visited_items2 * visitedOffset) >> 3) < endVisitedAddress2)
                        {
                            Addr toFetch = baseVisitedAddress2 + ((size_of_visited_items2 * visitedOffset) >> 3);
                            prefetchAddr(pkt, toFetch, addresses, 6, 1);
                        }

                        if (baseVisitedAddress3 + ((size_of_visited_items3 * visitedOffset) >> 3) < endVisitedAddress3)
                        {
                            Addr toFetch = baseVisitedAddress3 + ((size_of_visited_items3 * visitedOffset) >> 3);
                            prefetchAddr(pkt, toFetch, addresses, 6, 1);
                        }
                    }
                    edgePacketsToLoad[x] = 0;
                }
            }
        }

        //if(!matched) printf("failed on address %ld\n", paddr);
    }
}

uint64_t BFS::functionalDataValue(Addr vaddr, PacketPtr pkt)
{
    DPRINTF(BFS, "Functional access to virtual: %#x\n", vaddr);
    Addr paddr;
    tlb->translateFunctional(cache->system->threads[pkt->req->contextId()], vaddr, paddr);
    Addr paddr_req = paddr - (paddr % 64); //TODO: change to block size

    RequestPtr req = std::make_shared<Request>(paddr_req,
                     64, 0, requestorId);

    PacketPtr new_pkt = new Packet(req, MemCmd::ReadReq);
    
    uint64_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    new_pkt->dataStatic(&data);
    cache->system->getPhysMem().functionalAccess(new_pkt);

    int offset = (paddr - paddr_req) >> 3;

    return data[offset];
}

uint64_t BFS::functionalPDataValue(Addr paddr, PacketPtr pkt)
{
    DPRINTF(BFS, "Functional access to physical: %#x\n", paddr);
    Addr paddr_req = paddr - (paddr % 64); //TODO: change to block size

    RequestPtr req = std::make_shared<Request>(paddr_req,
                     64, 0, requestorId);

    PacketPtr new_pkt = new Packet(req, MemCmd::ReadReq);
    
    uint64_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    new_pkt->dataStatic(&data);
    cache->system->getPhysMem().functionalAccess(new_pkt);

    int offset = (paddr - paddr_req) >> 3;

    return data[offset];
}

void BFS::notifyFill(const PacketPtr &pkt)
{
    if (pkt->cmd != MemCmd::HardPFReq && pkt->cmd != MemCmd::HardPFResp)
        return;

    DPRINTF(BFS, "Prefetch fill!!!\n");
    
    if (pkt->req->hasVaddr())
        DPRINTF(BFS, "Oh my god!! Our prefetch req has a virtual addr: %#x\n", pkt->req->getVaddr());

    PrefetchInfo pfi(pkt, pkt->req->getPaddr(), true);
    notify(pkt, pfi);
}

} // namespace Prefetcher
