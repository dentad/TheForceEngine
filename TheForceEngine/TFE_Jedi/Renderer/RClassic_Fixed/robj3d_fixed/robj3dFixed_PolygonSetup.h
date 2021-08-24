#pragma once
//////////////////////////////////////////////////////////////////////
// Wall
// Dark Forces Derived Renderer - Wall functions
//////////////////////////////////////////////////////////////////////
#include <TFE_System/types.h>
#include <TFE_Asset/modelAsset_Jedi.h>
#include <TFE_Jedi/Math/fixedPoint.h>
#include <TFE_Jedi/Math/core_math.h>

#define POLY_MAX_VTX_COUNT 32

using namespace TFE_CoreMath;

namespace TFE_JediRenderer
{
	namespace RClassic_Fixed
	{
		extern vec3_fixed s_polygonVerticesVS[POLY_MAX_VTX_COUNT];
		extern vec3_fixed s_polygonVerticesProj[POLY_MAX_VTX_COUNT];
		extern vec2_fixed s_polygonUv[POLY_MAX_VTX_COUNT];
		extern fixed16_16 s_polygonIntensity[POLY_MAX_VTX_COUNT];

		void robj3d_setupPolygon(Polygon* polygon);
	}
}