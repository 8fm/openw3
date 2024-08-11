/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockPackNormal : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockPackNormal, CMaterialBlock, "Image Processing", "Pack/Unpack Normal" );

public:
	bool	m_unpack;

public:
	CMaterialBlockPackNormal();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockPackNormal )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_unpack, TXT("Set true to unpack normals") );
END_CLASS_RTTI()

#endif