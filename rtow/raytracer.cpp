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

#include "raytracer.h"

#include "color.h"
#include "material.h"
#include "sphere.h"
#include "box.h"
#include "cylinder.h"

#include "tiledimage.h"
#include "funraymaterial.h"
#include "mfunraymaterial.h"
#include "funraycamera.h"
#include "opython.h"

Bool SetupRenderer(maxon::JobGroupRef jobGroup, RENDERMODE renderMode, Raytracer *pRayTracer, TiledImage *pImage, GeUserArea *pArea)
{
	iferr_scope_handler
	{
		return false;
	};

	if (!pRayTracer || !pImage)
		return false;

	switch (renderMode)
	{
	case RENDERMODE::MULTITHREADED:
	{
		pRayTracer->Init(pArea, pImage, false);
		Int32 count = pImage->GetNumTiles();
		for (Int32 i = 0; i < count; i++)
		{
			maxon::JobRef job = TileJob::Create(pRayTracer, i) iferr_return;
			jobGroup.Add(job) iferr_return;
		}
	}
	break;
	default:
	case RENDERMODE::SINGLETHREAD:
	{
		pRayTracer->Init(pArea, pImage, false);
		maxon::JobRef job = SingleThreadJob::Create(pRayTracer) iferr_return;
		jobGroup.Add(job) iferr_return;
	}
	break;
	case RENDERMODE::SINGLEPROGRESSIVE:
	{
		pRayTracer->Init(pArea, pImage, true);
		maxon::JobRef job = SingleProgressiveThreadJob::Create(pRayTracer) iferr_return;
		jobGroup.Add(job) iferr_return;
	}
	break;
	case RENDERMODE::MULTITHREADEDPROGRESSIVE:
	{
		pRayTracer->Init(pArea, pImage, true);
		maxon::JobRef job = ProgressiveJob::Create(pRayTracer) iferr_return;
		jobGroup.Add(job) iferr_return;
	}
	break;
	}
	
	return true;
}

BaseObject* GetNextObject(BaseObject* op)
{
	if (!op)
		return nullptr;

	if (op->GetDown())
		return op->GetDown();

	while (!op->GetNext() && op->GetUp())
		op = op->GetUp();

	return op->GetNext();
}

color ray_color_booktwo(const ray& r, const color& background, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	// If the ray hits nothing, return the background color.
	if (!world.hit(r, 0.001, infinity, rec))
		return background;

	ray scattered;
	color attenuation;
	color emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);

	if (!rec.mat_ptr->scatter(r, rec, attenuation, scattered))
		return emitted;

	return emitted + attenuation * ray_color_booktwo(scattered, background, world, depth - 1);
}

color ray_color_bookone(const ray& r, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0.001, infinity, rec)) {
		ray scattered;
		color attenuation;
		if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return attenuation * ray_color_bookone(scattered, world, depth - 1);
		return color(0, 0, 0);
	}

	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

color ray_color(const ray& r, const color& background, bool domeBackground, const hittable& world, int depth) {

	if (domeBackground)
	{
		return ray_color_bookone(r, world, depth);
	}
	else
	{
		return ray_color_booktwo(r, background, world, depth);
	}
}

hittable_list random_scene() {
	hittable_list world;

	auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
	world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_double();
			point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

			if ((center - point3(4, 0.2, 0)).length() > 0.9) {
				shared_ptr<material> sphere_material;

				if (choose_mat < 0.8) {
					// diffuse
					auto albedo = color::random() * color::random();
					sphere_material = make_shared<lambertian>(albedo);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else if (choose_mat < 0.95) {
					// metal
					auto albedo = color::random(0.5, 1);
					auto fuzz = random_double(0, 0.5);
					sphere_material = make_shared<metal>(albedo, fuzz);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
				else {
					// glass
					sphere_material = make_shared<dielectric>(1.5);
					world.add(make_shared<sphere>(center, 0.2, sphere_material));
				}
			}
		}
	}

	auto material1 = make_shared<dielectric>(1.5);
	world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

	auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
	world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

	auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
	world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

	return world;
}

void write_color(int x, int y, TiledImage* image, color pixel_color, int samples_per_pixel, VPBuffer *buffer) {
	auto r = pixel_color.x();
	auto g = pixel_color.y();
	auto b = pixel_color.z();

	// Replace NaN components with zero. See explanation in Ray Tracing: The Rest of Your Life.
	if (r != r) r = 0.0;
	if (g != g) g = 0.0;
	if (b != b) b = 0.0;

	// Divide the color by the number of samples and gamma-correct for gamma=2.0.
	auto scale = 1.0 / samples_per_pixel;
	r = sqrt(scale * r);
	g = sqrt(scale * g);
	b = sqrt(scale * b);

	if (buffer)
	{
		Float32 data[4];
		data[0] = r;
		data[1] = g;
		data[2] = b;
		data[3] = 1.0;

		buffer->SetLine(x, y, 1, data, 32, true);
	}
	else
	{
		image->SetPixel(x, y, Vector32(r, g, b));
	}
}

