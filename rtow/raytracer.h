#ifndef GLD_RAYTRACER_H__
#define GLD_RAYTRACER_H__

#include "c4d.h"

#include "rtweekend.h"
#include "camera.h"
#include "hittable_list.h"
#include "sphere.h"

#include "tiledimage.h"

#define TILEZIE 64

enum class RENDERMODE
{
	SINGLETHREAD,
	MULTITHREADED,
	SINGLEPROGRESSIVE,
	MULTITHREADEDPROGRESSIVE,
};

struct DirtyObject
{
	AutoAlloc<BaseLink> obj;
	AutoAlloc<BaseLink> original;
	AutoAlloc<BaseLink> mat;
	UInt32 dirty;
	UInt32 originalDirty;
	UInt32 matDirty;
	std::shared_ptr<hittable> renderObject;
	std::shared_ptr<material> renderMat;
};

typedef maxon::PointerArray<DirtyObject> DirtyObjectList;

class TiledImage;
class Raytracer
{
public:
	Raytracer();
	~Raytracer();
	Bool Init(GeUserArea* pUserArea, TiledImage* image, Bool progressive);

	void SetupScene();
	void ExportObject(BaseObject* pObj, BaseObject* original);

	Bool AddMaterial(BaseObject* pObj, BaseObject* original, DirtyObject& out);

	void AddSphere(BaseObject* pObj, BaseObject* original);
	void AddCube(BaseObject* pObj, BaseObject* original);
	void AddPlane(BaseObject* pObj, BaseObject* original);
	void AddCylinder(BaseObject* pObj, BaseObject* original);

	void DoRecursionCacheAdd(BaseObject* op, BaseObject* original, Bool processChildren = true);

	void SetupCamera();
	void SetVideoPost(VPBuffer* buffer, BaseThread* pThread, Bool mainViewport);

	Bool Raytrace(maxon::JobRef job);
	Bool RaytraceTile(maxon::JobRef job, Int32 tileIndex);
	Bool RaytraceProgressive(maxon::JobRef job);
	Bool RaytraceProgressiveTile(maxon::JobRef job, Int32 tileIndex);

	TiledImage* GetTiledImage();

	void SetProgressiveSampleCount(Int32 samples);
	void ClearSamples();

	CameraObject* GetCamera();

	void SetDocument(BaseDocument* doc);
	void SetSamplesPerPixel(Int32 samples);
	void SetMaxDepth(Int32 maxDepth);

	Bool UpdateObjects(Bool* rebuildScene = nullptr);
private:
	BaseDocument* _doc = nullptr;
	GeUserArea* _area = nullptr;
	TiledImage* _image = nullptr;
	maxon::JobRef _job;

	BaseThread* _videopostThread = nullptr;
	VPBuffer* _videopostBuffer = nullptr;

	AutoAlloc<BaseLink> _camera;

	DirtyObjectList _objectList;

private:
	Float _aspectRatio = 16.0 / 9.0;
	Int32 _imageWidth;
	Int32 _imageHeight;
	Int32 _samplesPerPixel = 10;
	Int32 _maxDepth = 50;

	// World
	hittable_list _world;

	// Camera
	point3 _lookfrom = { 13, 2, 3 };
	point3 _lookat = { 0, 0, 0 };
	vec3 _vup = { 0, 1, 0 };
	Float _distToFocus = 10.0;
	Float _aperture = 0.1;

	color _background = { 0, 0, 0 };
	Bool _useDomeBackground = true;

	camera _cam;

	color* _progressiveSamples = nullptr;
	Int32 _progressiveSampleCount = 0;

	Bool _mainViewport = false;
};

class TileJob : public maxon::JobInterfaceTemplate<TileJob, maxon::Bool>
{
public:
	TileJob() { };
	MAXON_IMPLICIT TileJob(Raytracer* tracer, Int32 tileIndex)
	{
		_tracer = tracer;
		_tileIndex = tileIndex;
	}

	maxon::Result<void> operator ()()
	{
		_tracer->RaytraceTile(this, _tileIndex);
		return SetResult(std::move(true));
	}

private:
	Raytracer* _tracer = nullptr;
	Int32 _tileIndex;
};

