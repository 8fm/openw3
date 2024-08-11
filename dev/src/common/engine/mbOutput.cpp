#include "build.h"
#include "mbOutput.h"
#include "graphConnectionRebuilder.h"
#include "materialInputSocket.h"

IMPLEMENT_ENGINE_CLASS( CMaterialBlockOutput );

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CMaterialBlockOutput::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	CreateSocket( MaterialInputSocketSpawnInfo( CNAME( In ) ) );
}
#endif