Raytracer::Raytracer()
{

}

Raytracer::~Raytracer()
{
	DeleteMem(_progressiveSamples);
}

Bool Raytracer::Init(GeUserArea* pUserArea, TiledImage* image, Bool progressive)
{
	_area = pUserArea;
	_image = image;

	_imageWidth = _image->GetWidth();
	_imageHeight = _image->GetHeight();
	_aspectRatio = Float(image->GetWidth()) / Float(_image->GetHeight());



	SetupScene();
	SetupCamera();

	if (progressive)
	{
		iferr(_progressiveSamples = NewMemClear(color, _imageWidth * _imageHeight))
		{
			return false;
		}
	}

	return true;
}

CameraObject* Raytracer::GetCamera()
{
	return (CameraObject*)_camera->ForceGetLink();
}

void Raytracer::SetupScene()
{
	_world.clear();

	BaseDocument* doc = GetActiveDocument();
	if (_doc)
	{
		doc = _doc;
	}
	else
	{
		_doc = doc;
	}

	ExportObject(doc->GetFirstObject(), nullptr);
}

void Raytracer::ExportObject(BaseObject *pObj, BaseObject* original)
{
	while (pObj)
	{
		Bool objectHandled = false;
		Int objectType = pObj->GetType();
		switch (objectType)
		{
		case Osds:
		case Osymmetry:
		case Oboole:
		case Ometaball:
		case Oatomarray:
		case Oarray:
		{
			DoRecursionCacheAdd(pObj, original, false);
			objectHandled = true;
		}
		break;

		case Oextrude:
		case Olathe:
		case Osweep:
		case Obezier:
		case Opython:
		case Ospline:
		case Ofloor:
		case Oloft:
		case Ocharacter:
		case Oxref:
		case 1027397: //XP
		//case Osky:
		//case Oenvironment:
		//case Oforeground:
		//case Obackground:
		//case Ostage:
		{
			DoRecursionCacheAdd(pObj, original);
			objectHandled = true;
		}
		break;

		case 1018544: //mograph cloner
		{
			DoRecursionCacheAdd(pObj, original);
			objectHandled = true;
		}
		break;

		case Osphere:
		{
			AddSphere(pObj, original);
		}
		break;
		case Ocube:
		{
			AddCube(pObj, original);
		}
		break;
		case Oplane:
		{
			AddPlane(pObj, original);
		}
		break;
		case Ocylinder:
		{
			AddCylinder(pObj, original);
		}
		break;
		}

		if (!objectHandled)
		{
			//Try and catch all unknown generators
			if (pObj->GetInfo() & OBJECT_GENERATOR)
			{
				DoRecursionCacheAdd(pObj, original, false);
			}
			else if (pObj->GetInfo() & OBJECT_POLYGONOBJECT)
			{
			}
			else
			{
				if (pObj->GetDeformCache())
					DoRecursionCacheAdd(pObj->GetDeformCache(), original, false);
				else if (pObj->GetCache(nullptr))
					DoRecursionCacheAdd(pObj->GetCache(nullptr), original, false);
			}
		}

		if (objectHandled)
		{
			pObj = pObj->GetNext();
		}
		else
		{
			pObj = GetNextObject(pObj);
		}
	}
}

void Raytracer::DoRecursionCacheAdd(BaseObject* op, BaseObject* original, Bool processChildren)
{
	if (!op)
		return;

	if (op == original)
		return;

	if (!original)
		original = op;

	//Int32 opType = op->GetType();

	BaseObject* tp = op->GetDeformCache();
	if (tp)
	{
		DoRecursionCacheAdd(tp, original, processChildren);
	}
	else
	{
		tp = op->GetCache(nullptr);
		if (tp)
		{
			DoRecursionCacheAdd(tp, original, processChildren);
		}
		else
		{
			if (!op->GetBit(BIT_CONTROLOBJECT))
			{
				ExportObject(op, original);
				return;
			}
		}
	}

	if (processChildren)
	{
		for (tp = op->GetDown(); tp; tp = tp->GetNext())
		{
			DoRecursionCacheAdd(tp, original, processChildren);
		}
	}
}

