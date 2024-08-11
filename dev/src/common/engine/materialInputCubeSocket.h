/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#include "graphSocket.h"
#include "graphBlock.h"
#include "materialCompiler.h"

class CMaterialOutputCubeSocket;
class CMaterialBlockCompiler;

/// Material block input socket that accepts cubemap links
class CMaterialInputCubeSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialInputCubeSocket );

public:
	//! Get target material output socket we are connected to
	CMaterialOutputCubeSocket* GetDestinationSocket() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

#endif // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Compile socket input value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif // NO_RUNTIME_MATERIAL_COMPILATION
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialInputCubeSocket, CGraphSocket );

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material input socket spawn info
class MaterialInputCubeSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	MaterialInputCubeSocketSpawnInfo( const CName& name );
};

#endif // NO_EDITOR_GRAPH_SUPPORT
