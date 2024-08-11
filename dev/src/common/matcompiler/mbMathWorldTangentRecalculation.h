/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockWorldTangentRecalculation : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWorldTangentRecalculation, CMaterialBlock, "Image Processing", "World/Tangent" );

public:
	bool	m_worldToTangent;

public:
	CMaterialBlockWorldTangentRecalculation();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockWorldTangentRecalculation )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_worldToTangent, TXT("Set true to recalculate World to Tangent basis") );
END_CLASS_RTTI()

#endif