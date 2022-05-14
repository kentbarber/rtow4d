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

#include "funrayvideopost.h"
#include "vpfunray.h"
#include "tiledimage.h"
#include "raytracer.h"

Bool FunRayVideoPostData::Init(GeListNode* node)
{
	BaseVideoPost* post = (BaseVideoPost*)node;
	BaseContainer *bc = post->GetDataInstance();
	bc->SetInt32(VP_FUNRAY_SAMPLES, 10);
	bc->SetInt32(VP_FUNRAY_MAXDEPTH, 50);
	bc->SetInt32(VP_FUNRAY_RENDERMODE_VIEWPORT, VP_FUNRAY_RENDERMODE_MULTITHREAD_PROGRESSIVE);
	return true;
}

Bool FunRayVideoPostData::RenderEngineCheck(BaseVideoPost* node, Int32 id)
{
	switch (id)
	{
	case RENDERSETTING_STATICTAB_MULTIPASS:
	case RENDERSETTING_STATICTAB_ANTIALIASING:
	case RENDERSETTING_STATICTAB_OPTIONS:
	case RENDERSETTING_STATICTAB_STEREO:
	case RENDERSETTING_STATICTAB_OVERRIDEMAT:
	case RENDERSETTING_STATICTAB_NET:
		return false;
	}

	return true;
}

RENDERRESULT FunRayVideoPostData::Execute(BaseVideoPost* node, VideoPostStruct* vps)
{
	if (vps == nullptr)
		return RENDERRESULT::USERBREAK;

	if (vps->thread && vps->thread->TestBreak())
		return RENDERRESULT::USERBREAK;

	switch (vps->vp)
	{
	case (VIDEOPOSTCALL::TILE):
		break;
	case (VIDEOPOSTCALL::INNER):
	{
		if (vps->open && *vps->error == RENDERRESULT::OK)
		{
			iferr_scope_handler
			{
				return RENDERRESULT::OUTOFMEMORY;
			};

			BaseContainer* bc = node->GetDataInstance();

			VPBuffer *colorBuf = vps->render->GetBuffer(VPBUFFER_RGBA, NOTOK);

			maxon::Int32 const xres = colorBuf->GetInfo(VPGETINFO::XRESOLUTION);
			maxon::Int32 const yres = colorBuf->GetInfo(VPGETINFO::YRESOLUTION);

			RENDERMODE mode = RENDERMODE::MULTITHREADED;

			Bool mainViewport = false;
			if (!(vps->renderflags & (RENDERFLAGS::CREATE_PICTUREVIEWER | RENDERFLAGS::EXTERNAL)))
			{
				Int32 viewportMode = bc->GetInt32(VP_FUNRAY_RENDERMODE_VIEWPORT);
				switch(viewportMode)
				{ 
				case VP_FUNRAY_RENDERMODE_SINGLETHREAD:
					mode = RENDERMODE::SINGLETHREAD;
					break;
				case VP_FUNRAY_RENDERMODE_MULTITHREAD:
					mode = RENDERMODE::MULTITHREADED;
					break;
				case VP_FUNRAY_RENDERMODE_SINGLETHREAD_PROGRESSIVE:
					mode = RENDERMODE::SINGLEPROGRESSIVE;
					break;
				case VP_FUNRAY_RENDERMODE_MULTITHREAD_PROGRESSIVE:
					mode = RENDERMODE::MULTITHREADEDPROGRESSIVE;
					break;
				default:
					mode = RENDERMODE::MULTITHREADEDPROGRESSIVE;
					break;
				}
				mainViewport = true;
			}

			TiledImage image;
			Raytracer raytracer;

			image.Init(xres, yres);
			raytracer.SetDocument(vps->doc);
			raytracer.SetVideoPost(colorBuf, vps->thread, mainViewport);

			Int32 samples = bc->GetInt32(VP_FUNRAY_SAMPLES);
			Int32 maxDepth = bc->GetInt32(VP_FUNRAY_MAXDEPTH);
			raytracer.SetSamplesPerPixel(samples);
			raytracer.SetMaxDepth(maxDepth);

			auto jobGroup = maxon::JobGroupRef::Create() iferr_return;

			SetupRenderer(jobGroup, mode, &raytracer, &image, nullptr);

			jobGroup.Enqueue();  // enqueue the job in the current queue and start
			jobGroup.Wait();
		}
		break;

	}
	case (VIDEOPOSTCALL::RENDER):
	{
		if (vps->vd && vps->open)
			vps->vd->SkipRenderProcess();
		break;
	}
	}

	return RENDERRESULT::OK;
}

Bool RegisterFunRayVideoPost()
{
	return RegisterVideoPostPlugin(GLD_ID_FUNRAY_VIDEOPOST, "FunRay Renderer"_s, PLUGINFLAG_VIDEOPOST_ISRENDERER, FunRayVideoPostData::Alloc, "VPfunray"_s, 0, 0);
}