TextureTag* FindTextureTag(BaseObject* pObj)
{
	if (!pObj)
		return nullptr;

	BaseTag* pTag = (TextureTag*)pObj->GetTag(Ttexture);
	if (!pTag)
	{
		BaseObject* pParent = pObj->GetUp();
		while (pParent)
		{
			pTag = (TextureTag*)pParent->GetTag(Ttexture);
			if (pTag)
			{
				return (TextureTag*)pTag;
			}
			pParent = pParent->GetUp();
		}
	}
	return (TextureTag*)pTag;
}

Bool Raytracer::AddMaterial(BaseObject* pObj, BaseObject* original, DirtyObject &out)
{
	if (!pObj)
		return false;

	ObjectColorProperties prop;
	pObj->GetColorProperties(&prop);

	vec3 albedo(prop.color.x, prop.color.y, prop.color.z);

	std::shared_ptr<material> mat;

	BaseMaterial* pMat = nullptr;
	TextureTag* pTag = FindTextureTag(pObj);
	TextureTag* pOriginalTag = FindTextureTag(original);
	if (pOriginalTag)
	{
		pTag = pOriginalTag;
	}
	if (pTag)
	{
		BaseMaterial* pMatCheck = pTag->GetMaterial();
		if (pMatCheck && pMatCheck->GetType() == GLD_ID_FUNRAYMATERIAL)
		{
			pMat = pMatCheck;

			GeData dType;
			pMat->GetParameter(FUNRAYMATERIAL_TYPE, dType, DESCFLAGS_GET::NONE);

			GeData dColor;
			pMat->GetParameter(FUNRAYMATERIAL_COLOR, dColor, DESCFLAGS_GET::NONE);

			GeData dColorTexture;
			pMat->GetParameter(FUNRAYMATERIAL_COLOR_TEXTURE, dColorTexture, DESCFLAGS_GET::NONE);

			GeData dFuzz;
			pMat->GetParameter(FUNRAYMATERIAL_FUZZ, dFuzz, DESCFLAGS_GET::NONE);

			GeData dIOR;
			pMat->GetParameter(FUNRAYMATERIAL_IOR, dIOR, DESCFLAGS_GET::NONE);

			switch (dType.GetInt32())
			{
			case FUNRAYMATERIAL_TYPE_LAMBERT:
			{
				Vector c = dColor.GetVector();
				Filename f = dColorTexture.GetFilename();
				if (GeFExist(f))
				{
					maxon::UniqueRef<maxon::RawMem<Char>> filenameStr(f.GetString().GetCStringCopy());
					std::shared_ptr<image_texture> texture = make_shared<image_texture>(filenameStr);
					mat = make_shared<lambertian>(texture);
				}
				else
				{
					mat = make_shared<lambertian>(vec3(c.x, c.y, c.z));
				}
			}
			break;
			case FUNRAYMATERIAL_TYPE_METAL:
			{
				Vector c = dColor.GetVector();
				Float f = dFuzz.GetFloat();
				mat = make_shared<metal>(vec3(c.x, c.y, c.z), f);
			}
			break;
			case FUNRAYMATERIAL_TYPE_DIELECTRIC:
			{
				Float ior = dIOR.GetFloat();
				mat = make_shared<dielectric>(ior);
			}
			break;
			case FUNRAYMATERIAL_TYPE_ISOTROPIC:
			{
				Vector c = dColor.GetVector();
				Filename f = dColorTexture.GetFilename();
				if (GeFExist(f))
				{
					maxon::UniqueRef<maxon::RawMem<Char>> filenameStr(f.GetString().GetCStringCopy());
					std::shared_ptr<image_texture> texture = make_shared<image_texture>(filenameStr);
					mat = make_shared<isotropic>(texture);
				}
				else
				{
					mat = make_shared<isotropic>(vec3(c.x, c.y, c.z));
				}
			}
			break;
			case FUNRAYMATERIAL_TYPE_DIFFUSE_LIGHT:
			{
				Vector c = dColor.GetVector();
				Filename f = dColorTexture.GetFilename();
				if (GeFExist(f))
				{
					maxon::UniqueRef<maxon::RawMem<Char>> filenameStr(f.GetString().GetCStringCopy());
					std::shared_ptr<image_texture> texture = make_shared<image_texture>(filenameStr);
					mat = make_shared<diffuse_light>(texture);
				}
				else
				{
					mat = make_shared<diffuse_light>(vec3(c.x, c.y, c.z));
				}
			}
			break;
			default:
			{
				mat = make_shared<lambertian>(albedo);
			}
			break;
			}
		}
	}

	if (!mat)
	{
		mat = make_shared<lambertian>(albedo);
	}

	Matrix mg = pObj->GetMg();
	Vector pos = mg.off * 0.01;

	out.mat->SetLink(pMat);
	if (pMat)
	{
		out.matDirty = pMat->GetDirty(DIRTYFLAGS::DATA);
	}
	out.renderMat = mat;

	return true;
}


