#include "build.h"
#include "counterUtilityGraphBlock.h"

#include "journalBlock.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CCounterUtilityGraphBlock );

CCounterUtilityGraphBlock::CCounterUtilityGraphBlock()
:	m_target( 0 )
{
	m_name = TXT( "Counter" );
}

CCounterUtilityGraphBlock::~CCounterUtilityGraphBlock()
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CCounterUtilityGraphBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create inputs
	GraphSocketSpawnInfo info( ClassID< CQuestGraphSocket >() );

	info.m_name = CNAME( Increment );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Decrement );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Reset );
	info.m_direction = LSD_Input;
	info.m_placement = LSP_Left;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	// Create output
	info.m_name = CNAME( Changed );
	info.m_direction = LSD_Output;
	info.m_placement = LSP_Right;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );

	info.m_name = CNAME( Finished );
	info.m_direction = LSD_Output;
	info.m_placement = LSP_Right;
	info.m_isMultiLink = (info.m_direction == LSD_Input);
	CreateSocket( info );
}

#endif

void CCounterUtilityGraphBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if( inputName == CNAME( Increment ) )
	{
		++data[ i_count ];
	}
	if( inputName == CNAME( Decrement ) )
	{
		--data[ i_count ];
	}
	if( inputName == CNAME( Reset ) )
	{
		data[ i_count ] = 0;
	}

	if( data[ i_count ] == m_target )
	{
		ActivateOutput( data, CNAME( Finished ) );
	}
	else
	{
		ActivateOutputWithoutExiting( data, CNAME( Changed ) );
	}
}

void CCounterUtilityGraphBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );

	saver->WriteValue< Int32 >( CNAME( Count ), data[ i_count ] );
}

void CCounterUtilityGraphBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	loader->ReadValue< Int32 >( CNAME( Count ), data[ i_count ] );
}

void CCounterUtilityGraphBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_count;
}

void CCounterUtilityGraphBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_count ] = 0;
}
