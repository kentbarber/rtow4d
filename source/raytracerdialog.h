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

#ifndef GLD_RAYTRACER_DIALOG_H__
#define GLD_RAYTRACER_DIALOG_H__

#include "c4d.h"
#include "raytracerarea.h"

enum
{
	RT_RENDERVIEW = 1000,
	RT_RENDER_START,
	RT_RENDER_STOP,
	RT_RENDER_MULTITHREADED,
	RT_RENDER_PROGRESSIVE,
};

class RaytracerDialog : public GeDialog
{
	INSTANCEOF(RaytracerDialog, GeDialog);

public:
	RaytracerDialog();
	~RaytracerDialog();

	virtual Bool InitValues(void);
	virtual void DestroyWindow(void);

	virtual Bool CreateLayout();
	virtual Bool Command(Int32 id, const BaseContainer& msg);

	virtual void SetRenderState(Bool rendering);

private:
	RaytracerArea _area;
	maxon::JobGroupRef _jobs;
};


#endif //GLD_RAYTRACERDIALOG_H__