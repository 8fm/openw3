/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

typedef Uint16 TControlMapType;
typedef Uint32 TColorMapType;
typedef Uint64 TColorMapRawType;

// Default colormap value is 50% grey, which produces no darkening or brightening.
#define DEFAULT_COLOR_MAP_VALUE ( (TColorMapType)0xff808080 )

#define DEFAULT_COLOR_MAP_RAW_VALUE ( (TColorMapRawType)0x0000000084108410ULL )


enum ETerrainBufferType
{
	TBT_HeightMap = FLAG( 0 ),
	TBT_ControlMap = FLAG( 1 ),
	TBT_ColorMap = FLAG( 2 )
};
