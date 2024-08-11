/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#if !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )

#include "graphSocket.h"
#include "graphBlock.h"
#include "materialCompiler.h"

class CMaterialBlockCompiler;

/// Output socket for material block
class CMaterialOutputSocket : public CGraphSocket
{
	DECLARE_RTTI_SIMPLE_CLASS( CMaterialOutputSocket );

protected:
	String	m_swizzle;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Socket was added to block
	virtual void OnSpawned( class CGraphBlock* block, const class GraphSocketSpawnInfo& info );

	//! Can we connect to given socket
	virtual Bool CanConnectTo( CGraphSocket *otherSocket );
#endif // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

	Int32 GetShaderTarget() const;

	// Compile socket output value
	virtual CodeChunk Compile( CMaterialBlockCompiler& compiler, EMaterialShaderTarget shaderTarget, CMaterialOutputSocket* socket = NULL ) const;
#endif // NO_RUNTIME_MATERIAL_COMPILATION
};

BEGIN_CLASS_RTTI( CMaterialOutputSocket )
	PARENT_CLASS( CGraphSocket );
	PROPERTY( m_swizzle );
END_CLASS_RTTI();

#endif // !( defined( NO_RUNTIME_MATERIAL_COMPILATION ) && defined( NO_EDITOR_GRAPH_SUPPORT ) )
 // NO_EDITOR_GRAPH_SUPPORT

#ifndef NO_EDITOR_GRAPH_SUPPORT

/// Material output socket spawn info
class MaterialOutputSocketSpawnInfo : public GraphSocketSpawnInfo
{
public:
	String	m_swizzle;

public:
	MaterialOutputSocketSpawnInfo();
	MaterialOutputSocketSpawnInfo( const CName& name, const Color& color = Color::WHITE );
	MaterialOutputSocketSpawnInfo( const String& swizzle, const CName& name, const Color& color );
};

#endif // NO_EDITOR_GRAPH_SUPPORT
