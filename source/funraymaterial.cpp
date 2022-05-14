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

#include "funraymaterial.h"
#include "mfunraymaterial.h"
#include "customgui_matpreview.h"
#include "c4d_symbols.h"

Bool FunRayMaterial::Init(GeListNode* node)
{
	BaseContainer* data = ((BaseMaterial*)node)->GetDataInstance();
	data->SetInt32(FUNRAYMATERIAL_TYPE, FUNRAYMATERIAL_TYPE_LAMBERT);
	data->SetVector(FUNRAYMATERIAL_COLOR, Vector(1.0));
	data->SetFloat(FUNRAYMATERIAL_FUZZ, 0);
	data->SetFloat(FUNRAYMATERIAL_IOR, 1.5);

	updatecount = 0;

	GeData previewData(CUSTOMDATATYPE_MATPREVIEW, DEFAULTVALUE);
	MaterialPreviewData* preview = (MaterialPreviewData*)previewData.GetCustomDataType(CUSTOMDATATYPE_MATPREVIEW);
	if (!preview)
		return false;

	data->SetData(FUNRAYMATERIAL_MAT_PREVIEW, previewData);

	return true;
}

INITRENDERRESULT FunRayMaterial::InitRender(BaseMaterial* mat, const InitRenderStruct& irs)
{
	BaseContainer* data = mat->GetDataInstance();
	color = data->GetVector(FUNRAYMATERIAL_COLOR);
	return INITRENDERRESULT::OK;
}

static void SimpleIllumModel(VolumeData* sd, RayLightCache* rlc, void* dat)
{
	Bool	nodif, nospec;
	Int32	i;
	Float	cosa, cosb, exponent = 5.0;
	const Vector64& v = sd->ray->v;

	rlc->diffuse = rlc->specular = Vector(0.0);

	for (i = 0; i < rlc->cnt; i++)
	{
		RayLightComponent* lc = rlc->comp[i];
		if (lc->lv.IsZero())
			continue;		// light invisible

		RayLight* ls = lc->light;

		nodif = nospec = false;
		if (ls->lr.object)
			CalcRestrictionInc(&ls->lr, sd->op, nodif, nospec);

		lc->rdiffuse = lc->rspecular = Vector(0.0);
		if (ls->ambient)
		{
			lc->rdiffuse = Vector(1.0);
		}
		else if (ls->arealight)
		{
			sd->CalcArea(ls, nodif, nospec, exponent, v, sd->p, sd->bumpn, sd->orign, sd->raybits, false, &lc->rdiffuse, &lc->rspecular);
		}
		else
		{
			cosa = Dot(sd->bumpn, lc->lv);
			if (!(ls->nodiffuse || nodif) && sd->cosc * cosa >= 0.0)
			{
				Float trn = ls->trn;
				if (trn != 1.0)
					lc->rdiffuse = Vector(Pow(Abs(cosa), trn));
				else
					lc->rdiffuse = Vector(Abs(cosa));
			}

			if (!(ls->nospecular || nospec))
			{
				cosb = Dot(v, lc->lv - sd->bumpn * (2.0 * cosa));

				if (cosb > 0.0)
					lc->rspecular = Vector(Pow(cosb, exponent));
			}
		}

		rlc->diffuse	+= lc->rdiffuse * lc->col;
		rlc->specular += lc->rspecular * lc->col;
	}
}

void FunRayMaterial::CalcSurface(BaseMaterial* mat, VolumeData* vd)
{
	Vector diff, spec, att_spc, att_dif;
	Int32	 i;

	//sd->Illuminance1(&diff,&spec,5.0);
	vd->IlluminanceSimple(&diff, &spec, 0.0, SimpleIllumModel, this);	// replace standard model by custom model

	att_spc = Vector(0.5 + 0.5 * Turbulence(vd->uvw * 2.5, 4.0, true));
	att_dif = att_spc * color;

	vd->col = (att_dif * (diff + vd->ambient)) + (att_spc * spec);

	// process multipass data
	Multipass* buf = vd->multipass;
	if (!buf)
		return;

	*buf->vp_mat_color = att_dif;
	*buf->vp_mat_specularcolor = att_spc;
	*buf->vp_mat_specular	= 0.4;	// 2.0/exponent (or similar value)

	// values have only to be filled if != 0.0
	// *buf->vp_mat_luminance			= 0.0;
	// *buf->vp_mat_environment		= 0.0;
	// *buf->vp_mat_transparency		= 0.0;
	// *buf->vp_mat_reflection			= 0.0;
	// *buf->vp_mat_diffusion			= 0.0;

	// calculate ambient component
	*buf->vp_ambient = att_dif * vd->ambient;

	// attenuate diffuse components
	for (i = 0; i < buf->diffuse_cnt; i++)
		*buf->diffuse[i] = att_dif * (*buf->diffuse[i]);

	// attenuate specular components
	for (i = 0; i < buf->specular_cnt; i++)
		*buf->specular[i] = att_spc * (*buf->specular[i]);
}