void Raytracer::AddSphere(BaseObject* pObj, BaseObject* original)
{
	if (!pObj)
		return;

	BaseContainer* bc = pObj->GetDataInstance();
	Float radius = bc->GetFloat(PRIM_SPHERE_RAD) * 0.01;

	Matrix mg = pObj->GetMg();
	Vector pos = mg.off * 0.01;

	ifnoerr(DirtyObject & dirtyObj = _objectList.Append())
	{
		AddMaterial(pObj, original, dirtyObj);

		dirtyObj.renderObject = make_shared<sphere>(vec3(pos.x, pos.y, -pos.z), radius, dirtyObj.renderMat);
		dirtyObj.obj->SetLink(pObj);
		dirtyObj.dirty = pObj->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);

		if (original)
		{
			dirtyObj.original->SetLink(original);
			dirtyObj.originalDirty = original->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA | DIRTYFLAGS::CACHE);
		}

		_world.add(dirtyObj.renderObject);
	}
}

void Raytracer::AddCylinder(BaseObject* pObj, BaseObject* original)
{
	if (!pObj)
		return;

	BaseContainer* bc = pObj->GetDataInstance();
	Float radius = bc->GetFloat(PRIM_CYLINDER_RADIUS) * 0.01;
	Float height = bc->GetFloat(PRIM_CYLINDER_HEIGHT) * 0.01;

	Matrix mg = pObj->GetMg();
	Vector pos = mg.off * 0.01;

	ifnoerr(DirtyObject & dirtyObj = _objectList.Append())
	{
		AddMaterial(pObj, original, dirtyObj);

		dirtyObj.renderObject = make_shared<cylinder>(point3(pos.x, pos.y, -pos.z), height*0.5 , -height * 0.5, radius, dirtyObj.renderMat);
		dirtyObj.obj->SetLink(pObj);
		dirtyObj.dirty = pObj->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);

		if (original)
		{
			dirtyObj.original->SetLink(original);
			dirtyObj.originalDirty = original->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA | DIRTYFLAGS::CACHE);
		}

		_world.add(dirtyObj.renderObject);
	}
}


void Raytracer::AddCube(BaseObject* pObj, BaseObject* original)
{
	if (!pObj)
		return;

	BaseContainer* bc = pObj->GetDataInstance();
	Vector len = bc->GetVector(PRIM_CUBE_LEN) * 0.01 * 0.5;

	Matrix mg = pObj->GetMg();
	Vector pos = mg.off * 0.01;

	ifnoerr(DirtyObject & dirtyObj = _objectList.Append())
	{
		AddMaterial(pObj, original, dirtyObj);

		Vector rot = MatrixToHPB(mg, ROTATIONORDER::XYZGLOBAL);

		vec3 box_min = vec3(-len.x, -len.y, -len.z) + vec3(pos.x, pos.y, -pos.z);
		vec3 box_max = vec3(len.x, len.y, len.z) + vec3(pos.x, pos.y, -pos.z);
		dirtyObj.renderObject = make_shared<box>(box_min, box_max, dirtyObj.renderMat);
		dirtyObj.renderObject = make_shared<rotate_x>(dirtyObj.renderObject, maxon::RadToDeg(-rot.x));
		dirtyObj.renderObject = make_shared<rotate_y>(dirtyObj.renderObject, maxon::RadToDeg(rot.y));
		dirtyObj.renderObject = make_shared<rotate_z>(dirtyObj.renderObject, maxon::RadToDeg(rot.z));

		dirtyObj.obj->SetLink(pObj);
		dirtyObj.dirty = pObj->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);

		if (original)
		{
			dirtyObj.original->SetLink(original);
			dirtyObj.originalDirty = original->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA | DIRTYFLAGS::CACHE);
		}

		_world.add(dirtyObj.renderObject);
	}
}

