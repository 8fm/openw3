/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "idGraph.h"
#include "idBasicBlocks.h"
#include "idGraphBlockBranch.h"
#include "idGraphBlockChoice.h"
#include "idTopic.h"
#include "idThreadActivator.h"
#include "idThread.h"
#include "idEventSender.h"
#include "idEventSenderDataStructs.h"
#include "idInstance.h"
#include "idHud.h"

#include "../../common/core/instanceDataLayoutCompiler.h"
#include "../../common/engine/graphConnection.h"
#include "../../common/engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CIDGraphBlock )
IMPLEMENT_ENGINE_CLASS( CIDGraphBlockInput )
IMPLEMENT_ENGINE_CLASS( CIDGraphBlockOutput )
IMPLEMENT_ENGINE_CLASS( CIDGraphBlockFork )

//--------------------------------------------------------------------------------------------------
// Base block
//--------------------------------------------------------------------------------------------------
String CIDGraphBlock::GetBlockName() const
{
	return GetName().AsString();
}

void CIDGraphBlock::OnInitInstance( InstanceBuffer& data ) const
{
	// this is the base class... remember to call TBaseClass::OnInitInstance() in derived classes overrides

	data[ i_active ] = 0;
	data[ i_activeInputSocketIndex ] = 0;
}

void CIDGraphBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	// this is the base class... remember to call TBaseClass::OnBuildDataLayout() in derived classes overrides

	compiler << i_active;
	compiler << i_activeInputSocketIndex;
}

const CIDGraphBlock* CIDGraphBlock::ActivateInput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* input, Bool evaluate /*= true*/ ) const
{	
	R6_ASSERT( m_sockets.Size() < 255 );	
	RED_LOG( Dialog, TXT("Activating block %s of type %s"), GetName().AsString().AsChar(), GetClass()->GetName().AsString().AsChar() );

	InstanceBuffer& data = topicInstance->GetInstanceData();

	// Get the socket we come from
	if ( input )
	{
		// Linear search for socket will be the fastest option here
		Uint8 idx = 255;
		for ( Uint32 i = 0; i < m_sockets.Size(); ++i )
		{
			if ( input == m_sockets[ i ] )
			{
				idx = Uint8( i );
				break;
			}
		}

		R6_ASSERT( idx < 255 );
		data[ i_activeInputSocketIndex ] = idx;
	}

	// Activate the block
	SetActive( topicInstance, true ); 

	if ( IsRegular() )
	{
		// Inform the thread
		topicInstance->GetCurrentThread()->OnRegularBlockEncountered( this );
	}

	// Call the events
	if( m_events )
	{
		m_events->ActivateAll( topicInstance );
	}

	// Evaluate the block
	if ( evaluate )
	{
		return Evaluate( topicInstance, timeDelta );
	}
	else
	{
		return this;
	}
}

const CIDGraphBlock* CIDGraphBlock::ActivateOutput( CIDTopicInstance* topicInstance, Float& timeDelta, const CIDGraphSocket* output, Bool evaluate /*= true*/ ) const
{
	// only active blocks can activate outputs
	if ( false == IsActivated( topicInstance ) )
	{
		return this;
	}

	SetActive( topicInstance, false );
	R6_ASSERT( output && output->GetDirection() == LSD_Output );

	const CIDGraphBlock* returnBlock = NULL;
	const TDynArray< CGraphConnection* >& connections = output->GetConnections();
	for ( Uint32 i = 0; i < connections.Size(); ++i )
	{
		const CGraphSocket* socket = connections[ i ]->GetDestination();
		R6_ASSERT( socket && socket->GetDirection() == LSD_Input );

		const CIDGraphBlock* block = Cast< const CIDGraphBlock > ( socket->GetBlock() );
		R6_ASSERT( block );

		const CIDGraphBlockBranch* branch = Cast< CIDGraphBlockBranch > ( block );
		if ( branch )
		{
			topicInstance->GetCurrentThread()->OnBranchBlockEncountered( branch );
		}
		else
		{
			const CIDGraphBlockChoice* choice = Cast< CIDGraphBlockChoice > ( block );
			if ( choice )
			{
				CIDThreadInstance*	thread	= topicInstance->GetCurrentThread();
				thread->OnChoiceBlockEncountered( choice );
			}
			else
			{
				const CIDGraphBlock* newBlock = block->ActivateInput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( socket ), evaluate ); 
				if ( newBlock )
				{
					R6_ASSERT( returnBlock == NULL && newBlock->IsRegular() );
					returnBlock = newBlock;	
				}
			}
		}
	}

	if ( connections.Empty() )
	{
		if( topicInstance )
		{
			CIDThreadInstance*	thread	= topicInstance->GetCurrentThread();
			if( thread )
			{
				thread->OnDeadEndEncountered();
			}
		}
	}

	return returnBlock;
}

