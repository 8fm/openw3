/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#include "graphSocket.h"
#include "graphBlock.h"
#include "materialCompiler.h"

class CMaterialBlockCompiler;

/// Output socket that outputs texture reference
class CMaterialOutputTextureSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialOutputTextureSocket );

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	Int32 GetShaderTarget() const;

	// Compile socket output value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif // NO_RUNTIME_MATERIAL_COMPILATION
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialOutputTextureSocket, CGraphSocket );

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material output socket spawn info
class MaterialOutputTextureSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	MaterialOutputTextureSocketSpawnInfo( const CName& name );
};

#endif