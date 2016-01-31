
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2016 Haggi Krey, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef MTAP_QUEUE_H
#define MTAP_QUEUE_H

#include <memory>
#include <queue>
#include "boost/thread/mutex.hpp"
#include "boost/thread/condition_variable.hpp"
#include <maya/MRenderView.h>
#include "../world.h"

struct Event
{
    enum Types
    {
        INTERRUPT = 0,
        TILEDONE = 1,
        FRAMEDONE = 2,
        RENDERDONE = 3,
        PRETILE = 4,
        FRAMEUPDATE = 5,
        IPRSTART = 6,
        IPRSTOP = 7,
        IPRPAUSE = 8,
        IPRREMOVE = 9,
        IPRUPDATE = 10,
        IPRFRAMEDONE = 11,
        STARTRENDER = 12,
        FINISH = 13,
        IPRUPDATESCENE = 14,
        USER = 15,
        PIXELSDONE = 16,
        UPDATEUI = 17,
        RENDERERROR = 18,
        INITRENDER = 19,
        INTERACTIVEFBCALLBACK = 20,
        ADDIPRCALLBACKS = 21,
        FRAMERENDER = 22
    };

    enum PixelMode
    {
        RECT = 0,
        PIXELS = 1
    };

    Types type;
    PixelMode pixelMode;
    size_t numPixels;
    MString message;
    // i use shared ptr here because if I push it into a queue, I make a copy
    // of the original what means I would have to move the move the unique_ptr<>
    // of a const object because it is automatically created Event(const & Event) and const
    // elements cannot be modified so I have to use another way here
    sharedPtr<RV_PIXEL> pixelData;
    sharedPtr<CmdArgs> cmdArgsData;

    size_t tile_xmin, tile_xmax, tile_ymin, tile_ymax;
    Event()
    {
        type = INTERRUPT;
        pixelMode = RECT;
        numPixels = 0;
    }
    ~Event()
    {
    }
};

template<typename Data>
class concurrent_queue
{
  private:
    std::queue<Data> the_queue;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;

  public:
    void push(Data const& data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    size_t size() const
    {
        return the_queue.size();
    }

    bool try_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        if (the_queue.empty())
        {
            return false;
        }

        popped_value=the_queue.front();
        the_queue.pop();
        return true;
    }

    void wait_and_pop(Data& popped_value)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while (the_queue.empty())
        {
            the_condition_variable.wait(lock);
        }
        popped_value=the_queue.front();
        the_queue.pop();
    }

};

static concurrent_queue<Event> EventList;
static concurrent_queue<Event> LogEventList;

concurrent_queue<Event> *theQueue();
concurrent_queue<Event> *theLogQueue();

#endif
