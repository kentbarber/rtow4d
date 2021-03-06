#ifndef MFUNRAYMATERIAL_H__
#define MFUNRAYMATERIAL_H__

enum
{	
	FUNRAYMATERIAL_TYPE = 1000,
	FUNRAYMATERIAL_TYPE_LAMBERT = 0,
	FUNRAYMATERIAL_TYPE_METAL = 1,	
	FUNRAYMATERIAL_TYPE_DIELECTRIC = 2,		
	FUNRAYMATERIAL_TYPE_ISOTROPIC = 3,		
	FUNRAYMATERIAL_TYPE_DIFFUSE_LIGHT = 100,	
	
	FUNRAYMATERIAL_COLOR = 1001,
	FUNRAYMATERIAL_FUZZ = 1002,
	FUNRAYMATERIAL_IOR = 1003,
	
	FUNRAYMATERIAL_COLOR_TEXTURE = 2000,
	
	FUNRAYMATERIAL_MAT_PREVIEW  = 10000,
};

#endif // MFUNRAYMATERIAL_H__
