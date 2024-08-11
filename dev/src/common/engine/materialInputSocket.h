/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#include "graphSocket.h"
#include "graphBlock.h"
#include "materialCompiler.h"

class CMaterialOutputSocket;
class CMaterialBlockCompiler;

/// Material block input socket
class CMaterialInputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialInputSocket );

public:
	//! Get target material output socket we are connected to
	CMaterialOutputSocket* GetDestinationSocket() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
#endif

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Compile socket input value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, const CodeChunk& defaultValue = CodeChunk::EMPTY ) const;

#endif // NO_RUNTIME_MATERIAL_COMPILATION

};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialInputSocket, CGraphSocket );

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material input socket spawn info
class MaterialInputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	MaterialInputSocketSpawnInfo( const CName& name, Bool isVisible=true );
};

#endif // NO_EDITOR_GRAPH_SUPPORT
