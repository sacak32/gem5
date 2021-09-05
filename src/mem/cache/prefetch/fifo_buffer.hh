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

#ifndef __CACHE_PREFETCH_FIFO_BUFFER_HH__
#define __CACHE_PREFETCH_FIFO_BUFFER_HH__

#include "mem/cache/prefetch/base.hh"

struct FIFOBufferParams;

namespace Prefetcher {

class FIFOBuffer : public SimObject {
  private:
    struct BufferEntry
    {
        Addr tag;
        enum State {CALCULATED, ASSIGNED, VALID} state;
        uint8_t* data;

        BufferEntry(Addr t, State s = CALCULATED) :
            tag(t), state(s), data(nullptr) {}

        ~BufferEntry() {
            if (data)
                delete data;
        }
    };

    std::list<BufferEntry> pfb;
    std::list<BufferEntry> pfbWaiting;

    using iterator = std::list<BufferEntry>::iterator;

    const unsigned bufferSize;
    const unsigned waitingSize;
    const unsigned dataSize;
    const Cycles latency;

  struct FIFOStats : public Stats::Group
  {
      FIFOStats(Stats::Group *parent);
      // STATS
      Stats::Scalar dequeueHits;
      Stats::Scalar dequeueMisses;
      Stats::Scalar dequeueOutstanding;
  } statsFIFO;

  public:
    FIFOBuffer(const FIFOBufferParams &p);
    ~FIFOBuffer() = default;

   /* Enqueue the address to the end of the queue */
   bool enqueue(Addr tag);

   /* Set data of the buffer entry with matching tag */
   bool setData(Addr tag, uint8_t* data);

   /* If the head has the address tag, and head is VALID, return the data */
   bool dequeue(Addr tag, uint8_t* &data, Cycles &lat);

   /* If head is VALID, return the tag and data */
   bool dequeue(Addr &tag, uint8_t* &data);

   /* flush the buffer */
   void flush();
};

} // namespace Prefetcher

#endif //__CACHE_PREFETCH_FIFO_BUFFER_HH__
