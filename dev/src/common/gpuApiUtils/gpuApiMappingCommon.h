/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "gpuApiTypes.h"
#include "gpuApiErrorHandling.h"
#include "../redMath/numericalutils.h"

namespace GpuApi
{
	/// Convert number of draw elements (vertices or indices) to number of primitives, based on primitive type.
	inline Int32 MapDrawElementCountToPrimitiveCount( ePrimitiveType primitiveType, Int32 elementCount )
	{
		switch ( primitiveType )
		{
		case PRIMTYPE_LineList:			return elementCount / 2;
		case PRIMTYPE_LineStrip:		return elementCount - 1;
		case PRIMTYPE_TriangleList:		return elementCount / 3;
		case PRIMTYPE_TriangleStrip:	return elementCount - 2;
		case PRIMTYPE_PointList:		return elementCount;
		case PRIMTYPE_QuadList:			return elementCount / 4;
		case PRIMTYPE_1CP_PATCHLIST:	return elementCount;
		case PRIMTYPE_3CP_PATCHLIST:	return elementCount / 3;
		case PRIMTYPE_4CP_PATCHLIST:	return elementCount / 4;
		case PRIMTYPE_Invalid:			GPUAPI_HALT("");
		};

		return 0;
	}

	/// Convert number of render primitives to the appropriate number of vertices or indices.
	inline Int32 MapPrimitiveCountToDrawElementCount( ePrimitiveType primitiveType, Int32 primitiveCount )
	{
		switch ( primitiveType )
		{
		case PRIMTYPE_LineList:			return primitiveCount * 2;
		case PRIMTYPE_LineStrip:		return primitiveCount + 1;
		case PRIMTYPE_TriangleList:		return primitiveCount * 3;
		case PRIMTYPE_TriangleStrip:	return primitiveCount + 2;
		case PRIMTYPE_PointList:		return primitiveCount;		
		case PRIMTYPE_QuadList:			return primitiveCount * 4;
		case PRIMTYPE_1CP_PATCHLIST:	return primitiveCount;		
		case PRIMTYPE_3CP_PATCHLIST:	return primitiveCount * 3;
		case PRIMTYPE_4CP_PATCHLIST:	return primitiveCount * 4;
		case PRIMTYPE_Invalid:			GPUAPI_HALT("");
		};

		return 0;
	}

	inline Uint32 MapAnisotropy( eTextureFilterMin filter, Uint32 maxAnisotropy )
	{
		using namespace Red::Math::NumericalUtils;
		switch (filter)
		{
		case TEXFILTERMIN_Aniso:		return maxAnisotropy;
		case TEXFILTERMIN_AnisoLow:		return Min( Max( maxAnisotropy/4u, 2u ) , maxAnisotropy );	// Not less than 2 if there is aniso, 1 if aniso is off
		default:						return 1;
		}
	}

	inline Bool IsTextureFormatDXT( eTextureFormat format )
	{
		return TEXFMT_BC1 == format 
			|| TEXFMT_BC2 == format 
			|| TEXFMT_BC3 == format 
			|| TEXFMT_BC4 == format 
			|| TEXFMT_BC5 == format 
			|| TEXFMT_BC6H == format 
			|| TEXFMT_BC7 == format;
	}

	inline Bool IsTextureSizeValidForFormat( Uint32 width, Uint32 height, eTextureFormat format )
	{
		return width > 0 && height > 0 && !(IsTextureFormatDXT(format) && (width % 4 || height % 4));
	}
}
