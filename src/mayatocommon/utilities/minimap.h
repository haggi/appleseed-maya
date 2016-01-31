
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

#ifndef MINIMAP_H
#define MINIMAP_H

#include <stddef.h>

#include <vector>

template <class T, class S> class MiniMap
{
  public:
    void append(T t, S s);
    S *find(T t);
    void clear();
    int len();
    S *get(int i);

  private:
    std::vector<T> tClass;
    std::vector<S> sClass;
};

template <class T, class S> int MiniMap<T,S>::len()
{
    return (int)tClass.size();
}

template <class T, class S> void MiniMap<T,S>::clear()
{
    this->tClass.clear();
    this->sClass.clear();
}

template <class T, class S> S *MiniMap<T,S>::get(int i)
{
    return &sClass[i];
}

template <class T, class S> void MiniMap<T,S>::append(T t, S s)
{
    size_t i = 0;
    for (i = 0; i < this->tClass.size(); i++)
    {
        if (tClass[i] == t)
        {
            break;
        }
    }
    if (i >= this->tClass.size())
    {
        tClass.push_back(t);
        sClass.push_back(s);
    }
    else
    {
        sClass[i] = s;
    }
}

template <class T, class S> S *MiniMap<T,S>::find(T t)
{
    S *s = 0;
    for (size_t i = 0; i < this->tClass.size(); i++)
    {
        if (tClass[i] == t)
        {
            s = &sClass[i];
            break;
        }
    }
    return s;
}

#endif
