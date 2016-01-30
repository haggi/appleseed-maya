
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
#include <maya/MObject.h>
#include <maya/MImage.h>

namespace SQueue
{
    template<typename Data>
    class concurrent_queue
    {
    private:
        std::queue<Data> the_queue;
        mutable boost::mutex the_mutex;
        boost::condition_variable my_condition_variable;
    public:
        void push(Data const& data)
        {
            boost::mutex::scoped_lock lock(the_mutex);
            the_queue.push(data);
            lock.unlock();
            my_condition_variable.notify_one();
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

            popped_value = the_queue.front();
            the_queue.pop();
            return true;
        }

        void wait_and_pop(Data& popped_value)
        {
            boost::mutex::scoped_lock lock(the_mutex);
            while (the_queue.empty())
            {
                my_condition_variable.wait(lock);
            }
            popped_value = the_queue.front();
            the_queue.pop();
        }
    };
}

#endif
