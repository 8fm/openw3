#include "build.h"
#include "questGraphSocket.h"
#include "questGraph.h"
#include "questScopeBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnection.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphBlock )
IMPLEMENT_ENGINE_CLASS( SBlockDesc )
IMPLEMENT_ENGINE_CLASS( SCachedConnections )
RED_DEFINE_STATIC_NAME( PatchOut )
RED_DEFINE_STATIC_NAME( TerminationIn )

CQuestGraphBlock::CQuestGraphBlock()
	: m_forceKeepLoadingScreen( false )
{
}

void CQuestGraphBlock::UpdateGUID()
{
	if ( MarkModified() )
	{
		m_guid = CGUID::Create();
	}
}

void CQuestGraphBlock::GetAllOutputNames( TDynArray< CName >& outNames ) const
{
	if ( m_cachedConnections.Empty() )
	{
		// there are no cached connections
#ifndef NO_EDITOR_GRAPH_SUPPORT
		for ( Uint32 i=0; i<m_sockets.Size(); i++ )
		{
			CQuestGraphSocket *socket = Cast< CQuestGraphSocket >( m_sockets[ i ] );
			if ( socket && socket->GetDirection() == LSD_Output )
			{
				outNames.PushBack( socket->GetName() );
			}
		}
#endif
	}
	else
	{
		// there are cached connections
		for ( TDynArray< SCachedConnections >::const_iterator it = m_cachedConnections.Begin(); it != m_cachedConnections.End(); ++it )
		{
			outNames.PushBack( it->m_socketId );
		}
	}
}

String CQuestGraphBlock::GetDebugName() const
{
	// resource path
	CResource* rootResource = Cast< CResource >( GetRoot() );
	String debugName = rootResource ? rootResource->GetDepotPath() : String::EMPTY;

	// sub graph name
	if ( GetParent() && GetParent()->IsA< CQuestGraph >() )
	{
		const CQuestGraph* graph = static_cast< const CQuestGraph* >( GetParent() );
		if ( graph->GetParent() && graph->GetParent()->IsA< CQuestScopeBlock >() )
		{
			const CQuestScopeBlock* scope = static_cast< const CQuestScopeBlock* >( GetParent()->GetParent() );

			debugName += TXT("::");
			debugName += scope->GetCaption();
		}
	}

	// block caption
	debugName += TXT("::");
	debugName += GetCaption();
	return debugName;
}

void CQuestGraphBlock::CollectContent( IQuestContentCollector& collector ) const
{
	// nothing
	RED_UNUSED( collector );
}

void CQuestGraphBlock::GetGUIDs( TDynArray< CGUID >& outGUIDs ) const
{
	outGUIDs.PushBackUnique( m_guid );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestGraphBlock::CleanupSourceData()
{

}

void CQuestGraphBlock::CacheConnections()
{
	m_cachedConnections.ClearFast();	
	const TDynArray< CGraphSocket* >& sockets = GetSockets();
	
	m_cachedConnections.Reserve( sockets.Size() );

	for ( TDynArray< CGraphSocket* >::const_iterator it = sockets.Begin(); it != sockets.End(); ++it )
	{
		CGraphSocket* socket = *it;
		if ( socket->GetDirection() != LSD_Output )
		{
			continue;
		}

		m_cachedConnections.PushBack( SCachedConnections( socket->GetName() ) );
		TDynArray< SBlockDesc >& blocks = m_cachedConnections.Back().m_blocks;

		const TDynArray< CGraphConnection* >& connections = (*it)->GetConnections();
		blocks.Reserve( connections.Size() );
		for ( TDynArray< CGraphConnection* >::const_iterator connIt = connections.Begin(); connIt != connections.End(); ++connIt )
		{
			if ( !(*connIt)->IsActive() )
			{
				continue;
			}

			CGraphSocket* destSocket = (*connIt)->GetDestination();
			CQuestGraphBlock* block = Cast< CQuestGraphBlock >( destSocket->GetBlock() );
			if ( block )
			{
				blocks.PushBack( SBlockDesc( block, destSocket->GetName() ) );
			}
		}
	}
}

void CQuestGraphBlock::CleanupConnections()
{
	BreakAllLinks();
	RemoveAllSockets();
}

void CQuestGraphBlock::AddTerminationInput()
{
	if ( !m_hasTerminationInput )
	{
		m_hasTerminationInput = true;
		OnRebuildSockets();
	}
}

void CQuestGraphBlock::RemoveTerminationInput()
{
	if ( m_hasTerminationInput )
	{
		m_hasTerminationInput = false;
		OnRebuildSockets();
	}
}

void CQuestGraphBlock::GetContextMenuSpecialOptions( SpecialOptionsList& outOptions )
{
	CObject* context = this;
	CallFunctionRef( context, CNAME( GetContextMenuSpecialOptions ), outOptions );
}
void CQuestGraphBlock::RunSpecialOption( Int32 option )
{
	CObject* context = this;
	CallFunction( context, CNAME( RunSpecialOption ), option );
}

void CQuestGraphBlock::AddPatchOutput()
{
	if ( m_hasPatchOutput == false )
	{
		m_hasPatchOutput = true;
		OnRebuildSockets();
	}
}

void CQuestGraphBlock::RemovePatchOutput()
{
	if ( m_hasPatchOutput == true )
	{
		m_hasPatchOutput = false;
		OnRebuildSockets();
	}
}

void CQuestGraphBlock::OnRebuildSockets()
{
	if ( m_hasPatchOutput )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( PatchOut ), LSD_Output, LSP_Right ) );
	}

	if( m_hasTerminationInput )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( TerminationIn ), LSD_Input, LSP_Left ) );
	}
}