void Raytracer::AddPlane(BaseObject* pObj, BaseObject* original)
{
	if (!pObj)
		return;

	BaseContainer* bc = pObj->GetDataInstance();
	Float height = bc->GetFloat(PRIM_PLANE_HEIGHT) * 0.01 * 0.5;
	Float width = bc->GetFloat(PRIM_PLANE_WIDTH) * 0.01 * 0.5;
	Int32 dir = bc->GetInt32(PRIM_AXIS);

	Matrix mg = pObj->GetMg();
	Vector pos = mg.off * 0.01;

	ifnoerr(DirtyObject & dirtyObj = _objectList.Append())
	{
		AddMaterial(pObj, original, dirtyObj);

		switch (dir)
		{
		case PRIM_AXIS_ZP:
		case PRIM_AXIS_ZN:
			dirtyObj.renderObject = make_shared<xy_rect>(pos.x - width, pos.x + width, pos.y - height, pos.y + height, -pos.z, dirtyObj.renderMat);
			break;
		case PRIM_AXIS_XP:
		case PRIM_AXIS_XN:
			dirtyObj.renderObject = make_shared<yz_rect>(pos.y - width, pos.y + width, -pos.z - height, -pos.z + height, pos.x, dirtyObj.renderMat);
			break;
		case PRIM_AXIS_YP:
		case PRIM_AXIS_YN:
			dirtyObj.renderObject = make_shared<xz_rect>(pos.x - width, pos.x + width, -pos.z - height, -pos.z + height, pos.y, dirtyObj.renderMat);
			break;
		}

		dirtyObj.obj->SetLink(pObj);
		dirtyObj.dirty = pObj->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);

		if (original)
		{
			dirtyObj.original->SetLink(original);
			dirtyObj.originalDirty = original->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA | DIRTYFLAGS::CACHE);
		}

		_world.add(dirtyObj.renderObject);
	}
}

Bool Raytracer::UpdateObjects(Bool *rebuildScene)
{
	Bool objectChanged = false;
	for (auto& obj : _objectList)
	{
		BaseObject* pObj = (BaseObject * )obj.obj->GetLink(GetActiveDocument());
		if (pObj)
		{ 
			UInt32 d = pObj->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
			if (d != obj.dirty)
			{
				obj.dirty = d;

				Matrix mg = pObj->GetMg();
				Vector pos = mg.off * 0.01;

				if (pObj->GetType() == Osphere)
				{
					BaseContainer* bc = pObj->GetDataInstance();
					Float radius = bc->GetFloat(PRIM_SPHERE_RAD) * 0.01;

					std::shared_ptr<sphere> spherePtr = std::static_pointer_cast<sphere>(obj.renderObject);
					if (spherePtr)
					{
						spherePtr->radius = radius;
						spherePtr->center = vec3(pos.x, pos.y, -pos.z);
						objectChanged = true;
					}
				}
				else if (pObj->GetType() == Ocube)
				{
					BaseContainer* bc = pObj->GetDataInstance();
					Vector len = bc->GetVector(PRIM_CUBE_LEN) * 0.01 * 0.5;

					std::shared_ptr<box> boxPtr = std::static_pointer_cast<box>(obj.renderObject);
					if (boxPtr)
					{
						boxPtr->box_min = vec3(-len.x, -len.y, -len.z) + vec3(pos.x, pos.y, -pos.z);
						boxPtr->box_max = vec3(len.x, len.y, len.z) + vec3(pos.x, pos.y, -pos.z);
						objectChanged = true;
					}

					objectChanged = true;
					*rebuildScene = true;
				}
				else if (pObj->GetType() == Oplane)
				{
					objectChanged = true;
					*rebuildScene = true;
				}
				else if (pObj->GetType() == Ocylinder)
				{
					objectChanged = true;
					*rebuildScene = true;
				}
			}
		}

		BaseObject* original = (BaseObject*)obj.original->GetLink(GetActiveDocument());
		if (original)
		{
			UInt32 d = original->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA | DIRTYFLAGS::CACHE);
			if (d != obj.originalDirty)
			{
				obj.originalDirty = d;

				// Need to flush all generated objects and parse the scene
				if (rebuildScene)
				{
					*rebuildScene = true;
				}

				objectChanged = true;
			}
		}

		BaseMaterial* pMat = (BaseMaterial*)obj.mat->GetLink(GetActiveDocument());
		if (pMat)
		{
			UInt32 d = pMat->GetDirty(DIRTYFLAGS::DATA);
			if (d != obj.matDirty)
			{
				obj.matDirty = d;

				// TODO: Just Update the Material
				if (rebuildScene)
				{
					*rebuildScene = true;
				}

				objectChanged = true;
			}
		}
	}
	return objectChanged;
}

