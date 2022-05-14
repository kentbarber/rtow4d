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

#include "tiledimage.h"

//======================================

Random g_rand;

Tile::Tile() : _data(nullptr), _dirty(0), _lastDirty(0)
{
	_baseColor = Vector32(g_rand.Get01(), g_rand.Get01(), g_rand.Get01());
}

Tile::~Tile()
{
	if (_data)
	{
		DeleteMem(_data);
	}
}


Bool Tile::Init()
{
	if (_data == nullptr)
	{
		iferr(_data = NewMem(Vector32, TILESIZE * TILESIZE))
		{
			return false;
		}

		for (Int32 a = 0; a < TILESIZE * TILESIZE; a++)
		{
			_data[a] = _baseColor;
		}
		SetDirty();
	}

	return true;
}

void Tile::SetDirty()
{
	_dirty++;
}

UInt Tile::GetDirty()
{
	return _dirty;
}

Bool Tile::IsDirty()
{
	return _dirty != _lastDirty;
}

void Tile::ClearDirty()
{
	_lastDirty = _dirty;
}

void Tile::SetPixel(Int32 x, Int32 y, const Vector32& col, Bool setDirtyFlag)
{
	if (!Init() || !_data)
		return;

	if (x >= TILESIZE || y >= TILESIZE || x < 0 || y < 0)
	{
		DebugAssert(false);
		return;
	}

	_data[y * TILESIZE + x] = col;

	if (setDirtyFlag)
		SetDirty();
}

Vector32 Tile::GetPixel(Int32 x, Int32 y) const
{
	if (!_data)
		return _baseColor;

	if (x >= TILESIZE || y >= TILESIZE || x < 0 || y < 0)
	{
		DebugAssert(false);
		return Vector32(0);
	}
	
	return _data[y * TILESIZE + x];
}

//======================================

TiledImage::TiledImage()
{
	
}

TiledImage::~TiledImage()
{

}

Bool TiledImage::Init(Int32 width, Int32 height)
{
	_width = width;
	_height = height;

	_numTilesX = maxon::Ceil(Float(_width) / TILESIZE);
	_numTilesY = maxon::Ceil(Float(_height) / TILESIZE);

	Int32 totalTiles = _numTilesX * _numTilesY;

	for (Int32 i = 0; i < totalTiles; i++)
	{
		iferr(Tile & tile = _tiles.Append())
		{
			DebugAssert(false);
			return false;
		}
	}
	return true;
}

void TiledImage::Clear()
{
	_tiles.Reset();
}

Int32 TiledImage::GetWidth()
{
	return _width;
}

Int32 TiledImage::GetHeight()
{
	return _height;
}

Int32 TiledImage::GetNumTilesX() const
{
	return _numTilesX;
}

Int32 TiledImage::GetNumTilesY() const
{
	return _numTilesY;
}

Int32 TiledImage::GetNumTiles() const
{
	return _numTilesX * _numTilesY;
}

Tile* TiledImage::GetTile(Int32 x, Int32 y)
{
	if (x >= 0 && x < _numTilesX && y >= 0 && y < _numTilesY)
	{
		Int32 theTile = y * _numTilesX + x;
		return &_tiles[theTile];
	}
	return nullptr;
}

Tile* TiledImage::GetTile(Int32 tileIndex)
{
	if (tileIndex < GetNumTiles())
	{
		return &_tiles[tileIndex];
	}
	return nullptr;
}

void TiledImage::SetPixel(Int32 x, Int32 y, const Vector32& col)
{
	Int32 tileX = x / TILESIZE;
	Int32 tileY = y / TILESIZE;

	Int32 tileLocalX = x % TILESIZE;
	Int32 tileLocalY = y % TILESIZE;

	Tile* tile = GetTile(tileX, tileY);
	if (tile)
	{
		tile->SetPixel(tileLocalX, tileLocalY, col);
	}
}

Vector32 TiledImage::GetPixel(Int32 x, Int32 y)
{
	Int32 tileX = x / TILESIZE;
	Int32 tileY = y / TILESIZE;

	Int32 tileLocalX = x % TILESIZE;
	Int32 tileLocalY = y % TILESIZE;

	Tile* tile = GetTile(tileX, tileY);
	if (tile)
	{
		return tile->GetPixel(tileLocalX, tileLocalY);
	}

	return Vector32();

}

void TiledImage::DirtyAll()
{
	UInt count = _tiles.GetCount();
	for (UInt i = 0; i < count; i++)
	{
		_tiles[i].SetDirty();
	}
}