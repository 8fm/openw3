/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "materialInputSocket.h"
#include "graphBlock.h"


#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

class CMaterialOutputBoolSocket;

/// Material block input socket that accepts bool links
class CMaterialInputBoolSocket : public CMaterialInputSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialInputBoolSocket );

public:
	//! Get target material output socket we are connected to
	CMaterialOutputBoolSocket* GetDestinationSocket() const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
#endif // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Compile socket input value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
	using CMaterialInputSocket::Compile;
#endif // NO_RUNTIME_MATERIAL_COMPILATION
};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialInputBoolSocket, CMaterialInputSocket );

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material input socket spawn info
class MaterialInputBoolSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	MaterialInputBoolSocketSpawnInfo( const CName& name );
};

#endif
