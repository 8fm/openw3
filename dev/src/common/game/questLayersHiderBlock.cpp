#include "build.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "questLayersHiderBlock.h"
#include "questGraphSocket.h"
#include "questThread.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/worldStreaming.h"

IMPLEMENT_ENGINE_CLASS( CQuestLayersHiderBlock )

CQuestLayersHiderBlock::CQuestLayersHiderBlock()
	: m_syncOperation( false )
	, m_purgeSavedData( false )
{
	m_name = TXT("Hide/Show layers");
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestLayersHiderBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}

#endif

void CQuestLayersHiderBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_layerStreamingFence;
}

void CQuestLayersHiderBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_layerStreamingFence ] = 0;
}

void CQuestLayersHiderBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	// Pass to base class
	TBaseClass::OnActivate( data, inputName, parentThread );
	
	// Create loading fence - so we will know EXACTLY when the given layers finished loading/unloading
	if ( IsCurrentWorld() && ( !m_layersToHide.Empty() || !m_layersToShow.Empty() ) )
	{
		const String fenceName = GetDebugName();
		CWorldLayerStreamingFence* fence = new CWorldLayerStreamingFence( fenceName );

		// Schedule the layer loading
		if ( GGame->ScheduleLayersVisibilityChange( m_world, m_layersToHide, m_layersToShow, m_purgeSavedData, fence ) == false )
		{
			fence->Release();
			ThrowErrorNonBlocking( data, CNAME( Out ), TXT( "Failed to schedule a layers visibility change task" ) );
			return;
		}

		// store the fence
		data[ i_layerStreamingFence ] = (TGenericPtr)fence;
	}
}

void CQuestLayersHiderBlock::OnDeactivate( InstanceBuffer& data ) const
{
	// Pass to base class
	TBaseClass::OnDeactivate( data );

	// Release loading fence
	if ( data[ i_layerStreamingFence ] )
	{
		CWorldLayerStreamingFence* fence = (CWorldLayerStreamingFence*)( data[ i_layerStreamingFence ] );
		fence->Release();

		data[ i_layerStreamingFence ] = nullptr;
	}
}

void CQuestLayersHiderBlock::OnExecute( InstanceBuffer& data ) const
{
	// Pass to base class
	TBaseClass::OnExecute( data );

	if ( IsCurrentWorld() )
	{
		// Check the fence
		if ( data[ i_layerStreamingFence ] )
		{
			CWorldLayerStreamingFence* fence = (CWorldLayerStreamingFence*)( data[ i_layerStreamingFence ] );
			if ( !fence->CheckIfCompleted() )
				return;

			// loading completed
			data[ i_layerStreamingFence ] = nullptr;
			fence->Release();
		}
	}
	else
	{
		// this is not the current world, just change the flags in universe sorage and bye bye
		GGame->GetUniverseStorage()->OnLayersVisibilityChanged( m_world, m_layersToHide, m_layersToShow );
	}

	// propagate quest signal
	ActivateOutput( data, CNAME( Out ) );
}
