/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockTexCoords : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockTexCoords, CMaterialBlock, "Input", "Texture Coordinates" );

public:
	Int32			m_coordinatesIndex;

public:
	CMaterialBlockTexCoords();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
};

BEGIN_CLASS_RTTI( CMaterialBlockTexCoords )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_coordinatesIndex, TXT("Index of UV coordinates set") );
END_CLASS_RTTI()

#endif