void Raytracer::SetupCamera()
{
	if (_imageWidth == 0)
	{
		DebugAssert(false);
		return;
	}

	BaseDocument* doc = GetActiveDocument();
	if (_doc)
	{
		doc = _doc;
	}

	BaseDraw* bd = doc->GetActiveBaseDraw();
	CameraObject* pCamera = (CameraObject *)bd->GetSceneCamera(doc);
	if (!pCamera)
	{
		pCamera = (CameraObject*)bd->GetEditorCamera();
	}
	if (pCamera)
	{
		_camera->SetLink(pCamera);

		GeData dVFov;
		pCamera->GetParameter(DescID(CAMERAOBJECT_FOV_VERTICAL), dVFov, DESCFLAGS_GET::NONE);
		Float fov_v_deg = maxon::RadToDeg(dVFov.GetFloat());

		// Adjust the vertical fov if rendering in the viewport because the FOV for the camera is related to the aspect ratio
		// of the dimensions set in the Render Settings, not the aspect ratio of the actual viewport. So when the viewports vertical height
		// changes from that of the render settings then the vertical fov needs to be adjusted.
		if (_mainViewport)
		{
			BaseContainer bcRender = doc->GetActiveRenderData()->GetData();
			Float w = bcRender.GetFloat(RDATA_XRES);
			Float h = bcRender.GetFloat(RDATA_YRES);

			Float aspectRatioForCameraFOV = w / h;

			if (_aspectRatio < aspectRatioForCameraFOV)
			{
				Float theta = dVFov.GetFloat();
				Float hh = tan(theta / 2.0);
				Float viewport_height = 2.0 * hh;
				Float viewport_width = aspectRatioForCameraFOV * viewport_height;

				Float vFov = 2 * atan(viewport_width / (2 * _aspectRatio));

				fov_v_deg = maxon::RadToDeg(vFov);
			}
		}

		Matrix camMg = pCamera->GetMg();
		_lookfrom = vec3(camMg.off.x, camMg.off.y, -camMg.off.z) * 0.01;
		_lookat = (_lookfrom + vec3(camMg.sqmat.v3.x, camMg.sqmat.v3.y, -camMg.sqmat.v3.z));
		_vup = vec3(camMg.sqmat.v2.x, camMg.sqmat.v2.y, -camMg.sqmat.v2.z);

		GeData dDist;
		pCamera->GetParameter(DescID(CAMERAOBJECT_TARGETDISTANCE), dDist, DESCFLAGS_GET::NONE);
		_distToFocus = dDist.GetFloat() * 0.01;
		_aperture = 0.1;

		// Convert FOV to focal length,
		// 
		// fov = 2 * atan(d/(2*f))
		// where,
		//   d = sensor dimension (APS-C 24.576 mm)
		//   f = focal length
		// 
		// f = 0.5 * d * (1/tan(fov/2))

		GeData dFstop;
		pCamera->GetParameter(DescID(CAMERAOBJECT_FNUMBER_VALUE), dFstop, DESCFLAGS_GET::NONE);
		Float fstop = dFstop.GetFloat();

		GeData dHFov;
		pCamera->GetParameter(DescID(CAMERAOBJECT_FOV), dHFov, DESCFLAGS_GET::NONE);

		//Float sensorSize = pCamera->GetAperture(); //This is actually the sensor size
		//Float halfFOV = dHFov.GetFloat() * 0.5;
		//Float focalLengthCalculated = 0.5f * sensorSize * (1.0f / maxon::Tan(halfFOV)); //This calculation works out the same as the camera->GetFocus()

		Float focalLength = pCamera->GetFocus();

		// Convert f-stop, focal length, and focal distance to
		// projected circle of confusion size at infinity in mm.
		//
		// coc = f*f / (n * (d - f))
		// where,
		//   f = focal length
		//   d = focal distance
		//   n = fstop (where n is the "n" in "f/n")

		fstop *= 10; //If I do this it matches the physical renderer

		float Radius = focalLength * focalLength / (fstop * (dDist.GetFloat()*10.0 - focalLength));
		_aperture = Radius * 2;

		BaseTag* pCamTag = (BaseTag*)pCamera->GetTag(GLD_FUNRAY_CAMERA_TAG);
		if (pCamTag)
		{
			BaseContainer* camBC = pCamTag->GetDataInstance();
			_aperture = camBC->GetFloat(FUNRAY_CAMERA_APERATURE);
		}

		_cam = camera(_lookfrom, _lookat, _vup, fov_v_deg, _aspectRatio, _aperture, _distToFocus);
	}
}

TiledImage* Raytracer::GetTiledImage()
{
	return _image;
}

Bool Raytracer::Raytrace(maxon::JobRef job)
{
	Int32 startTime = GeGetTimer();

	for (int j = _imageHeight - 1; j >= 0; --j) 
	{
		StatusSetBar(Int32((Float(j) / Float(_imageHeight)) * 100));
		for (int i = 0; i < _imageWidth; ++i) 
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < _samplesPerPixel; ++s) 
			{
				if (job) 
				{
					if (job.IsCancelled()) 
					{
						return true;
					}
				}

				if (_videopostThread && _videopostThread->TestBreak())
				{
					if (job)
					{
						if (job.GetJobGroup())
						{
							job.GetJobGroup()->Cancel();
						}
						job.Cancel();
					}
					return true;
				}

				auto u = (i + random_double()) / (_imageWidth - 1);
				auto v = (j + random_double()) / (_imageHeight - 1);
				ray r = _cam.get_ray(u, v);
				pixel_color += ray_color(r, _background, _useDomeBackground, _world, _maxDepth);
			}
			write_color(i, _imageHeight - j - 1, _image, pixel_color, _samplesPerPixel, _videopostBuffer);
		}
		if (_area)
		{
			_area->Redraw(true);
		}
	}

	Int32 endTime = GeGetTimer();
	Int32 renderTime = endTime - startTime;
	GeConsoleOut("Width: " + String::IntToString(_imageWidth));
	GeConsoleOut("Height: " + String::IntToString(_imageHeight));
	GeConsoleOut("RenderTime: " + String::IntToString(renderTime));

	StatusClear();
	return true;
}

