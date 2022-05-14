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

#ifndef FUNRAYMATERIAL_H__
#define FUNRAYMATERIAL_H__

#include "c4d.h"

#define GLD_ID_FUNRAYMATERIAL 1058689

class FunRayMaterial : public MaterialData
{
	INSTANCEOF(FunRayMaterial, MaterialData)

private:
	Vector color;

public:
	virtual Bool Init(GeListNode* node);
	virtual	void CalcSurface (BaseMaterial* mat, VolumeData* vd);
	virtual	INITRENDERRESULT InitRender(BaseMaterial* mat, const InitRenderStruct& irs);
	virtual Bool GetDParameter(GeListNode* node, const DescID& id, GeData& t_data, DESCFLAGS_GET& flags);
	virtual Bool SetDParameter(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_SET& flags);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);
	virtual Bool CopyTo(NodeData* dest, GeListNode* snode, GeListNode* dnode, COPYFLAGS flags, AliasTrans* trn);
	virtual Bool GetDEnabling(GeListNode* node, const DescID& id, const GeData& t_data, DESCFLAGS_ENABLE flags, const BaseContainer* itemdesc);

	static NodeData* Alloc() { return NewObjClear(FunRayMaterial); }

	Int32 updatecount;
};

#endif