const CIDGraphBlock* CIDGraphBlock::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	return this;
}

const CIDGraphSocket* CIDGraphBlock::GetCurrentInput( InstanceBuffer& data ) const
{
	Uint8 index = data[ i_activeInputSocketIndex ];
	R6_ASSERT( index < 255 && m_sockets.Size() > size_t( index ) );
	return Cast< const CIDGraphSocket > ( m_sockets[ index ] );
}

const CIDGraphSocket* CIDGraphBlock::FindOutputSocketByIndex( Int32 outputIndex ) const
{
	for ( Int32 i = 0, k = 0; i < m_sockets.SizeInt(); ++i )
	{
		if ( m_sockets[ i ]->GetDirection() == LSD_Output )
		{
			if ( outputIndex == k )
			{
				return Cast< const CIDGraphSocket > ( m_sockets[ i ] );
			}
			++k;
		}
	}
	return NULL;
}

CIDGraphBlock::CIDGraphBlock()
	: m_isCheckpoint	( true )
{

}

Int32 CIDGraphBlock::FindIndexOfOutputSocket( const CIDGraphSocket* socket ) const
{
	for ( Int32 i = 0, k = 0; i < m_sockets.SizeInt(); ++i )
	{
		if ( m_sockets[ i ]->GetDirection() == LSD_Output )
		{
			if ( m_sockets[ i ] == socket )
			{
				return k;
			}
			++k;
		}
	}
	return -1;
}

void CIDGraphBlock::SetActive( CIDTopicInstance* topicInstance, Bool activate ) const
{
	InstanceBuffer& data = topicInstance->GetInstanceData(); 
	data[ i_active ] = data[ i_active ] + ( activate ? 1 : -1 );
}

Bool CIDGraphBlock::IsActivated( const CIDTopicInstance* topicInstance ) const
{
	const InstanceBuffer& data = topicInstance->GetInstanceData();
	return data[ i_active ] > 0;
}

String CIDGraphBlock::GetFriendlyName() const
{
	return GetClass()->GetName().AsString() + TXT(" \"") + GetName().AsString() + TXT("\" inside ") + GetParent()->GetParent()->GetFriendlyName();
}

/* static */ String CIDGraphBlock::TrimCaptionLine( const String& captionLine )
{
	const Uint32 maxChars( 35 );
	const Uint32 len = captionLine.GetLength();

	if ( len > maxChars )
	{
		return captionLine.LeftString( maxChars - 3 ) + TXT("...");
	}
	return captionLine;
}

#ifndef NO_EDITOR
void CIDGraphBlock::OnPropertyPostChange( IProperty* prop )
{
	R6_ASSERT( GIsEditor );
	CIDTopic* topic = Cast< CIDTopic > ( GetParent() );
	R6_ASSERT( topic );

	if ( prop->GetName() == CNAME( name ) )
	{
		topic->EnsureBlockNameUniqueness( this );
	}

	TBaseClass::OnPropertyPostChange( prop );
}

void CIDGraphBlock::OnEditorPostLoad()
{
	R6_ASSERT( GIsEditor && !GIsEditorGame );
	if ( m_guid.IsZero() )
	{
		m_guid = CGUID::Create();
	}
	R6_ASSERT( false == m_guid.IsZero() );
}

void CIDGraphBlock::OnCreatedInEditor()
{
	R6_ASSERT( GIsEditor && !GIsEditorGame );
	m_guid = CGUID::Create();
	R6_ASSERT( false == m_guid.IsZero() );
	Cast< CIDTopic > ( GetParent() )->EnsureBlockNameUniqueness( this );
	OnRebuildSockets();
	TBaseClass::OnCreatedInEditor();
}