Bool Raytracer::RaytraceTile(maxon::JobRef job, Int32 tileIndex)
{
	Tile* tile = _image->GetTile(tileIndex);
	if (!tile)
		return false;

	Int32 tileSizeX = _image->GetNumTilesX();
	Int32 xTile = tileIndex % tileSizeX;
	Int32 yTile = tileIndex / tileSizeX;
	Int32 xOff = xTile * TILESIZE;
	Int32 yOff = yTile * TILESIZE;

	Int32 maxY = maxon::ClampValue(yOff + TILESIZE, 0, _imageHeight);
	Int32 maxX = maxon::ClampValue(xOff + TILESIZE, 0, _imageWidth);

	Int32 startTime = GeGetTimer();
	for (Int32 j = yOff; j < maxY; j++)
	{
		for (Int32 i = xOff; i < maxX + TILESIZE; ++i)
		{
			color pixel_color(0, 0, 0);
			for (int s = 0; s < _samplesPerPixel; ++s) 
			{
				if (job) 
				{
					if (job.IsCancelled()) 
					{
						return true;
					}
				}

				if (_videopostThread && _videopostThread->TestBreak())
				{
					if (job)
					{
						if (job.GetJobGroup())
						{
							job.GetJobGroup()->Cancel();
						}
						job.Cancel();
					}
					return true;
				}

				auto u = (i + random_double()) / (_imageWidth - 1);
				auto v = (j + random_double()) / (_imageHeight - 1);
				ray r = _cam.get_ray(u, v);
				pixel_color += ray_color(r, _background, _useDomeBackground, _world, _maxDepth);
			}
			write_color(i, _imageHeight - j - 1, _image, pixel_color, _samplesPerPixel, _videopostBuffer);
		}

		if (_area)
		{
			_area->Redraw(true);
		}
	}

	Int32 endTime = GeGetTimer();
	Int32 renderTime = endTime - startTime;
	GeConsoleOut("Width: " + String::IntToString(_imageWidth));
	GeConsoleOut("Height: " + String::IntToString(_imageHeight));
	GeConsoleOut("RenderTime: " + String::IntToString(renderTime));

	StatusClear();
	return true;
}

Bool Raytracer::RaytraceProgressive(maxon::JobRef job)
{
	Int32 startTime = GeGetTimer();

	finally
	{
		Int32 endTime = GeGetTimer();
		Int32 renderTime = endTime - startTime;
		GeConsoleOut("Width: " + String::IntToString(_imageWidth));
		GeConsoleOut("Height: " + String::IntToString(_imageHeight));
		GeConsoleOut("RenderTime: " + String::IntToString(renderTime));
		StatusClear();
	};

	UInt32 cameraDirty = 0;
	UInt32 camTagDirty = 0;
	BaseObject* pCamera = GetCamera();
	BaseTag* pCamTag = nullptr;
	if (pCamera)
	{
		cameraDirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);

		pCamTag = (BaseTag*)pCamera->GetTag(GLD_FUNRAY_CAMERA_TAG);
		if (pCamTag)
		{
			camTagDirty = pCamTag->GetDirty(DIRTYFLAGS::DATA);
		}
	}


	Bool restart = false;
	Int32 samplesPerPixel = 0;
	while (true)
	{
		Bool rebuildScene = false;
		restart |= UpdateObjects(&rebuildScene);

		if (restart)
		{
			samplesPerPixel = 0;
			restart = false;

			if (rebuildScene)
			{
				SetupScene();
			}

			SetupCamera();
			ClearSamples();
		}

		StatusSetText("Sample Count: " + String::IntToString(samplesPerPixel));
		samplesPerPixel++;
		for (int j = _imageHeight - 1; j >= 0; --j) 
		{
			StatusSetBar(Int32((Float(j) / Float(_imageHeight)) * 100));
			for (int i = 0; i < _imageWidth; ++i)
			{
				if (pCamera)
				{
					UInt32 dirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
					if (dirty != cameraDirty)
					{
						cameraDirty = dirty;
						restart = true;
						break;
					}
				}

				if (pCamTag)
				{
					UInt32 dirty = pCamTag->GetDirty(DIRTYFLAGS::DATA);
					if (dirty != camTagDirty)
					{ 
						camTagDirty = dirty;
						restart = true;
						break;
					}
				}

				if (job)
				{
					if (job.IsCancelled())
					{
						return true;
					}
				}

				if (_videopostThread && _videopostThread->TestBreak())
				{
					if (job)
					{
						if (job.GetJobGroup())
						{
							job.GetJobGroup()->Cancel();
						}
						job.Cancel();
					}
					return true;
				}

				auto u = (i + random_double()) / (_imageWidth - 1);
				auto v = (j + random_double()) / (_imageHeight - 1);
				ray r = _cam.get_ray(u, v);
				_progressiveSamples[j * _imageWidth + i] += ray_color(r, _background, _useDomeBackground, _world, _maxDepth);
			}

			if (restart)
			{
				break;
			}

			for (int i = 0; i < _imageWidth; ++i)
			{
				if (job)
				{
					if (job.IsCancelled())
					{
						return true;
					}
				}
				const color& pixel_color = _progressiveSamples[j * _imageWidth + i];
				write_color(i, _imageHeight - j - 1, _image, pixel_color, samplesPerPixel, _videopostBuffer);
			}

			if (_area)
			{
				_area->Redraw(true);
			}
		}
	}
	return true;
}

