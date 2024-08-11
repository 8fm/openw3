#include "build.h"
#include "effectComponent.h"

IMPLEMENT_ENGINE_CLASS( CR4EffectComponent );

CR4EffectComponent::CR4EffectComponent()
{

}

void CR4EffectComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PlayEffect();
}

void CR4EffectComponent::OnDetached( CWorld* world )
{
	StopEffect();

	TBaseClass::OnDetached( world );
}


void CR4EffectComponent::PlayEffect()
{
	CEntity* entity = GetEntity();

	if ( entity && m_effectName != CName::NONE )
	{
		entity->PlayEffect( m_effectName, CName::NONE, m_effectTarget.Get(), m_targetBone );
	}
}

void CR4EffectComponent::StopEffect()
{
	CEntity* entity = GetEntity();
	if ( entity && m_effectName != CName::NONE )
	{
		entity->StopEffect( m_effectName );
	}
}

void CR4EffectComponent::OnSaveGameplayState( IGameSaver* saver )
{
	TBaseClass::OnSaveGameplayState( saver );

	CGameSaverBlock block( saver, GetClass()->GetName() );

	saver->WriteValue( CNAME( name ), m_effectName );
	saver->WriteValue( CNAME( handle ), m_effectTarget );
	saver->WriteValue( CNAME( bone ), m_targetBone );
}

void CR4EffectComponent::OnLoadGameplayState( IGameLoader* loader )
{
	TBaseClass::OnLoadGameplayState( loader );

	CGameSaverBlock block( loader, GetClass()->GetName() );

	loader->ReadValue( CNAME( name ), m_effectName );
	loader->ReadValue( CNAME( handle ), m_effectTarget );
	loader->ReadValue( CNAME( bone ), m_targetBone );
}

void CR4EffectComponent::funcPlayEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER( THandle< CEntity >, target, NULL );
	GET_PARAMETER( CName, targetBone, CName::NONE );
	FINISH_PARAMETERS;

	StopEffect();

	m_effectTarget.Set( target );
	m_effectName = effectName;
	m_targetBone = targetBone;

	PlayEffect();
}

void CR4EffectComponent::funcStopEffect( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	StopEffect();

	m_effectTarget.Set( nullptr );
	m_effectName = CName::NONE;
	m_targetBone = CName::NONE;
}
