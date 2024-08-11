/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/materialBlock.h"

enum EFrameBasisTypes
{
	Tangent,
	Binormal,
	Normal
};

BEGIN_ENUM_RTTI( EFrameBasisTypes );
ENUM_OPTION( Tangent );
ENUM_OPTION( Binormal );
ENUM_OPTION( Normal );
END_ENUM_RTTI();

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

/// A material block that emits texture coordinates
class CMaterialBlockWorldBasis : public CMaterialBlock
{
	DECLARE_ENGINE_MATERIAL_BLOCK( CMaterialBlockWorldBasis, CMaterialBlock, "Input", "World Basis (TBN)" );

public:
	EFrameBasisTypes m_frameBasis;

public:
	CMaterialBlockWorldBasis();
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	virtual String GetCaption() const;
};

BEGIN_CLASS_RTTI( CMaterialBlockWorldBasis )
	PARENT_CLASS(CMaterialBlock)
	PROPERTY_EDIT( m_frameBasis, TXT("Choose the frame basis") );
END_CLASS_RTTI()

#endif