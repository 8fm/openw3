/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderGrassUpdateData.h"
#include "renderProxyTerrain.h"

CRenderGrassUpdateData::CRenderGrassUpdateData( const TDynArray< SAutomaticGrassDesc >* setup )
{
	for ( Uint32 i=0; i<NUM_TERRAIN_TEXTURES_AVAILABLE; ++i )
	{
		m_descriptors[i] = setup[i];
	}
}

CRenderGrassUpdateData::~CRenderGrassUpdateData()
{
}

IRenderObject* CRenderInterface::CreateGrassUpdateData( const TDynArray< struct SAutomaticGrassDesc >* automaticGrass )
{
	return new CRenderGrassUpdateData( automaticGrass );
}