void CQuestGraphBlock::OnSpawned( const GraphBlockSpawnInfo& info )
{
	TBaseClass::OnSpawned( info );
	UpdateGUID();
}

void CQuestGraphBlock::OnPasted( Bool wasCopied )
{
	TBaseClass::OnPasted( wasCopied );
	UpdateGUID();
}

#endif
void CQuestGraphBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	compiler << i_activeOutput;
	compiler << i_activeInputs;
	compiler << i_wasOutputActivated;
	compiler << i_canDeactivate;
	compiler << i_canActivate;
	compiler << i_isEnabled;
	compiler << i_state;
	compiler << i_errorMsg;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	compiler << i_isVisited;
#endif //NO_EDITOR_GRAPH_SUPPORT
}

void CQuestGraphBlock::OnInitInstance( InstanceBuffer& data ) const
{
	data[ i_activeInputs ].Clear();
	data[ i_activeOutput ] = CName::NONE;
	data[ i_wasOutputActivated ] = false;
	data[ i_canDeactivate ] = false;
	data[ i_canActivate ] = true;
	data[ i_isEnabled ] = true;
	data[ i_state ] = ST_INACTIVE;
	data[ i_errorMsg ] = TXT( "" );
#ifndef NO_EDITOR_GRAPH_SUPPORT
	data[ i_isVisited ] = false;
#endif //NO_EDITOR_GRAPH_SUPPORT
}

void CQuestGraphBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	if ( m_hasPatchOutput )
	{
		// activate a patch output if one's defined
		ActivateOutputWithoutExiting( data, CNAME( PatchOut ), true );
	}

	if( m_hasTerminationInput && inputName == CNAME( TerminationIn ) )
	{
		// terminate block if running
		Deactivate( data );
		ActivateOutput( data, CNAME( Out ), true);
	}
}

const TDynArray< CName >& CQuestGraphBlock::GetActiveInputs( InstanceBuffer& instanceData ) const
{
	return instanceData[ i_activeInputs ];
}

Bool CQuestGraphBlock::Activate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	if ( data[ i_canActivate ] )
	{
		data[ i_isEnabled ] = true;
		data[ i_activeInputs ].PushBackUnique( inputName );
		OnActivate( data, inputName, parentThread );
		data[ i_state ] = ST_ACTIVATING;
#ifndef NO_EDITOR_GRAPH_SUPPORT
		data[ i_isVisited ] = true;
#endif //NO_EDITOR_GRAPH_SUPPORT
		return true;
	}
	else
	{
		return false;
	}
}

