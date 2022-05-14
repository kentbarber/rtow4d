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

#include "c4d.h"
#include "funraymaterial.h"
#include "mfunraymaterial.h"
#include "funraycamera.h"

#define GLD_ID_RTOWTESTSCENE_CMD 1058686

const double pi = 3.1415926535897932385;

inline double degrees_to_radians(double degrees) {
	return degrees * pi / 180.0;
}

inline double clamp(double x, double min, double max) {
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

inline double random_double() {
	// Returns a random real in [0,1).
	return rand() / (RAND_MAX + 1.0);
}

inline double random_double(double min, double max) {
	// Returns a random real in [min,max).
	return min + (max - min) * random_double();
}

inline int random_int(int min, int max) {
	// Returns a random integer in [min,max].
	return static_cast<int>(random_double(min, max + 1));
}

inline static Vector color_random(double min, double max) {
	return Vector(random_double(min, max), random_double(min, max), random_double(min, max));
}

inline static Vector color_random() {
	return Vector(random_double(), random_double(), random_double());
}

class TestSceneCmd : public CommandData
{
#if (API_VERSION >= 21000)
	virtual Bool Execute(BaseDocument *doc, GeDialog* parentManager);
#else
	virtual Bool Execute(BaseDocument* doc);
#endif

	void AddSphere(BaseDocument* doc, const Vector& pos, Float radius, BaseMaterial* pMat, BaseObject* pParent);
};

void TestSceneCmd::AddSphere(BaseDocument *doc, const Vector& pos, Float radius, BaseMaterial *pMat, BaseObject *pParent)
{
	radius *= 100;
	BaseObject* pSphere = BaseObject::Alloc(Osphere);
	pSphere->MakeTag(Tphong);

	Matrix mg;
	mg.off = pos * 100;
	mg.off.z *= -1;
	pSphere->SetMg(mg);

	BaseContainer* bc = pSphere->GetDataInstance();
	bc->SetFloat(PRIM_SPHERE_RAD, radius);

	Vector albedo(0.5);
	if (pMat)
	{
		TextureTag* pTag = (TextureTag*)pSphere->MakeTag(Ttexture);
		pTag->SetMaterial(pMat);

		GeData d;
		pMat->GetParameter(DescID(FUNRAYMATERIAL_COLOR), d, DESCFLAGS_GET::NONE);
		albedo = d.GetVector();
	}

	ObjectColorProperties prop;
	pSphere->GetColorProperties(&prop);
	prop.color = albedo;
	prop.usecolor = ID_BASEOBJECT_USECOLOR_ALWAYS;

	pSphere->SetColorProperties(&prop);

	pSphere->Message(MSG_UPDATE);
	doc->InsertObject(pSphere, pParent, nullptr);

	doc->AddUndo(UNDOTYPE::NEWOBJ, pSphere);
}

#if (API_VERSION >= 21000)
Bool TestSceneCmd::Execute(BaseDocument *doc, GeDialog* parentManager)
#else
Bool TestSceneCmd::Execute(BaseDocument* doc)
#endif
{
	doc->StartUndo();

	MAXON_SCOPE 
	{
		BaseDraw* bd = doc->GetActiveBaseDraw();
		CameraObject* pEditorCamera = (CameraObject*)bd->GetEditorCamera();

		CameraObject* pCamera = (CameraObject * )pEditorCamera->GetClone(COPYFLAGS::NONE, nullptr);
		doc->InsertObject(pCamera, nullptr, nullptr);
		doc->AddUndo(UNDOTYPE::NEWOBJ, pCamera);

		bd->SetSceneCamera(pCamera);

		Matrix camMg;
		camMg.off = Vector(13, 2, -3) * 100.0;

		Vector look(0, 0, 0);
		Vector up(0, 1, 0);
		Vector w = (look - camMg.off).GetNormalized();
		Vector u = Cross(up, w).GetNormalized();
		Vector v = Cross(w, u).GetNormalized();

		camMg.sqmat.v1 = u;
		camMg.sqmat.v2 = v;
		camMg.sqmat.v3 = w;

		pCamera->SetMg(camMg);

		pCamera->SetParameter(DescID(CAMERAOBJECT_FOV_VERTICAL), maxon::DegToRad(20.0), DESCFLAGS_SET::FORCESET);
		pCamera->SetParameter(DescID(CAMERAOBJECT_TARGETDISTANCE), 1000.0, DESCFLAGS_SET::FORCESET);
	}

	BaseObject* pNull = BaseObject::Alloc(Onull);
	doc->AddUndo(UNDOTYPE::NEWOBJ, pNull);
	doc->InsertObject(pNull, nullptr, nullptr);

	BaseMaterial* pGroundMat= BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
	if (pGroundMat)
	{
		pGroundMat->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_LAMBERT, DESCFLAGS_SET::FORCESET);
		pGroundMat->SetParameter(DescID(FUNRAYMATERIAL_COLOR), Vector(0.5), DESCFLAGS_SET::FORCESET);

		doc->InsertMaterial(pGroundMat);
		doc->AddUndo(UNDOTYPE::NEWOBJ, pGroundMat);
	}

	AddSphere(doc, Vector(0, -1000, 0), 1000, pGroundMat, pNull);

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			auto choose_mat = random_double();
			Vector center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

			if ((center - Vector(4, 0.2, 0)).GetLength() > 0.9) {
				if (choose_mat < 0.8) {
					// diffuse
					Vector albedo = color_random() * color_random();

					BaseMaterial* pMat = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
					if (pMat)
					{
						pMat->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_LAMBERT, DESCFLAGS_SET::FORCESET);
						pMat->SetParameter(DescID(FUNRAYMATERIAL_COLOR), albedo, DESCFLAGS_SET::FORCESET);
						doc->InsertMaterial(pMat);
						doc->AddUndo(UNDOTYPE::NEWOBJ, pMat);
					}

					AddSphere(doc, center, 0.2, pMat, pNull);
				}
				else if (choose_mat < 0.95) {
					// metal
					Vector albedo = color_random(0.5, 1);
					Float fuzz = random_double(0, 0.5);

					BaseMaterial* pMat = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
					if (pMat)
					{
						pMat->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_METAL, DESCFLAGS_SET::FORCESET);
						pMat->SetParameter(DescID(FUNRAYMATERIAL_COLOR), albedo, DESCFLAGS_SET::FORCESET);
						pMat->SetParameter(DescID(FUNRAYMATERIAL_FUZZ), fuzz, DESCFLAGS_SET::FORCESET);

						doc->InsertMaterial(pMat);
						doc->AddUndo(UNDOTYPE::NEWOBJ, pMat);
					}
					AddSphere(doc, center, 0.2, pMat, pNull);
				}
				else {
					BaseMaterial* pMat = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
					if (pMat)
					{
						pMat->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_DIELECTRIC, DESCFLAGS_SET::FORCESET);
						pMat->SetParameter(DescID(FUNRAYMATERIAL_IOR), 1.5, DESCFLAGS_SET::FORCESET);

						doc->InsertMaterial(pMat);
						doc->AddUndo(UNDOTYPE::NEWOBJ, pMat);
					}
					// glass
					AddSphere(doc, center, 0.2, pMat, pNull);
				}
			}
		}
	}

	BaseMaterial* pGlass = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
	if (pGlass)
	{
		pGlass->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_DIELECTRIC, DESCFLAGS_SET::FORCESET);
		pGlass->SetParameter(DescID(FUNRAYMATERIAL_IOR), 1.5, DESCFLAGS_SET::FORCESET);

		doc->InsertMaterial(pGlass);
		doc->AddUndo(UNDOTYPE::NEWOBJ, pGlass);
	}
	AddSphere(doc, Vector(0, 1, 0), 1, pGlass, pNull);

	BaseMaterial* pLambert = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
	if (pLambert)
	{
		pLambert->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_LAMBERT, DESCFLAGS_SET::FORCESET);
		pLambert->SetParameter(DescID(FUNRAYMATERIAL_COLOR), Vector(0.4, 0.2, 0.1), DESCFLAGS_SET::FORCESET);

		doc->InsertMaterial(pLambert);
		doc->AddUndo(UNDOTYPE::NEWOBJ, pLambert);
	}
	AddSphere(doc, Vector(-4, 1, 0), 1.0, pLambert, pNull);

	BaseMaterial* pMetal = BaseMaterial::Alloc(GLD_ID_FUNRAYMATERIAL);
	if (pMetal)
	{
		pMetal->SetParameter(DescID(FUNRAYMATERIAL_TYPE), FUNRAYMATERIAL_TYPE_METAL, DESCFLAGS_SET::FORCESET);
		pMetal->SetParameter(DescID(FUNRAYMATERIAL_COLOR), Vector(0.7, 0.6, 0.5), DESCFLAGS_SET::FORCESET);
		pMetal->SetParameter(DescID(FUNRAYMATERIAL_FUZZ), 0.0, DESCFLAGS_SET::FORCESET);

		doc->InsertMaterial(pMetal);
		doc->AddUndo(UNDOTYPE::NEWOBJ, pMetal);
	}
	AddSphere(doc, Vector(4, 1, 0), 1.0, pMetal, pNull);

	doc->EndUndo();
	EventAdd();
	return true;
}

Bool RegisterTestSceneCmd()
{
	return RegisterCommandPlugin(GLD_ID_RTOWTESTSCENE_CMD, "Test Scene 1"_s , 0, nullptr, String(), NewObjClear(TestSceneCmd));
}