/**
 * Copyright (c) 2018 Metempsy Technology Consulting
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/cache/prefetch/fifo_buffer.hh"
#include "debug/FIFOBuffer.hh"
#include "params/FIFOBuffer.hh"

#define DATA_SIZE 8 // bytes

namespace Prefetcher {

FIFOBuffer::FIFOBuffer(const FIFOBufferParams &p) : 
    SimObject(p),
    size(p.size), waitingSize(p.waiting_size),
    latency(p.latency), statsFIFO(this)
{}

bool 
FIFOBuffer::enqueue(Addr tag)
{
    if (pfb.size() == size) 
        panic("Buffer full.\n");

    BufferEntry be(tag, BufferEntry::ASSIGNED);
    pfb.emplace_back(be);
    DPRINTF(FIFOBuffer, "Buffer enqueue addr: %#x\n", tag);
    return true;
}

bool 
FIFOBuffer::setData(Addr tag, uint8_t* data)
{
    iterator it = pfb.begin();
    while (it != pfb.end() && it->tag != tag)
        it++;

    if (it == pfb.end()) {
        DPRINTF(FIFOBuffer, "Buffer set data for addr: %#x failed.\n",
            tag);
        return false;
    } 
       
    assert(it->data == nullptr);
    it->data = new uint8_t[DATA_SIZE];
    
    std::memcpy(it->data, data, DATA_SIZE);
    it->state = BufferEntry::VALID;
    DPRINTF(FIFOBuffer, "Buffer set data for addr: %#x succeeded.\n",
        tag);
    return true;
}
    
bool 
FIFOBuffer::dequeue(Addr tag, uint8_t* &data, Cycles &lat)
{
    if (pfb.empty()) {
        DPRINTF(FIFOBuffer, "Request addr: %#x failed, buffer empty.\n", tag);
        return false;
    }

    BufferEntry* be = &pfb.front();
    bool satisfied = false;
    if (be->tag != tag) {
        statsFIFO.dequeueMisses++;
        DPRINTF(FIFOBuffer, "Request addr: %#x not found in the head of buffer.\n",
            tag);
    } else if (be->state != BufferEntry::VALID) {
        statsFIFO.dequeueOutstanding++;
        DPRINTF(FIFOBuffer, "Request addr: %#x is still being fetched.\n",
            tag);
    } else {
        std::memcpy(data, be->data, DATA_SIZE);
        lat = latency;
        satisfied = true; 

        statsFIFO.dequeueHits++;
        DPRINTF(FIFOBuffer, "Request addr: %#x is succesfully dequeued from buffer.\n", 
            tag);
    }

    pfb.pop_front();
    return satisfied;
}
        
void 
FIFOBuffer::flush()
{
   while (!pfb.empty())
       pfb.pop_front();
   DPRINTF(FIFOBuffer, "Buffer flushed.\n");
}

FIFOBuffer::FIFOStats::FIFOStats(Stats::Group *parent)
    : Stats::Group(parent),
    ADD_STAT(dequeueHits, UNIT_COUNT,
             "number of dequeue requests hit in buffer"),
    ADD_STAT(dequeueMisses, UNIT_COUNT,
             "number of dequeue requests miss in buffer"),
    ADD_STAT(dequeueOutstanding, UNIT_COUNT,
             "number of dequeue requests outstanding")
{
}
    
} // namespace Prefetcher
