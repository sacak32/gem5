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

#ifndef  __CACHE_PREFETCH_FIFO_BUFFER_IMPL_HH__
#define  __CACHE_PREFETCH_FIFO_BUFFER_IMPL_HH__

#include "mem/cache/prefetch/fifo_buffer.hh"
#include "debug/FIFOBuffer.hh"

FIFOBuffer::FIFOBuffer(unsigned ps, unsigned pws) : 
    pfbSize(ps), pfbWaitingSize(pws)
{}

bool 
FIFOBuffer::enqueue(Addr tag)
{
    if (pfb.size() == pfbSize) 
        panic("Buffer full.\n");

    BufferEntry be(tag, BufferEntry::ASSIGNED);
    pfb.emplace_back(be);
    DPRINTF(FIFOBuffer, "Buffer enqueue addr: %#x\n", tag);
    return true;
}

bool 
FIFOBuffer::setData(Addr tag, uint64_t data)
{
    iterator it = pfb.begin();
    while (it != pfb.end() && it->tag != tag)
        it++;

    if (it == pfb.end()) {
        DPRINTF(FIFOBuffer, "Buffer set data for addr: %#x data: %lu failed.\n",
            tag, data);
        return false;
    } 
        
    it->data = data;
    it->state = BufferEntry::VALID;
    DPRINTF(FIFOBuffer, "Buffer set data for addr: %#x data: %lu succeeded.\n",
        tag, data);
    return true;
}
    
bool 
FIFOBuffer::dequeue(Addr tag, uint64_t &data)
{
    assert(!pfb.empty());

    BufferEntry* be = &pfb.front();
    bool satisfied = false;
    if (be->tag != tag) 
        DPRINTF(FIFOBuffer, "Request addr: %#x data: %lu not found in the head of buffer.\n",
            tag, data);
    else if (be->state != BufferEntry::VALID)
        DPRINTF(FIFOBuffer, "Request addr: %#x data: %lu is still being fetched.\n",
            tag, data);
    else {
        data = be->data;
        satisfied = true; 
        DPRINTF(FIFOBuffer, "Request addr: %#x data: %lu is succesfully dequeued from buffer.\n", 
            tag, data);
    }

    pfb.pop_front();
    return satisfied;
}
        
void 
FIFOBuffer::flush()
{
   while (!pfb.empty())
       pfb.pop_front();
}

#endif //__CACHE_PREFETCH_FIFO_BUFFER_IMPL_HH__
