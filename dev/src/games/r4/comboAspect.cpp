
#include "build.h"
#include "comboAspect.h"

IMPLEMENT_ENGINE_CLASS( CComboAspect );

CComboString* CComboAspect::CreateComboString( Bool leftSide )
{
	CComboString* str = CreateObject< CComboString >( this );
	str->SetSide( leftSide );
	m_strings.PushBack( str );
	return str;
}
CComboString* CComboAspect::GetRandomComboString( Bool leftSide ) const
{
	Uint32 n = m_strings.Size();
	Uint32 val = GEngine->GetRandomNumberGenerator().Get< Uint32 >( n );
	Uint32 i = val;
	do
	{
		if ( m_strings[i]->IsLeftSide() == leftSide ) return m_strings[i];
		i = ( i + 1 ) % n;
	}
	while ( i != val );
	return NULL;
}

void CComboAspect::AddLinks( const CName& animationName, const TDynArray< CName >& connections )
{
	ASSERT( animationName != CName::NONE && !connections.Empty() );

	TDynArray< CName >* linkSet = m_links.FindPtr( animationName );
	if ( ! linkSet )
	{
		m_links.Insert( animationName, TDynArray< CName >() );
		linkSet = m_links.FindPtr( animationName );
	}
	linkSet->PushBack( connections );
}

void CComboAspect::AddLink( const CName& animationName, const CName& linkedAnimationName )
{
	ASSERT( animationName != CName::NONE && linkedAnimationName != CName::NONE );

	TDynArray< CName >* linkSet = m_links.FindPtr( animationName );
	if ( ! linkSet )
	{
		m_links.Insert( animationName, TDynArray< CName >() );
		linkSet = m_links.FindPtr( animationName );
	}
	linkSet->PushBack( linkedAnimationName );
}

void CComboAspect::AddHit( const CName& animationName, const CName& hitAnimationName )
{
	ASSERT( animationName != CName::NONE && hitAnimationName != CName::NONE );

	TDynArray< CName >* hitSet = m_hits.FindPtr( animationName );
	if ( ! hitSet )
	{
		m_hits.Insert( animationName, TDynArray< CName >() );
		hitSet = m_hits.FindPtr( animationName );
	}
	hitSet->PushBack( hitAnimationName );
}

const CName& CComboAspect::GetLinkedAnimation( const CName& animationName ) const
{
	const TDynArray< CName >* linkSet = m_links.FindPtr( animationName );
	if ( linkSet )
	{
		return (*linkSet)[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( linkSet->Size() ) ];
	}
	return CName::NONE;
}

const CName& CComboAspect::GetHitAnimation( const CName& animationName ) const
{
	const TDynArray< CName >* hitSet = m_hits.FindPtr( animationName );
	if ( hitSet )
	{
		return (*hitSet)[ GEngine->GetRandomNumberGenerator().Get< Uint32 >( hitSet->Size() ) ];
	}
	return CName::NONE;
}


void CComboAspect::funcCreateComboString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Bool, leftSide, true );
	FINISH_PARAMETERS;

	RETURN_OBJECT( CreateComboString( leftSide ) );
}

void CComboAspect::funcAddLinks( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( TDynArray< CName >, connections, TDynArray< CName >() );
	FINISH_PARAMETERS;

	AddLinks( animationName, connections );
}

void CComboAspect::funcAddLink( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( CName, linkedAnimationName, CName::NONE );
	FINISH_PARAMETERS;

	AddLink( animationName, linkedAnimationName );
}

void CComboAspect::funcAddHit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animationName, CName::NONE );
	GET_PARAMETER( CName, hitAnimationName, CName::NONE );
	FINISH_PARAMETERS;

	AddHit( animationName, hitAnimationName );
}
