/*
Copyright(c) 2022 GameLogicDesign Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef GLD_RAYTRACER_AREA_H__
#define GLD_RAYTRACER_AREA_H__

#include "c4d.h"
#include "raytracer.h"
#include "tiledimage.h"

class RaytracerArea : public GeUserArea
{
public:
	RaytracerArea();
	~RaytracerArea();

	virtual Bool GetMinSize(Int32& w, Int32& h);
	virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg);

	void Clear();

public:
	Bool Render(maxon::JobGroupRef job, RENDERMODE renderMode = RENDERMODE::MULTITHREADED);

private:
	Raytracer _raytracer;
	TiledImage _tiledImage;
	AutoAlloc<BaseBitmap> _img;
};

#endif //GLD_RAYTRACER_AREA_H__