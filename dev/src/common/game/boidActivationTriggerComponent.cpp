#include "build.h"
#include "boidActivationTriggerComponent.h"
#include "boidLairEntity.h"


/////////////////////////////////////////////////////////////////////////
// CBoidActivationTriggerComponent
/////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CBoidActivationTriggerComponent );

CBoidActivationTriggerComponent::CBoidActivationTriggerComponent()
	: m_handleEnter( true )
	, m_handleExit( true )
{
	// For optimisation purpose this must be player only
	m_includedChannels = TC_Player;
}

void CBoidActivationTriggerComponent::OnAttachFinished( CWorld* world )
{
	CTriggerAreaComponent::OnAttachFinished( world );
	CEntity* entity = m_lair.Get();
	if ( !entity )
		return;
	IBoidLairEntity* lair = Cast< IBoidLairEntity >( entity );
	if ( !lair )
		return;
	lair->OnActivationTriggerAdded( GetEntity() );
}
void CBoidActivationTriggerComponent::OnDetached( CWorld* world )
{
	CTriggerAreaComponent::OnDetached( world );
	CEntity* entity = m_lair.Get();
	if ( !entity )
		return;
	IBoidLairEntity* lair = Cast< IBoidLairEntity >( entity );
	if ( !lair )
		return;
	lair->OnActivationTriggerRemoved( GetEntity() );
}

void CBoidActivationTriggerComponent::EnteredArea( CComponent* component )
{
	if ( !m_handleEnter )
		return;

	CEntity* entity = m_lair.Get();
	if ( !entity )
		return;
	IBoidLairEntity* lair = Cast< IBoidLairEntity >( entity );
	if ( !lair )
		return;

	lair->OnDynamicComponentEntered( component );

}
void CBoidActivationTriggerComponent::ExitedArea( CComponent* component )
{
	if ( !m_handleExit )
		return;

	CEntity* entity = m_lair.Get();
	if ( !entity )
		return;
	IBoidLairEntity* lair = Cast< IBoidLairEntity >( entity );
	if ( !lair )
		return;

	lair->OnDynamicComponentLeft( component );
}

void CBoidActivationTriggerComponent::OnPostLoad()
{
	TBaseClass::OnPostLoad();
	// HACK W3
	m_includedChannels = TC_Player;
}
