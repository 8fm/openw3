
#include "build.h"
#include "controlRigBehaviorNodeController.h"
#include "behaviorGraphStack.h"
#include "animatedComponent.h"

RED_DEFINE_STATIC_NAME( eHandL );
RED_DEFINE_STATIC_NAME( eHandR );
RED_DEFINE_STATIC_NAME( eHandPosL );
RED_DEFINE_STATIC_NAME( eHandPosR );
RED_DEFINE_STATIC_NAME( eHandWeaponL );
RED_DEFINE_STATIC_NAME( eHandWeaponR );

TCrBehaviorNodeController::TCrBehaviorNodeController()
	: m_component( NULL )
	, m_hasVarHandPosL( false )
	, m_hasVarHandPosR( false )
	, m_hasVarHandL( false )
	, m_hasVarHandR( false )
{

}

TCrBehaviorNodeController::~TCrBehaviorNodeController()
{
	Deinit();
}

void TCrBehaviorNodeController::Init( CAnimatedComponent* component )
{
	m_component = component && component->GetBehaviorStack() ? component : NULL;

	if ( m_component )
	{
		m_hasVarHandL = m_component->GetBehaviorStack()->HasBehaviorFloatVariable( CNAME( eHandL ) );
		m_hasVarHandR = m_component->GetBehaviorStack()->HasBehaviorFloatVariable( CNAME( eHandR ) );

		m_hasVarHandPosL = m_component->GetBehaviorStack()->HasBehaviorFloatVariable( CNAME( eHandPosL ) );
		m_hasVarHandPosR = m_component->GetBehaviorStack()->HasBehaviorFloatVariable( CNAME( eHandPosR ) );
	}
}

void TCrBehaviorNodeController::Deinit()
{
	m_component = NULL;
}

void TCrBehaviorNodeController::SetEffector_HandL( const Vector& positionWS )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandPosL ), positionWS );
	}
}

void TCrBehaviorNodeController::SetEffector_HandR( const Vector& positionWS )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandPosR ), positionWS );
	}
}

void TCrBehaviorNodeController::SetEffectorActive_HandL( Float weight )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandL ), weight );
	}
}

void TCrBehaviorNodeController::SetEffectorActive_HandR( Float weight )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandR ), weight );
	}
}

void TCrBehaviorNodeController::SetWeaponOffset_HandL( Float weight )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandWeaponL ), weight );
	}
}

void TCrBehaviorNodeController::SetWeaponOffset_HandR( Float weight )
{
	if ( m_component )
	{
		m_component->GetBehaviorStack()->SetBehaviorVariable( CNAME( eHandWeaponR ), weight );
	}
}
