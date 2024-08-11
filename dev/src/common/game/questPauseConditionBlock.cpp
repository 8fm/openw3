#include "build.h"
#include "questGraphSocket.h"
#include "questPauseConditionBlock.h"
#include "questCondition.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestPauseConditionBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestPauseConditionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );

	Uint32 count = m_conditions.Size();
	if ( count == 0 )
	{
		// make sure there's always an output - even if no conditions are defined
		CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
	}
	else
	{
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( m_conditions[ i ] == NULL )
			{
				continue;
			}
			CreateSocket( CQuestGraphSocketSpawnInfo( m_conditions[ i ]->GetName(), LSD_Output, LSP_Right ) );
		}
	}
}

void CQuestPauseConditionBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( conditions ) )
	{
		OnRebuildSockets();
	}
}

#endif

void CQuestPauseConditionBlock::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( TDynArray< IQuestCondition* >::iterator it = m_gcList.Begin(); it != m_gcList.End(); ++it )
		{
			file << *it;
		}
	}
}

void CQuestPauseConditionBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_activeConditions;
}

void CQuestPauseConditionBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	Uint32 count = m_conditions.Size();
	if ( count == 0 )
	{
		// if no condition is defined, exit immediately
		ActivateOutput( data, CNAME( Out ) );
	}
	else
	{
		ResetRuntimeConditions( data );

		// Add new conditions
		for ( Uint32 i = 0; i < count; ++i )
		{
			if ( m_conditions[ i ] )
			{
				IQuestCondition* condition = Cast< IQuestCondition >( m_conditions[ i ]->Clone( const_cast< CQuestPauseConditionBlock* >( this ) ) );
				condition->Activate();
				data[ i_activeConditions ].PushBack( condition );
				m_gcList.PushBack( condition );
			}
		}
	}
}

void CQuestPauseConditionBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	Bool conditionFulfilled = false;

	TDynArray< IQuestCondition* >& conditions = data[ i_activeConditions ];
	Uint32 count = conditions.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		if ( conditions[ i ]->IsFulfilled() )
		{
			ActivateOutput( data, conditions[ i ]->GetName() );
			conditionFulfilled = true;
			break;
		}
	}

	// remove completed conditions
	if ( conditionFulfilled )
	{
		ResetRuntimeConditions( data );
	}
}

void CQuestPauseConditionBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	ResetRuntimeConditions( data );
}

void CQuestPauseConditionBlock::ResetRuntimeConditions( InstanceBuffer& data ) const
{
	// Reset
	TDynArray< IQuestCondition* >& conditions = data[ i_activeConditions ];
	Uint32 count = conditions.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		m_gcList.Remove( conditions[i] );

		conditions[i]->Deactivate();
		conditions[i]->Discard();
	}
	conditions.Clear();
}

void CQuestPauseConditionBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver ) const
{
	TBaseClass::SaveGame( data, saver );

	// Conditions
	{
		CGameSaverBlock block( saver, CNAME(conditions) );

		// Save condition count
		TDynArray< IQuestCondition* >& conditions = data[ i_activeConditions ];
		Uint32 conditionCount = conditions.Size();
		saver->WriteValue( CNAME(numConditions), conditionCount );

		// Save conditions
		for ( Uint32 i=0; i<conditionCount; ++i )
		{
			conditions[ i ]->SaveGame( saver );
		}
	}
}

void CQuestPauseConditionBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader ) const
{
	TBaseClass::LoadGame( data, loader );

	// Conditions
	{
		CGameSaverBlock block( loader, CNAME(conditions) );

		// Load condition count
		Uint32 conditionCount = 0;
		loader->ReadValue( CNAME(numConditions), conditionCount );

		// Invalid condition count
		// This can happen due to invalid data or an old save file, so it shouldn't be an assert
		// ASSERT( conditionCount == data[ i_activeConditions ].Size() );
		// ASSERT( conditionCount == m_conditions.Size() );

		conditionCount = Min( conditionCount, data[ i_activeConditions ].Size() );

		// Load condition states
		TDynArray< IQuestCondition* >& conditions = data[ i_activeConditions ];
		for ( Uint32 i = 0; i < conditionCount; ++i )
		{
			conditions[ i ]->LoadGame( loader );
		}
	}
}
