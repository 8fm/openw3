#include "build.h"
#include "mbInput.h"
#include "graphConnectionRebuilder.h"
#include "materialOutputSocket.h"

IMPLEMENT_ENGINE_CLASS( CMaterialBlockInput );

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CMaterialBlockInput::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialOutputSocketSpawnInfo( CNAME( Out ) ) );
}

#endif