#include "build.h"
#include "questCondition.h"
#include "quest.h"
#include "storySceneChoice.h"
#include "../core/gameSave.h"
#include "../engine/gameSaveManager.h"

IMPLEMENT_ENGINE_CLASS( IQuestCondition )

IQuestCondition::IQuestCondition()
	: m_active( true )	// obsolete - maintain for the game saves backwards compatibility
{
}

void IQuestCondition::Activate()
{
	OnActivate();
}

void IQuestCondition::Deactivate()
{
	OnDeactivate();
}

Bool IQuestCondition::IsFulfilled()
{
	return OnIsFulfilled();
}

void IQuestCondition::SaveGame( IGameSaver* saver )
{
	// Get properties to save
	const Uint32 maxNumProperties( 32 );
	Uint32 numPropertiesToSave( 0 );
	CProperty* propertiesToSave[ maxNumProperties ];

	CGameSaverBlock block( saver, CNAME( p ) );

	numPropertiesToSave = saver->GetSavableProperties( GetClass(), propertiesToSave, maxNumProperties );

	saver->WriteValue( CNAME( nP ), numPropertiesToSave );
	for ( Uint32 i = 0; i < numPropertiesToSave; ++i )
	{
		saver->WriteProperty( this, propertiesToSave[ i ] );
	}
}

void IQuestCondition::LoadGame( IGameLoader* loader )
{
	if ( loader->GetSaveVersion() < SAVE_VERSION_QUEST_CONDITION_PROPERTIES )
	{
		loader->ReadValue( CNAME( active ), m_active );
	}
	else
	{
		CGameSaverBlock block( loader, CNAME( p ) );

		const Uint32 numProperties = loader->ReadValue< Uint32 >( CNAME( nP ) );

		CClass* theClass = GetClass();
		for ( Uint32 i = 0; i < numProperties; ++i )
		{
			loader->ReadProperty( this, theClass, this );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestScriptedCondition )

CQuestScriptedCondition::CQuestScriptedCondition()
	: m_evaluateFunction( nullptr )
{
}

void CQuestScriptedCondition::OnActivate()
{
	CacheScriptedFunctions();
	CallFunction( this, CNAME( Activate ) );
}

void CQuestScriptedCondition::OnDeactivate()
{
	CallFunction( this, CNAME( Deactivate ) );
	m_evaluateFunction = nullptr;
}

Bool CQuestScriptedCondition::OnIsFulfilled()
{
	Bool result = false;
	if ( m_evaluateFunction != nullptr )
	{
		m_evaluateFunction->Call( this, nullptr, &result );
	}
	return result;
}

void CQuestScriptedCondition::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	CacheScriptedFunctions();
}

void CQuestScriptedCondition::CacheScriptedFunctions()
{
	m_evaluateFunction = nullptr;
	IScriptable* context = this;
	m_evaluateFunction = this->FindFunction( context, CNAME( Evaluate ), false );
	RED_ASSERT( m_evaluateFunction != nullptr );
	RED_ASSERT( m_evaluateFunction->GetNumParameters() == 0 );
	RED_ASSERT( m_evaluateFunction->GetReturnValue() != nullptr );
	RED_ASSERT( m_evaluateFunction->GetReturnValue()->GetType()->GetName() == TTypeName< Bool >::GetTypeName() );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_RTTI_ENUM( ELogicOperation )
IMPLEMENT_ENGINE_CLASS( CQuestLogicOperationCondition )

	CQuestLogicOperationCondition::CQuestLogicOperationCondition()
	: m_logicOperation( LO_And )
{
}

void CQuestLogicOperationCondition::OnActivate()
{
	TDynArray< IQuestCondition* >::iterator itEnd = m_conditions.End();
	for ( TDynArray< IQuestCondition* >::iterator it = m_conditions.Begin(); it != itEnd; ++it )
	{
		if ( IQuestCondition* cond = *it )
		{
			cond->Activate();
		}
	}
}

void CQuestLogicOperationCondition::OnDeactivate()
{
	TDynArray< IQuestCondition* >::iterator itEnd = m_conditions.End();
	for ( TDynArray< IQuestCondition* >::iterator it = m_conditions.Begin(); it != itEnd; ++it )
	{
		if ( IQuestCondition* cond = *it )
		{			
			cond->Deactivate();
		}		
	}
}

Bool CQuestLogicOperationCondition::OnIsFulfilled()
{
	Bool result = true;
	for ( TDynArray< IQuestCondition* >::iterator conditionIter = m_conditions.Begin();
		conditionIter != m_conditions.End(); ++conditionIter )
	{
		if ( !( *conditionIter ) )
		{
			// when a condition is nulled, always return false
			String info;
			CObject* object = GetParent();
			while ( object )
			{
				if ( CQuest* quest = Cast< CQuest >( object ) )
				{
					info = quest->GetFriendlyName();
					break;
				}
				else if ( CStorySceneChoice* choice = Cast< CStorySceneChoice >( object ) )
				{
					info = choice->GetFriendlyName();
					break;
				}
				object = object->GetParent();
			}
			RED_LOG( RED_LOG_CHANNEL( Warning ), TXT("CQuestLogicOperationCondition - one of conditions is null in [%s]"), info.AsChar() );
			return false;
		}
		if ( conditionIter == m_conditions.Begin() )
		{
			result = ( *conditionIter )->IsFulfilled();
		}
		else
		{
			switch ( m_logicOperation )
			{
			case LO_And:
			case LO_Nand:
				result &= ( *conditionIter )->IsFulfilled();
				break;
			case LO_Or:
			case LO_Nor:
				result |= ( *conditionIter )->IsFulfilled();
				break;
			case LO_Xor:
			case LO_Nxor:
				result ^= ( *conditionIter )->IsFulfilled();
				break;
			}
		}
	}

	if ( m_logicOperation == LO_Nand ||
		 m_logicOperation == LO_Nor ||
		 m_logicOperation == LO_Nxor )
	{
		result = !result;
	}
	return result;
}


