#include "build.h"
#include "questGraphSocket.h"
#include "questPerformGCBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"
#ifndef NO_TEST_FRAMEWORK
#include "../engine/testFramework.h"
#endif

IMPLEMENT_ENGINE_CLASS( CQuestPerformGCBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPerformGCBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestPerformGCBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_GCRequestId;
}

void CQuestPerformGCBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

#ifndef NO_TEST_FRAMEWORK
	if( STestFramework::GetInstance().IsActive() ) 
	{
		data[ i_GCRequestId ] = STestFramework::GetInstance().GetGCScheduler().RequestGC();
	} 
	else
#endif
	{
		ActivateOutput( data, CNAME( Out ) );
	}
}

void CQuestPerformGCBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data  );

#ifndef NO_TEST_FRAMEWORK
	if( STestFramework::GetInstance().IsActive() && 
		STestFramework::GetInstance().GetGCScheduler().WasGCPerformed( data[ i_GCRequestId ] ) ) 
	{
		ActivateOutput( data, CNAME( Out ) );
	}
#endif
}