/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockDerivativeNormal : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockDerivativeNormal, CMaterialBlock, "Image Processing", "Normal/Derivative" );

public:
	bool	m_derivativeToNormal;

public:
	CMaterialBlockDerivativeNormal();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockDerivativeNormal )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_derivativeToNormal, TXT("Set true to recalculate Derivative to Normal") );
END_CLASS_RTTI()

#endif