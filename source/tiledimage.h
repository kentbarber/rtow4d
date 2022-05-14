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

#ifndef GLD_TILEDIMAGE_H__
#define GLD_TILEDIMAGE_H__

#include "c4d.h"
#include "maxon/pointerarray.h"

#define TILESIZE 64

class Tile
{
public:
	Tile();
	~Tile();

	Bool Init();

	Vector32 GetPixel(Int32 x, Int32 y) const;
	void SetPixel(Int32 x, Int32 y, const Vector32& col, Bool setDirtyFlag = true);

	UInt GetDirty();
	void SetDirty();

	Bool IsDirty();
	void ClearDirty();

private:
	UInt _dirty;
	UInt _lastDirty;
	Vector32* _data;
	Vector32 _baseColor;
};

class TiledImage
{
public:
	TiledImage();
	~TiledImage();

	Bool Init(Int32 _Width, Int32 _Height);
	void Clear();

	Int32 GetWidth();
	Int32 GetHeight();

	Int32 GetNumTilesX() const;
	Int32 GetNumTilesY() const;
	Int32 GetNumTiles() const;

	Tile* GetTile(Int32 x, Int32 _Y);
	Tile* GetTile(Int32 tileIndex);

	Vector32 GetPixel(Int32 x, Int32 y);
	void SetPixel(Int32 x, Int32 y, const Vector32& col);

	void DirtyAll();

private:
	Int32 _width;
	Int32 _height;
	Int32 _numTilesX;
	Int32 _numTilesY;

	maxon::PointerArray<Tile> _tiles;
};

#endif