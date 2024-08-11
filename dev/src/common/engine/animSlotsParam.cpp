#include "build.h"
#include "animSlotsParam.h"

IMPLEMENT_ENGINE_CLASS( CAnimationSlots );
IMPLEMENT_ENGINE_CLASS( CAnimSlotsParam );

CAnimationSlots* CAnimSlotsParam::AddAnimationSlots( const CName& animSlotsName )
{
	Int32 index = static_cast< Int32 >( m_animationSlots.Grow( 1 ) );
	m_animationSlots[ index ] = CreateObject< CAnimationSlots >( this );
	m_animationSlots[ index ]->m_name = animSlotsName;
	return m_animationSlots[ index ];
}

Bool CAnimSlotsParam::RemoveAnimationSlots( const CName& animSlotsName )
{
	for ( Uint32 i = 0; i < m_animationSlots.Size(); ++i )
	{
		if ( m_animationSlots[ i ]->m_name == animSlotsName )
		{
			m_animationSlots[ i ]->Discard();
			m_animationSlots.Erase( m_animationSlots.Begin() + i );
			return true;
		}
	}

	return false;
}

Bool CAnimSlotsParam::HasAnimSlots( const CName& animSlotsName ) const
{
	for ( Uint32 i = 0; i < m_animationSlots.Size(); ++i )
	{
		if ( m_animationSlots[ i ]->m_name == animSlotsName )
		{
			return true;
		}
	}

	return false;
}

const CAnimationSlots* CAnimSlotsParam::FindAnimationSlots( const CName& animSlotsName ) const
{
	for ( Uint32 i = 0; i < m_animationSlots.Size(); ++i )
	{
		if ( m_animationSlots[ i ]->m_name == animSlotsName )
		{
			return m_animationSlots[i];
		}
	}

	return NULL;
}