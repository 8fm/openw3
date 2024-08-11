/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"


#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockGammaCorrection : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockGammaCorrection, CMaterialBlock, "Image Processing", "Gamma/Linear" );

public:
	bool	m_linearToGamma;

public:
	CMaterialBlockGammaCorrection();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockGammaCorrection )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_linearToGamma, TXT("Set true to convert from Linear to Gamma") );
END_CLASS_RTTI()

#endif