class SingleThreadJob : public maxon::JobInterfaceTemplate<SingleThreadJob, maxon::Bool>
{
public:
	SingleThreadJob() { };
	MAXON_IMPLICIT SingleThreadJob(Raytracer* tracer)
	{
		_tracer = tracer;
	}

	maxon::Result<void> operator ()()
	{
		_tracer->Raytrace(this);
		return SetResult(std::move(true));
	}

private:
	Raytracer* _tracer = nullptr;
};

class SingleProgressiveThreadJob : public maxon::JobInterfaceTemplate<SingleProgressiveThreadJob, maxon::Bool>
{
public:
	SingleProgressiveThreadJob() { };
	MAXON_IMPLICIT SingleProgressiveThreadJob(Raytracer* tracer)
	{
		_tracer = tracer;
	}

	maxon::Result<void> operator ()()
	{
		_tracer->RaytraceProgressive(this);
		return SetResult(std::move(true));
	}

private:
	Raytracer* _tracer = nullptr;
};

class ProgressiveTileJob : public maxon::JobInterfaceTemplate<ProgressiveTileJob, maxon::Bool>
{
public:
	ProgressiveTileJob() { };
	MAXON_IMPLICIT ProgressiveTileJob(Raytracer* tracer, Int32 tileIndex)
	{
		_tracer = tracer;
		_tileIndex = tileIndex;
	}

	maxon::Result<void> operator ()()
	{
		_tracer->RaytraceProgressiveTile(this, _tileIndex);
		return SetResult(std::move(true));
	}

private:
	Raytracer* _tracer = nullptr;
	Int32 _tileIndex;
};

class ProgressiveJob : public maxon::JobInterfaceTemplate<ProgressiveJob, maxon::Bool>
{
public:
	ProgressiveJob() { };
	MAXON_IMPLICIT ProgressiveJob(Raytracer* tracer)
	{
		_tracer = tracer;
	}

	maxon::Result<void> operator ()()
	{
		iferr_scope_handler
		{
			return SetResult(std::move(false));
		};

		UInt32 cameraDirty = 0;
		MAXON_SCOPE
		{
			BaseObject * pCamera = _tracer->GetCamera();
			if (pCamera)
			{
				cameraDirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
			}
		}

		Int32 startTime = GeGetTimer();

		Bool restart = false;
		Int32 sampleCount = 0;
		while (!IsCancelled())
		{
			Bool rebuildScene = false;
			restart |= _tracer->UpdateObjects(&rebuildScene);

			if (restart)
			{
				sampleCount = 0;
				restart = false;

				if (rebuildScene)
				{
					_tracer->SetupScene();
				}

				_tracer->SetupCamera();
				_tracer->ClearSamples();
			}

			sampleCount++;

			StatusSetText("Sample Count: " + String::IntToString(sampleCount));

			_tracer->SetProgressiveSampleCount(sampleCount);
			TiledImage* pImage = _tracer->GetTiledImage();
			if (!pImage)
				break;

			maxon::JobGroupRef tileJobs = maxon::JobGroupRef::Create() iferr_return;

			Int32 count = pImage->GetNumTiles();
			for (Int32 i = 0; i < count; i++)
			{
				maxon::JobRef job = ProgressiveTileJob::Create(_tracer, i) iferr_return;
				tileJobs->Add(job) iferr_return;
			}
			this->AddSubGroup<maxon::JobGroupRef>(tileJobs) iferr_return;
			tileJobs->Wait();

			BaseObject* pCamera = _tracer->GetCamera();
			if (pCamera)
			{
				UInt32 dirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
				if (cameraDirty != dirty)
				{
					cameraDirty = dirty;
					restart = true;
				}
			}
			GeSleep(60);
		}

		Int32 endTime = GeGetTimer();
		Int32 renderTime = endTime - startTime;
		GeConsoleOut("RenderTime: " + String::IntToString(renderTime));

		return SetResult(std::move(true));
	}

private:
	Raytracer* _tracer = nullptr;
};

Bool SetupRenderer(maxon::JobGroupRef jobGroup, RENDERMODE renderMode, Raytracer* pRayTracer, TiledImage* pImage, GeUserArea* pArea);

#endif