void CQuestGraphBlock::Deactivate( InstanceBuffer& data ) const
{
	OnDeactivate( data );
	data[ i_wasOutputActivated ] = false;
	data[ i_canDeactivate ] = false;
	data[ i_activeInputs ].Clear();
	data[ i_activeOutput ] = CName::NONE;
	data[ i_state ] = ST_INACTIVE;
}

Bool CQuestGraphBlock::Execute( InstanceBuffer& data, TDynArray< SBlockDesc >& outBlocks ) const
{
	// if the block is disabled at this point, deactivate it without activating subsequent blocks
	if (  !data[ i_isEnabled ] )
	{
		return true;
	}

	// if the block's output was activated - activate subsequent blocks...
	if ( data[ i_wasOutputActivated ] )
	{
		GetConnectedBlocks( data[ i_activeOutput ], outBlocks );
		data[ i_wasOutputActivated ] = false;

		// ... and deactivate this block if needed
		if ( data[ i_canDeactivate ] )
		{
			// that's right - return only if we want the block deactivated
			return true;
		}
	}

	// ok - we got this far - process block's state
	switch ( data[ i_state ] )
	{
	case ST_ACTIVATING:
		{
			if ( OnProcessActivation( data ) )
			{
				data[ i_state ] = ST_ACTIVE;
				// fallthrough
			}
			else
			{
				break;
			}
		} 

	case ST_ACTIVE:
		{
			OnExecute( data );
			break;
		}
	}


	// recheck the block's state:
	// ----------------------------
	// if the block is disabled at this point, deactivate it without activating subsequent blocks
	if (  !data[ i_isEnabled ] )
	{
		return true;
	}

	// if the block's output was activated - activate subsequent blocks...
	if ( data[ i_wasOutputActivated ] )
	{
		GetConnectedBlocks( data[ i_activeOutput ], outBlocks );
		data[ i_wasOutputActivated ] = false;

		// ... and deactivate this block if needed
		return data[ i_canDeactivate ];
	}
	else
	{
		return false;
	}
}

Bool CQuestGraphBlock::ActivateOutput( InstanceBuffer& data, 
									  const CName& outSocket, 
									  Bool onlyIfOutputExists ) const
{
	bool canActivate = true;

	if ( onlyIfOutputExists )
	{
		canActivate = DoesSocketExist( outSocket );
	}

	if ( canActivate )
	{
		data[ i_activeOutput ] = outSocket;
		data[ i_wasOutputActivated ] = true;
		data[ i_canDeactivate ] = true;
	}

	return data[ i_wasOutputActivated ];
}

Bool CQuestGraphBlock::ActivateOutputWithoutExiting( InstanceBuffer& data, const CName& outSocket, Bool onlyIfOutputExists ) const
{
	if ( ActivateOutput( data, outSocket, onlyIfOutputExists ) )
	{
		data[ i_canDeactivate ] = false;
		return true;
	}
	else
	{
		return false;
	}
}

void CQuestGraphBlock::GetConnectedBlocks( const CName& socketId, TDynArray< SBlockDesc >& outBlocks ) const
{
	if ( m_cachedConnections.Empty() )
	{
		// there are no cached connections - search through the actual graph structure

		CGraphSocket* outSocket = FindSocket< CGraphSocket >( socketId );
		if ( outSocket == NULL )
		{
			return;
		}

#ifndef NO_EDITOR_GRAPH_SUPPORT

		if ( outSocket->GetDirection() != LSD_Output )
		{
			String caption;
			outSocket->GetCaption( caption );
			ERR_GAME( TXT( "Quest block %s is trying to exit using a socket %s which is not an output." ), GetCaption().AsChar(), caption.AsChar() );
			//return;
		}

#endif

		const TDynArray< CGraphConnection* >& connectedInputs = outSocket->GetConnections();

		Uint32 count = connectedInputs.Size();
		outBlocks.Reserve( outBlocks.Size() + count );
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( connectedInputs[ i ]->IsActive() )
			{
				CGraphSocket* destSocket = connectedInputs[ i ]->GetDestination();
				CQuestGraphBlock* block = SafeCast< CQuestGraphBlock >( destSocket->GetBlock() );
				outBlocks.PushBack( SBlockDesc( block, destSocket->GetName() ) );
			}
		}
	}
	else
	{
		// there are cached connections - fetch the blocks from there
		for ( TDynArray< SCachedConnections >::const_iterator it = m_cachedConnections.Begin(); it != m_cachedConnections.End(); ++it )
		{
			if ( it->m_socketId != socketId )
			{
				continue;
			}

			outBlocks.Reserve( outBlocks.Size() + it->m_blocks.Size() );

			for ( TDynArray< SBlockDesc >::const_iterator blockIt = it->m_blocks.Begin(); blockIt != it->m_blocks.End(); ++blockIt )
			{
				outBlocks.PushBack( *blockIt );
			}
			break;
		}
	}
}