Bool FunRayMaterial::GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags)
{
	BaseContainer* data = ((BaseMaterial*)node)->GetDataInstance();


	switch (id[0].id)
	{
		case FUNRAYMATERIAL_MAT_PREVIEW:
			return GetDParameterPreview(data, (GeData*)&t_data, flags, FUNRAYMATERIAL_MAT_PREVIEW, updatecount, (BaseMaterial*)node);
			break;
	}

	return MaterialData::GetDParameter(node, id, t_data, flags);
}

Bool FunRayMaterial::SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags)
{
	BaseContainer* data = ((BaseMaterial*)node)->GetDataInstance();

	updatecount++;

	switch (id[0].id)
	{
		case FUNRAYMATERIAL_MAT_PREVIEW:
			return SetDParameterPreview(data, &t_data, flags, FUNRAYMATERIAL_MAT_PREVIEW);
			break;
	}

	return MaterialData::SetDParameter(node, id, t_data, flags);
}

Bool FunRayMaterial::Message(GeListNode* node, Int32 type, void* data)
{
	if (type == MSG_UPDATE)
		updatecount++;

	switch (type)
	{
		case MATPREVIEW_GET_OBJECT_INFO:
		{
			MatPreviewObjectInfo* info = (MatPreviewObjectInfo*)data;
			info->bHandlePreview = true;		// own preview handling
			info->bNeedsOwnScene = true;		// we need our own entry in the preview scene cache
			info->bNoStandardScene = false;	// we modify the standard scene
			info->lFlags = 0;
			return true;
			break;
		}
		case MATPREVIEW_MODIFY_CACHE_SCENE:
			// modify the preview scene here. We have a pointer to a scene inside the preview scene cache.
			// our scene contains the object
		{
			MatPreviewModifyCacheScene* scene = (MatPreviewModifyCacheScene*)data;
			// get the striped plane from the preview
			BaseObject* plane = scene->pDoc->SearchObject("Polygon"_s);
			if (plane)
				plane->SetRelScale(Vector(0.1));	// scale it a bit
			return true;
			break;
		}
		case MATPREVIEW_PREPARE_SCENE:
			// let the preview handle the rest...
			return true;
			break;
		case MATPREVIEW_GENERATE_IMAGE:
		{
			MatPreviewGenerateImage* image = (MatPreviewGenerateImage*)data;
			if (image->pDoc)
			{
				Int32					w = image->pDest->GetBw();
				Int32					h = image->pDest->GetBh();
				BaseContainer bcRender = image->pDoc->GetActiveRenderData()->GetData();
				bcRender.SetFloat(RDATA_XRES, w);
				bcRender.SetFloat(RDATA_YRES, h);
				bcRender.SetInt32(RDATA_ANTIALIASING, ANTI_GEOMETRY);
				if (image->bLowQuality)
					bcRender.SetInt32(RDATA_RENDERENGINE, RDATA_RENDERENGINE_PREVIEWHARDWARE);
				image->pDest->Clear(0, 0, 0);
				image->lResult = RenderDocument(image->pDoc, bcRender, nullptr, nullptr, image->pDest,
													 RENDERFLAGS::EXTERNAL | RENDERFLAGS::PREVIEWRENDER, image->pThread);
			}
			return true;
			break;
		}

		case MATPREVIEW_GET_PREVIEW_ID:
			*((Int32*)data) = FUNRAYMATERIAL_MAT_PREVIEW;
			return true;
	}

	return true;
}

Bool FunRayMaterial::CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn)
{
	((FunRayMaterial*)dest)->updatecount = updatecount;
	return NodeData::CopyTo(dest, snode, dnode, flags, trn);
}

Bool FunRayMaterial::GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc)
{
	Int32 type = ((BaseMaterial*)node)->GetDataInstance()->GetInt32(FUNRAYMATERIAL_TYPE);

	switch (id[0].id)
	{
	case FUNRAYMATERIAL_COLOR:
	case FUNRAYMATERIAL_COLOR_TEXTURE:
		return (type == FUNRAYMATERIAL_TYPE_LAMBERT || type == FUNRAYMATERIAL_TYPE_METAL || type == FUNRAYMATERIAL_TYPE_ISOTROPIC || type == FUNRAYMATERIAL_TYPE_DIFFUSE_LIGHT);
	case FUNRAYMATERIAL_FUZZ:
		return type == FUNRAYMATERIAL_TYPE_METAL;
	case FUNRAYMATERIAL_IOR:
		return type == FUNRAYMATERIAL_TYPE_DIELECTRIC;
	}
	return SUPER::GetDEnabling(node, id, t_data, flags, itemdesc);
}

Bool RegisterFunRayMaterial()
{
	String name = GeGetDefaultFilename(DEFAULTFILENAME_SHADER_VOLUME) + GeLoadString(IDS_FUNRAYMATERIAL);	// place in default Shader section

	// add a preview scene that can only be selected in the Simple Material's preview
	//AddUserPreviewScene(GeGetPluginResourcePath() + String("scene") + String("Stairs.c4d"), GLD_ID_FUNRAYMATERIAL, nullptr);

	return RegisterMaterialPlugin(GLD_ID_FUNRAYMATERIAL, name, 0, FunRayMaterial::Alloc, "Mfunraymaterial"_s, 0);
}