void Raytracer::SetProgressiveSampleCount(Int32 samples)
{
	_progressiveSampleCount = samples;
}

Bool Raytracer::RaytraceProgressiveTile(maxon::JobRef job, Int32 tileIndex)
{
	Tile* tile = _image->GetTile(tileIndex);
	if (!tile)
		return false;

	UInt32 cameraDirty = 0;
	MAXON_SCOPE
	{
		BaseObject * pCamera = GetCamera();
		if (pCamera)
		{
			cameraDirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
		}
	}

	Int32 tileSizeX = _image->GetNumTilesX();
	Int32 xTile = tileIndex % tileSizeX;
	Int32 yTile = tileIndex / tileSizeX;
	Int32 xOff = xTile * TILESIZE;
	Int32 yOff = yTile * TILESIZE;

	Int32 maxY = maxon::ClampValue(yOff + TILESIZE, 0, _imageHeight);
	Int32 maxX = maxon::ClampValue(xOff + TILESIZE, 0, _imageWidth);

	BaseTime time = _doc->GetTime();

	for (Int32 j = yOff; j < maxY; j++)
	{
		for (Int32 i = xOff; i < maxX; ++i)
		{
			BaseObject* pCamera = GetCamera();
			if (pCamera)
			{
				UInt32 dirty = pCamera->GetDirty(DIRTYFLAGS::MATRIX | DIRTYFLAGS::DATA);
				if (dirty != cameraDirty)
				{
					return true;
				}
			}

			if (time != _doc->GetTime())
			{
				return true;
			}

			if (job)
			{
				if (job.IsCancelled())
				{
					return true;
				}
			}

			if (_videopostThread && _videopostThread->TestBreak())
			{
				if (job)
				{
					if (job.GetJobGroup())
					{
						job.GetJobGroup()->Cancel();
					}
					job.Cancel();
				}
				return true;
			}

			auto u = (i + random_double()) / (_imageWidth - 1);
			auto v = (j + random_double()) / (_imageHeight - 1);
			ray r = _cam.get_ray(u, v);
			_progressiveSamples[j * _imageWidth + i] += ray_color(r, _background, _useDomeBackground, _world, _maxDepth);
		}

		for (int i = xOff; i < maxX; ++i)
		{
			if (job)
			{
				if (job.IsCancelled())
				{
					return true;
				}
			}
			const color& pixel_color = _progressiveSamples[j * _imageWidth + i];
			write_color(i, _imageHeight - j - 1, _image, pixel_color, _progressiveSampleCount, _videopostBuffer);
		}
		if (_area)
		{
			_area->Redraw(true);
		}
	}

	return true;
}

void Raytracer::ClearSamples()
{
	ClearMemType<color>(_progressiveSamples, _imageWidth * _imageHeight);
}

void Raytracer::SetVideoPost(VPBuffer* buffer, BaseThread* pThread, Bool mainViewport)
{
	_videopostThread = pThread;
	_videopostBuffer = buffer;
	_mainViewport = mainViewport;
}

void Raytracer::SetDocument(BaseDocument* doc)
{
	_doc = doc;
}

void Raytracer::SetSamplesPerPixel(Int32 samples)
{
	_samplesPerPixel = samples;
}

void Raytracer::SetMaxDepth(Int32 maxDepth)
{
	_maxDepth = maxDepth;
}