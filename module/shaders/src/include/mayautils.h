
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2014 The masked shader writer, The appleseedhq Organization
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
						  
#define MAYA_COLORBALANCE float exposure = 0.0, \
						  vector defaultColor = vector(.5,.5,.5), \
						  vector colorGain = vector(1.0,1.0,1.0), \
						  vector colorOffset = vector(.0,.0,.0), \
						  float alphaGain = 1.0, \
						  float alphaOffset = 0.0, \
						  int alphaIsLuminance = 1, \
						  int invert=0

#define MAYA_DEFAULT_OUTPUT output vector outColor = 0, \
							output float outAlpha = 0

#define EPSILON 0.000001
#define OUTSIDEUV = (1.0 + EPSILON)

float sampleArea()
{
	return length(cross(dPdu, dPdv));
}

vector colorBalance( vector c, vector colorGain, vector colorOffset)
{
	vector result = c;
	result *= colorGain;
	result += colorOffset;
	return result;
}

#define WRAPMAX (1.000001)

float integral(float t, float nedge)
{
   return ((1.0 - nedge) * floor(t) + max(0.0, t - floor(t) - nedge));
}

float filteredPulseTrain(float edge, float period, float x, float dx)
{
   float invPeriod = 1.0 / period;

   float w = dx * invPeriod;
   float x0 = x * invPeriod - 0.5 * w;
   float x1 = x0 + w;
   float nedge = edge * invPeriod;

   float result;

   if (x0 == x1)
   {
     result = (x0 - floor(x0) < nedge) ? 0.0 : 1.0;
   }
   else
   {
      result = (integral(x1, nedge) - integral(x0, nedge)) / w;
   }

   return result;
}

