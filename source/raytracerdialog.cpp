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

#include "raytracerdialog.h"

RaytracerDialog::RaytracerDialog()
{

}

RaytracerDialog::~RaytracerDialog()
{
}

Bool RaytracerDialog::InitValues(void)
{
#if defined(MAXON_TARGET_DEBUG)
	SetBool(RT_RENDER_MULTITHREADED, false);
	SetBool(RT_RENDER_PROGRESSIVE, false);
#else
	SetBool(RT_RENDER_MULTITHREADED, true);
	SetBool(RT_RENDER_PROGRESSIVE, true);
#endif

	SetRenderState(false);
	return true;
}

void RaytracerDialog::DestroyWindow()
{
	_jobs.Cancel();
	SetRenderState(false);
	_area.Clear();
}

Bool RaytracerDialog::CreateLayout()
{
	if (!SUPER::CreateLayout())
		return false;

	GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 0, 0, ""_s, 0);
	AddUserArea(RT_RENDERVIEW, BFH_SCALEFIT | BFV_SCALEFIT, SizePix(512), SizePix(512));
	GroupEnd();

	GroupBegin(0, BFH_SCALEFIT | BFV_FIT, 2, 0, ""_s, 0);
	AddCheckbox(RT_RENDER_MULTITHREADED, BFH_SCALEFIT | BFV_FIT, 100, 20, "Multi-Threaded"_s);
	AddCheckbox(RT_RENDER_PROGRESSIVE, BFH_SCALEFIT | BFV_FIT, 100, 20, "Progressive"_s);
	GroupEnd();

	GroupBegin(0, BFH_SCALEFIT | BFV_FIT, 2, 0, ""_s, 0);
	AddButton(RT_RENDER_START, BFH_SCALEFIT | BFV_FIT, 100, 20, "Render"_s);
	AddButton(RT_RENDER_STOP, BFH_SCALEFIT | BFV_FIT, 100, 20, "Stop"_s);
	GroupEnd();

	SetTitle("FunRay Raytracer"_s);
	AttachUserArea(_area, RT_RENDERVIEW);

	return true;
}

void RaytracerDialog::SetRenderState(Bool rendering)
{
	Enable(RT_RENDER_MULTITHREADED, !rendering);
	Enable(RT_RENDER_PROGRESSIVE, !rendering);
	Enable(RT_RENDER_START, !rendering);
	Enable(RT_RENDER_STOP, rendering);
}

Bool RaytracerDialog::Command(Int32 id, const BaseContainer& msg)
{
	iferr_scope_handler
	{
		DebugAssert(false);
		return false;
	};

	switch (id)
	{
	case RT_RENDER_START:
	{
		Bool multiThreaded = true;
		Bool progressive = true;
		GetBool(RT_RENDER_MULTITHREADED, multiThreaded);
		GetBool(RT_RENDER_PROGRESSIVE, progressive);
		_jobs = maxon::JobGroupRef::Create() iferr_return;

		RENDERMODE mode = RENDERMODE::SINGLETHREAD;
		if (multiThreaded)
		{ 
			if (progressive)
			{
				mode = RENDERMODE::MULTITHREADEDPROGRESSIVE;
			}
			else
			{
				mode = RENDERMODE::MULTITHREADED;
			}
		}
		else
		{
			if (progressive)
			{
				mode = RENDERMODE::SINGLEPROGRESSIVE;
			}
			else
			{
				mode = RENDERMODE::SINGLETHREAD;
			}
		}

		if (!_area.Render(_jobs, mode))
		{ 
			DebugAssert(false);
			return false;
		}

		_jobs.ObservableFinished().AddObserver([this]()
			{
				auto myLambda = [this]() -> void {
					SetRenderState(false);
					EventAdd();
				};
#if API_VERSION >= 26000
				maxon::ExecuteOnMainThread(myLambda, maxon::WAITMODE::DONT_WAIT);
#else
				maxon::ExecuteOnMainThread(myLambda, false);
#endif
			}) iferr_return;

		_jobs.Enqueue();  // enqueue the job in the current queue and start
		SetRenderState(true);
		EventAdd();
	}
	break;
	case RT_RENDER_STOP:
	{
		_jobs.Cancel();
		SetRenderState(false);
		EventAdd();
	}
	break;
	}
	return true;
}

//===========================================================

#define ID_RAYTRACERDIALOGCMD 1058663

class RaytracerDialogCmd : public CommandData
{
	RaytracerDialog m_dialog;

public:

#if (API_VERSION >= 21000)
	Bool Execute(BaseDocument* doc, GeDialog* parentManager)
#else
	Bool Execute(BaseDocument* doc)
#endif
	{
		return m_dialog.Open(DLG_TYPE::ASYNC, ID_RAYTRACERDIALOGCMD, -1, -1, 512, 288);
	}

	Bool RestoreLayout(void* secret)
	{
		return m_dialog.RestoreLayout(ID_RAYTRACERDIALOGCMD, 0, secret);
	}
};

Bool RegisterRaytracerDialog()
{
	return RegisterCommandPlugin(ID_RAYTRACERDIALOGCMD, "FunRay RenderView"_s, 0, nullptr, String(), NewObjClear(RaytracerDialogCmd));
}
