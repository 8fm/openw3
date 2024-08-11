/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/terrainStructs.h"
#include "renderProxyTerrain.h"

class CRenderGrassUpdateData : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderObjectSpeedtree )
protected:
	TDynArray< SAutomaticGrassDesc >	m_descriptors[ NUM_TERRAIN_TEXTURES_AVAILABLE ];

public:
	CRenderGrassUpdateData( const TDynArray< SAutomaticGrassDesc >* setup );
	~CRenderGrassUpdateData();

	const TDynArray< SAutomaticGrassDesc >* GetDescriptors()  const { return &m_descriptors[0]; }
};