Bool CQuestGraphBlock::DoesSocketExist( const CName& socketId ) const
{
	Bool doesExist = false;
	if ( m_cachedConnections.Empty() )
	{
		// there are no cached connections - search through the actual graph structure

		CQuestGraphSocket* socket = FindSocket< CQuestGraphSocket >( socketId );
		doesExist = ( socket != NULL ) ;

	#ifndef NO_EDITOR_GRAPH_SUPPORT
		if ( socket && !HasFlag( OF_WasCooked ) && socket->GetDirection() != LSD_Output )
		{
			ERR_GAME( TXT( "Quest block %s is trying to exit using a socket %s which is not an output." ), GetCaption().AsChar() , socketId.AsString().AsChar() );
			doesExist = false;
		}
	#endif
	}
	else
	{
		// there are cached connections - fetch the blocks from there
		for ( TDynArray< SCachedConnections >::const_iterator it = m_cachedConnections.Begin(); it != m_cachedConnections.End(); ++it )
		{
			if ( it->m_socketId == socketId )
			{
				doesExist = true;
				break;
			}
		}
	}

	return doesExist;
}

void CQuestGraphBlock::ThrowError( InstanceBuffer& data, const Char* errorMsg, ... ) const
{
	// disable the block
	Disable( data );

	// create an error message
	va_list argptr;
	va_start( argptr, errorMsg );
	Char buf[ 4096 ];
	Red::System::VSNPrintF( buf, ARRAY_COUNT(buf), errorMsg, argptr ); 
	data[ i_errorMsg ] = buf;
}

void CQuestGraphBlock::ThrowErrorNonBlocking( InstanceBuffer& data, const CName& outputName, const Char* errorMsg, ... ) const
{
	ActivateOutput( data, outputName, false );

	// create an error message
	va_list argptr;
	va_start( argptr, errorMsg );
	Char buf[ 4096 ];
	Red::System::VSNPrintF( buf, ARRAY_COUNT(buf), errorMsg, argptr ); 
	data[ i_errorMsg ] = buf;
}

void CQuestGraphBlock::Disable( InstanceBuffer& data, Bool canActivate ) const
{
	data[ i_isEnabled ] = false;
	data[ i_canActivate ] = canActivate;
}

void CQuestGraphBlock::SetCanActivate( InstanceBuffer& data, Bool canActivate ) const
{
	data[ i_canActivate ] = canActivate;
}

void CQuestGraphBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{

}

void CQuestGraphBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{

}

CQuestGraphBlock::EState CQuestGraphBlock::GetActivationState( InstanceBuffer& data ) const
{
	return (EState)data[ i_state ];
}

Bool CQuestGraphBlock::IsBlockEnabled( InstanceBuffer& data ) const
{
	return data[ i_isEnabled ];
}

Bool CQuestGraphBlock::WasOutputActivated( InstanceBuffer& data ) const
{
	return data[ i_wasOutputActivated ];
}

CName CQuestGraphBlock::GetActivatedOutputName( InstanceBuffer& data ) const
{
	return data[ i_activeOutput ];
}

const String& CQuestGraphBlock::GetErrorMsg( InstanceBuffer& data ) const
{
	return data[ i_errorMsg ];
}

#ifndef NO_EDITOR_GRAPH_SUPPORT
Bool CQuestGraphBlock::IsBlockVisited( InstanceBuffer& data ) const
{
	return data[ i_isVisited ];
}
#endif //NO_EDITOR_GRAPH_SUPPORT
