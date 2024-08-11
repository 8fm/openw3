/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "materialOutputSocket.h"

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

/// Output socket that outputs bool reference
class CMaterialOutputBoolSocket : public CMaterialOutputSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialOutputBoolSocket );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );

#endif
	
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	Int32 GetShaderTarget() const;

	// Compile socket output value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif

};

DEFINE_SIMPLE_RTTI_CLASS( CMaterialOutputBoolSocket, CMaterialOutputSocket );

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material output socket spawn info
class MaterialOutputBoolSocketSpawnInfo : public MaterialOutputSocketSpawnInfo
{
public:
	MaterialOutputBoolSocketSpawnInfo( const CName& name );
};

#endif // NO_EDITOR_GRAPH_SUPPORT
