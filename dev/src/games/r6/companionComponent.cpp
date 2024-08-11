#include "build.h"

#include "companionComponent.h"

IMPLEMENT_ENGINE_CLASS( CCompanionComponent );


CCompanionComponent::CCompanionComponent()
{
	m_members.Resize( MAX_COMPANON_SIZE );

	for ( Uint32 i = 0; i < m_members.Size(); i++ )
	{
		m_members[ i ] = CName::NONE;
	}
}

void CCompanionComponent::SetMember( CName name, Int32 slot )
{
	m_members[ slot ] = name;
}

CName CCompanionComponent::GetMember( Int32 slot ) const
{
	return m_members[ slot ];
}

Bool CCompanionComponent::HasMember( CName name ) const
{
	Int32 index = m_members.GetIndex( name );
	return ( index != -1 );
}

Int32 CCompanionComponent::GetMemberNum() const
{
	Int32 num = 0;
	for ( Uint32 i = 0; i < m_members.Size(); i++ )
	{
		if ( m_members[ i ] != CName::NONE )
		{
			num++;
		}
	}
	return num;
}

void CCompanionComponent::funcSetMember( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER( Int32, slot, 0 );
	FINISH_PARAMETERS;

	if ( slot < 0 || slot >= MAX_COMPANON_SIZE )
	{
		RETURN_BOOL( false );
	}

	SetMember( name, slot );

	RETURN_BOOL( true );
}

void CCompanionComponent::funcGetMember( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, slot, 0 );
	FINISH_PARAMETERS;

	CName name = CName::NONE;
	if ( slot >= 0 && slot < MAX_COMPANON_SIZE )
	{
		name = GetMember( slot );
	}

	RETURN_NAME( name );
}

void CCompanionComponent::funcHasMember( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasMember( name ) );
}
