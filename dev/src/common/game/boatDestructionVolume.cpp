#include "build.h"
#include "boatDestructionVolume.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SBoatDestructionVolume );

//////////////////////////////////////////////////////////////////////////

const Float SBoatDestructionVolume::DEFAULT_HEALTH = 100.0f;

//////////////////////////////////////////////////////////////////////////

SBoatDestructionVolume::SBoatDestructionVolume()
    : m_volumeLocalPosition( Vector( -0.5f, -0.5f, -0.5f ) )
    , m_volumeCorners( Vector( 0.5f, 0.5f, 0.5f ) )
    , m_areaHealth( DEFAULT_HEALTH )
{}

//////////////////////////////////////////////////////////////////////////

Bool SBoatDestructionVolume::IsLocalPointInVolume( const Vector& point ) const
{
    const Box box( m_volumeLocalPosition - m_volumeCorners * 0.5f, m_volumeLocalPosition + m_volumeCorners * 0.5f );
    return box.ContainsExcludeEdges( point );
}

//////////////////////////////////////////////////////////////////////////