#endif // ifndef NO_EDITOR

//--------------------------------------------------------------------------------------------------
// Input block
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockInput::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	if( m_activator )
	{
		m_activator->OnInitInstance( data );
	}
}

void CIDGraphBlockInput::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	if( m_activator )
	{
		m_activator->OnBuildDataLayout( compiler );
	}
}

void CIDGraphBlockInput::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( Start ), LSD_Output ) );
}

Color CIDGraphBlockInput::GetTitleColor() const
{
	return Color::LIGHT_GREEN;
}

Bool CIDGraphBlockInput::GetWantsToPlay( InstanceBuffer& data, Uint32 SceneId ) const
{
	// No condition means that the thread can be activated
	if( !m_activator )
	{
		return true;
	}

	return m_activator->IsFulfilled(  data, SceneId );
}

const CIDGraphBlock* CIDGraphBlockInput::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	// Set default communicator
	CInteractiveDialogInstance* dialog	=	topicInstance->GetDialogInstance();
	dialog->SetComunicator( DDM_ActiveDialog );

	return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( m_sockets[ 0 ] ) );
}

EGraphBlockShape CIDGraphBlockInput::GetBlockShape() const
{
	return GBS_Arrow;
}

Color CIDGraphBlockInput::GetClientColor() const
{
	return Color::LIGHT_GREEN;
}

Uint32	CIDGraphBlockInput::GetPriority() const	
{	
	if( m_activator )
	{
		return m_activator->GetPriority();
	}

	return IDP_Irrelevant;	
}

//--------------------------------------------------------------------------------------------------
// Output block
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockOutput::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( End ), LSD_Input ) );
}

Color CIDGraphBlockOutput::GetTitleColor() const
{
	return Color::LIGHT_RED;
}

EGraphBlockShape CIDGraphBlockOutput::GetBlockShape() const
{
	return GBS_Arrow;
}

Color CIDGraphBlockOutput::GetClientColor() const
{
	return Color::LIGHT_RED;
}

const CIDGraphBlock* CIDGraphBlockOutput::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{
	topicInstance->GetCurrentThread()->OnFinished();

	return NULL;
}

//--------------------------------------------------------------------------------------------------
// Fork block
//--------------------------------------------------------------------------------------------------
void CIDGraphBlockFork::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( SIDGraphSocketSpawnInfo( CNAME( In ), LSD_Input ) );

	if ( m_numOutputs < 2 )
	{
		m_numOutputs = 2;
	}

	if ( m_numOutputs > 10 )
	{
		m_numOutputs = 10;
	}

	for ( Uint32 i = 0; i < m_numOutputs; ++i )
	{
		CreateSocket( SIDGraphSocketSpawnInfo( CName( String::Printf( TXT("%ld"), i + 1  ) ), LSD_Output ) );
	}
}

void CIDGraphBlockFork::OnPropertyPostChange( IProperty* prop )
{
	if ( prop->GetName() == CNAME( numOutputs ) )
	{
		OnRebuildSockets();
	}

	TBaseClass::OnPropertyPostChange( prop );
}

const CIDGraphBlock* CIDGraphBlockFork::Evaluate( CIDTopicInstance* topicInstance, Float& timeDelta ) const
{

	Int32 output( -1 );
	for ( Uint32 i = 0; i < m_numOutputs; ++i )
	{
		// find next output
		for ( ++output; output < m_sockets.SizeInt(); ++output )
		{
			if ( m_sockets[ Uint32( output ) ]->GetDirection() == LSD_Output )
			{
				break;
			}
		}
		R6_ASSERT( output < m_sockets.SizeInt() );

		if ( i < m_numOutputs - 1 )
		{
			topicInstance->Fork( Cast< const CIDGraphSocket > ( m_sockets[ output ] ) );	
		}
	}

	return ActivateOutput( topicInstance, timeDelta, Cast< const CIDGraphSocket > ( m_sockets[ output ] ) );
}

EGraphBlockShape CIDGraphBlockFork::GetBlockShape() const
{
	return GBS_TriangleLeft;
}

Color CIDGraphBlockFork::GetClientColor() const
{
	return Color( 180, 180, 0 );
}

