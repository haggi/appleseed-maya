
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

//#include "threads.h"
//
//#include <vector>
//#include <boost/thread/thread.hpp>
//#include <boost/bind.hpp>
//
//void threadFunc( ThreadData *ptr)
//{
//  if( ptr->numArgs > 0)
//      (*ptr->functionArg)(ptr->argument);
//  else
//      (*ptr->functionEmpty)();
//}
//
////void createThreads( int numThreads, int type, void *ptr, void *list )
////{
////    boost::thread_group threadGroup;
////
////    std::vector<ThreadData> threadDataList;
////    std::vector<boost::thread> threads;
////
////    for( int i = 0; i < numThreads; i++ )
////    {
////        ThreadData td;
////        td.counter = i;
////        td.ptr = ptr;
////        td.type = type;
////        td.threadSize = numThreads;
////        td.list = list;
////        threadDataList.push_back(td);
////    }
////
////    for( int i = 0; i < numThreads; i++ )
////    {
////        ThreadData *td = &(threadDataList[i]);
////        threadGroup.create_thread(boost::bind(&threadFunc, td));
////    }
////
////    threadGroup.join_all();
////}
//
//void createThread(ThreadData *ptr)
//{
//  boost::thread workerThread(threadFunc, ptr);
//  workerThread.join();
//}
