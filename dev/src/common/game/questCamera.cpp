
#include "build.h"
#include "questGraphSocket.h"
#include "questCamera.h"
#include "../engine/tagManager.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestCameraBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestCameraBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

CStaticCamera* CQuestCameraBlock::FindStaticCamera( const CName& tag ) const
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetTagManager() )
	{
		return Cast< CStaticCamera >( world->GetTagManager()->GetTaggedEntity( tag ) );
	}
	return NULL;
}
