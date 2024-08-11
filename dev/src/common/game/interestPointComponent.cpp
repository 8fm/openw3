#include "build.h"
#include "interestPointComponent.h"

#include "potentialField.h"
#include "gameWorld.h"

IMPLEMENT_ENGINE_CLASS( CInterestPoint );

CInterestPoint::CInterestPoint()
	: m_potentialField( NULL )
{
}

CInterestPointInstance* CInterestPoint::CreateInstance( CObject* parent, const Vector& position, Float timeout ) 
{
	CInterestPointInstance* instance = CreateObject< CInterestPointInstance > ( parent );
	instance->Bind( this, position, timeout );
	return instance;
}

CInterestPointInstance* CInterestPoint::CreateInstance( CObject* parent, const THandle< CNode >& node, Float timeout )
{
	CInterestPointInstance* instance = CreateObject< CInterestPointInstance > ( parent );
	instance->Bind( this, node, timeout );
	return instance;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CScriptedInterestPoint );

CScriptedInterestPoint::CScriptedInterestPoint()
{

}

RED_DEFINE_STATIC_NAME( SetupInstance );

CInterestPointInstance* CScriptedInterestPoint::CreateInstance( CObject* parent, const THandle< CNode >& node, Float timeout /*= 0.0f */ )
{
	CInterestPointInstance* ip = TBaseClass::CreateInstance( parent, node, timeout );
	CallFunction( this, CNAME( SetupInstance ), THandle< CInterestPointInstance >( ip ), THandle< CObject >( parent ) );
	return ip;
}

CInterestPointInstance* CScriptedInterestPoint::CreateInstance( CObject* parent, const Vector& position, Float timeout /*= 0.0f */ )
{
	CInterestPointInstance* ip = TBaseClass::CreateInstance( parent, position, timeout );
	CallFunction( this, CNAME( SetupInstance ), THandle< CInterestPointInstance >( ip ), THandle< CObject >( parent ) );
	return ip;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CInterestPointInstance );

CInterestPointInstance::CInterestPointInstance()
: m_parent( NULL )
, m_timeToLive( 0.0f )
{
}

CInterestPointInstance::~CInterestPointInstance()
{
}

void CInterestPointInstance::Bind( const THandle< CInterestPoint >& parent, const Vector& position, Float timeout )
{
	ASSERT( m_parent.Get() == NULL, TXT( "This instance is already bound" ) );

	m_parent = parent;
	m_position = position;
	m_timeToLive = timeout;
}

void CInterestPointInstance::Bind( const THandle< CInterestPoint >& parent, const THandle< CNode >& node, Float timeout )
{
	ASSERT( m_parent.Get() == NULL, TXT( "This instance is already bound" ) );

	m_parent = parent;
	m_parentNode = node;
	m_timeToLive = timeout;
}

void CInterestPointInstance::Rebind( const THandle< CNode >& node, Float timeout )
{
	m_parentNode = node;
	m_timeToLive = timeout;
}

void CInterestPointInstance::Rebind( const Vector& position, Float timeout )
{
	m_parentNode = NULL;
	m_position = position;
	m_timeToLive = timeout;
}

Bool CInterestPointInstance::Update( Float timeElapsed )
{
	// If handle was set test it
	THandle< CNode > nullHandle;
	if( m_parentNode != nullHandle )
	{
		if( !m_parentNode.Get() )
		{
			return true;
		}
	}

	m_timeToLive -= timeElapsed;
	return ( m_timeToLive <= 0.0f );
}

const CName& CInterestPointInstance::GetName() const
{
	CInterestPoint* parent = m_parent.Get();
	if ( parent )
	{
		return parent->GetFieldName();
	}
	else
	{
		return CName::NONE;
	}
}

Vector CInterestPointInstance::GetWorldPosition() const
{
	CNode* parentNode = m_parentNode.Get();
	if ( parentNode )
		return parentNode->GetWorldPosition();
	else
		return m_position;
}

Bool CInterestPointInstance::RangeTest( const Vector& pos ) const
{
	Vector localFieldPos;
	CNode* parentNode = m_parentNode.Get();
	if ( parentNode )
	{
		localFieldPos = pos - parentNode->GetWorldPosition();
	}
	else
	{
		localFieldPos = pos - m_position;
	}

	CInterestPoint* parent = m_parent.Get();
	if ( parent )
	{
		const IPotentialField* field = parent->GetField();
		if( field )
		{
			return field->RangeTest( localFieldPos, m_fieldStrengthMultiplier );
		}
	}

	return false;
}

Float CInterestPointInstance::FieldStrength( const Vector& pos ) const
{
	Vector localFieldPos;
	CNode* parentNode = m_parentNode.Get();
	if ( parentNode )
	{
		localFieldPos = pos - parentNode->GetWorldPosition();
	}
	else
	{
		localFieldPos = pos - m_position;
	}

	CInterestPoint* parent = m_parent.Get();
	if ( parent )
	{
		const IPotentialField* field = parent->GetField();
		if( field )
		{
			return field->GetPotentialValue( localFieldPos, m_fieldStrengthMultiplier );
		}
	}

	return 0.0f;
}

void CInterestPointInstance::funcGetParentPoint( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( const_cast< CInterestPoint* >( m_parent.Get() ) );
}

void CInterestPointInstance::funcGetWorldPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CNode* parentNode = m_parentNode.Get();
	if ( parentNode )
	{
		RETURN_STRUCT( Vector, parentNode->GetWorldPosition() );
	}
	else
	{
		RETURN_STRUCT( Vector, m_position );
	}
}

void CInterestPointInstance::funcGetNode( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( m_parentNode.Get() );
}

void CInterestPointInstance::funcGetGeneratedFieldName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( GetName() )
}

void CInterestPointInstance::funcGetFieldStrength( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	RETURN_FLOAT( FieldStrength( position ) );

}

void CInterestPointInstance::funcSetFieldStrengthMultiplier( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, parameter, 0.0f );
	FINISH_PARAMETERS;

	SetFieldStrengthMultiplier( parameter );
}

void CInterestPointInstance::funcGetFieldStrengthMultiplier( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float param = GetFieldStrengthMultiplier();
	RETURN_FLOAT( param );
}

void CInterestPointInstance::funcSetTestParameter( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, testParameter, 0.0f );
	FINISH_PARAMETERS;

	SetTestParameter( testParameter );
}

void CInterestPointInstance::funcGetTestParameter( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float param = GetTestParameter(); 
	RETURN_FLOAT( param );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CInterestPointComponent );

CInterestPointComponent::CInterestPointComponent()
: m_active( true )
{
}

void CInterestPointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CInterestPointComponent_OnAttached );

	if ( GCommonGame && GCommonGame->IsActive() && GCommonGame->GetActiveWorld() )
	{
		GCommonGame->GetActiveWorld()->AddInterestPoint( this );
	}
}

void CInterestPointComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	if ( GCommonGame && GCommonGame->IsActive() && GCommonGame->GetActiveWorld() )
	{
		GCommonGame->GetActiveWorld()->RemoveInterestPoint( this );
	}
}
