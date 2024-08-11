/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

enum ENoiseTypes
{
	Noise1D,
	Noise2D
};

BEGIN_ENUM_RTTI( ENoiseTypes );
ENUM_OPTION( Noise1D );
ENUM_OPTION( Noise2D );
END_ENUM_RTTI();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockNoise : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockNoise, CMaterialBlock, "System Samplers", "Noise" );

public:
	ENoiseTypes m_noiseType;

public:
	CMaterialBlockNoise();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	
};

BEGIN_CLASS_RTTI( CMaterialBlockNoise )
	PARENT_CLASS( CMaterialBlock )
	PROPERTY_EDIT( m_noiseType, TXT("Choose the noise type") );
END_CLASS_RTTI()

#endif