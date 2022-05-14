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

#include "raytracerarea.h"
#include "raytracer.h"

RaytracerArea::RaytracerArea()
{

}

RaytracerArea::~RaytracerArea()
{
}

Bool RaytracerArea::GetMinSize(Int32& w, Int32& h)
{
	w = 512;
	h = 288;

	if (!_tiledImage.Init(w, h))
	{
		DebugAssert(false);
		return false;
	}

	if (_img->Init(w, h, 32) != IMAGERESULT::OK)
	{ 
		DebugAssert(false);
		return false;
	}

	return true;
}

Bool RaytracerArea::Render(maxon::JobGroupRef jobGroup, RENDERMODE renderMode)
{
	return SetupRenderer(jobGroup, renderMode, &_raytracer, &_tiledImage, this);
}

void RaytracerArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
	SetClippingRegion(0, 0, GetWidth(), GetHeight());

	Int32 numTiles = _tiledImage.GetNumTiles();
	Int32 numTilesx = _tiledImage.GetNumTilesX();

	for (Int a = 0; a < numTiles; a++)
	{
		Tile* pTile = _tiledImage.GetTile(a);
		Int xTile = a % numTilesx;
		Int yTile = a / numTilesx;
		if (pTile && pTile->IsDirty())
		{
			pTile->ClearDirty();
			for (Int y = 0; y < TILESIZE; y++)
			{
				for (Int x = 0; x < TILESIZE; x++)
				{
					Vector32 pixel = pTile->GetPixel(x, y);
					_img->SetPixelCnt(xTile * TILESIZE + x, yTile * TILESIZE + y, 1, (UChar*)&pixel, 24,COLORMODE::RGBf, PIXELCNT::NONE);
				}
			}
		}
	}

	Int32 startX = (GetWidth() - _img->GetBw()) / 2;
	Int32 starty = (GetHeight() - _img->GetBh()) / 2;

	DrawBitmap(_img, startX + x1, starty + y1, _img->GetBw(), _img->GetBh(), 0, 0, _img->GetBw(), _img->GetBh(), BMP_NORMAL);
}

void RaytracerArea::Clear()
{
	if (_img)
	{
		_img->Clear(0, 0, 0);
	}
	_tiledImage.Clear();
	Redraw();
}
