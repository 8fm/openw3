/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// Sample terrain material
class CMaterialTerrainMaterialBlending : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialTerrainMaterialBlending, CMaterialBlock, "Samplers", "Terrain Material Blending" );

public:
	CMaterialTerrainMaterialBlending();

	Float m_uvScales[7];

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket ) const;
};

BEGIN_CLASS_RTTI(CMaterialTerrainMaterialBlending)
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_uvScales, TXT("UV Scales Legend") )
END_CLASS_RTTI()

#endif