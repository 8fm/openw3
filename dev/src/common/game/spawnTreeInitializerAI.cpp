#include "build.h"
#include "spawnTreeInitializerAI.h"

#include "../core/dataError.h"

#include "../engine/utils.h"

#include "aiParameters.h"
#include "aiSpawnTreeParameters.h"
#include "aiParamInjectHandler.h"
#include "behTreeDynamicNodeEvent.h"
#include "encounter.h"


IMPLEMENT_ENGINE_CLASS( ISpawnTreeInitializerAI );

////////////////////////////////////////////////////////////////////
// ISpawnTreeInitializerAI
////////////////////////////////////////////////////////////////////

ISpawnTreeInitializerAI::~ISpawnTreeInitializerAI()
{

}

void ISpawnTreeInitializerAI::CacheStuff() const
{
	if ( !m_cached )
	{
		auto funIterate =
			[ this ] ( CProperty* p )
		{
			if ( p->GetName() == CNAME( ai ) )
			{
				IRTTIType* propType = p->GetType();
				switch( propType->GetType() )
				{
				case RT_Pointer:
				case RT_Handle:
					{
						const IRTTIType* pointedType = static_cast< const IRTTIPointerTypeBase* >( propType )->GetPointedType();
						if ( pointedType->GetType() == RT_Class && static_cast< const CClass* >( pointedType )->IsA( IAIParameters::GetStaticClass() ) )
						{
							ISpawnTreeInitializerAI* me = const_cast< ISpawnTreeInitializerAI* >( this );
							if ( propType->GetType() == RT_Pointer )
							{
								IAIParameters* ptr;
								p->Get( me, &ptr );
								m_lazyAI = ptr;
							}
							else
							{
								p->Get( me, &m_lazyAI );
							}
							break;
						}
					}

					break;
				default:
					break;
				}
			}
		};
		GetClass()->IterateProperties( funIterate );

		IAIParameters* tree = m_lazyAI.Get();
		if ( tree )
		{
			tree->GetParameter( m_dynamicTreeParameterName, m_lazyTree );
		}
		else
		{
			DATA_HALT( DES_Major, CResourceObtainer::GetResource( GetParent() ), TXT("Script"), TXT("%s class should have property ai of type IAIParameters* defined."), GetClass()->GetName().AsString().AsChar() );
		}

		m_cached = true;
	}
}

ISpawnTreeInitializerAI::EOutput ISpawnTreeInitializerAI::Activate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry, EActivationReason reason ) const
{
	CacheStuff();
	IAITree* tree = m_lazyTree.Get();
	if ( !tree )
	{
		return OUTPUT_SUCCESS;
	}

	SBehTreeDynamicNodeEventData e( tree );

	if( instance )
	{
		SBehTreeDynamicNodeEventData::Parameters params;
		params.PushBack( instance->GetEncounter()->GetEncounterParameters() );
		e.PushParameters( params );
	}

	actor->SignalGameplayEvent( GetDynamicNodeEventName(), &e, SBehTreeDynamicNodeEventData::GetStaticClass() );
	switch ( e.m_isHandled )
	{
	case SBehTreeDynamicNodeEventData::OUTPUT_NOT_HANDLED:
		break;
	case SBehTreeDynamicNodeEventData::OUTPUT_HANDLED:
		return OUTPUT_SUCCESS;
	case SBehTreeDynamicNodeEventData::OUTPUT_DELAYED:
		return OUTPUT_POSTPONED;
	default:
		ASSERT( false );
		ASSUME( false );
	}
	return OUTPUT_FAILED;
}

void ISpawnTreeInitializerAI::Deactivate( CActor* actor, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	TBaseClass::Deactivate( actor, instance, entry );
	SBehTreeDynamicNodeCancelEventData e( true );

	actor->SignalGameplayEvent( GetDynamicNodeEventName(), &e, SBehTreeDynamicNodeCancelEventData::GetStaticClass() );
}

void ISpawnTreeInitializerAI::OnCreatureSpawn( EntitySpawnInfo& entityInfo, CSpawnTreeInstance* instance, CBaseCreatureEntry* entry ) const
{
	// TODO: Change this temporary shit
	CacheStuff();
	IAIParameters* lazyAI = m_lazyAI.Get();
	if ( lazyAI )
	{
		entityInfo.AddHandler( new CAiParamInjectHandler( lazyAI ) );
	}
}

Bool ISpawnTreeInitializerAI::CallActivateOnRestore() const
{
	return true;
}

Bool ISpawnTreeInitializerAI::CallActivateOnSpawn() const
{
	return false;
}
Bool ISpawnTreeInitializerAI::CallActivateOnPoolSpawn() const
{
	return false;
}
Bool ISpawnTreeInitializerAI::CallActivateWhenStealing() const
{
	return true;
}

Bool ISpawnTreeInitializerAI::IsConflicting( const ISpawnTreeInitializer* initializer ) const
{
	const ISpawnTreeInitializerAI* aiInit = Cast< ISpawnTreeInitializerAI >( initializer );
	if ( aiInit )
	{
		return aiInit->m_dynamicTreeParameterName == m_dynamicTreeParameterName;
	}
	return false;
}

#ifndef NO_EDITOR
void ISpawnTreeInitializerAI::OnCreatedInEditor()
{
	TBaseClass::OnCreatedInEditor();

	CObject* context = this;
	CallFunction( context, CNAME( Init ) );
}
#endif 

String ISpawnTreeInitializerAI::GetEditorFriendlyName() const
{
	static String STR( TXT("AI") );
	return STR;
}

String ISpawnTreeInitializerAI::GetBitmapName() const
{
	static String STR( TXT("IMG_SPAWNTREE_AI_0") );
	return STR;
}

