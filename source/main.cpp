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

//Forward declaration
Bool RegisterRaytracerDialog();
Bool RegisterTestSceneCmd();
Bool RegisterFunRayMaterial();
Bool RegisterFunRayVideoPost();
Bool RegisterCameraTag();

Bool PluginStart()
{
	if (!RegisterRaytracerDialog()) return false;
	if (!RegisterTestSceneCmd()) return false;
	if (!RegisterFunRayMaterial()) return false;
	if (!RegisterFunRayVideoPost()) return false;
	if (!RegisterCameraTag()) return false;
	return true;
}

void PluginEnd()
{
}

Bool PluginMessage(Int32 id, void *data)
{
	switch (id)
	{
	case C4DPL_INIT_SYS:
		if (!g_resource.Init()) return false; // don't start plugin without resource
		return true;
	}
	return false;
}



