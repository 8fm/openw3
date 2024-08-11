/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newNpcNoticedObject.h"

const Float		NewNPCNoticedObject::VISIBILITY_TEST_DISTANCE_SQRT = 0.25f * 0.25f;

String NewNPCNoticedObject::ToString() const
{
	static const String F_VISION = TXT("VISION,");
	static const String F_ABSOLUTE = TXT("ABSOLUTE,");
	static const String F_FORCED = TXT("FORCED,");

	CActor* actor = m_actorHandle.Get();
	Float relativeTime = m_lastNoticedTime - GGame->GetEngineTime();

	String flags;
	if( m_flags & NewNPCNoticedObject::FLAG_DETECTION_VISION ) flags += F_VISION;
	if( m_flags & NewNPCNoticedObject::FLAG_DETECTION_ABSOLUTE ) flags += F_ABSOLUTE;
	if( m_flags & NewNPCNoticedObject::FLAG_DETECTION_FORCED ) flags += F_FORCED;	

	return String::Printf( TXT("Actor: %s, Last noticed: %g (%g), Flags: %s"), actor ? actor->GetName().AsChar() : NULL, Float(m_lastNoticedTime), relativeTime, flags.AsChar() );
}

void NewNPCNoticedObject::UpdateLastNoticedPosition()
{
	if( m_actorHandle.Get() )
	{
		m_lastNoticedPosition = m_actorHandle.Get()->GetWorldPosition();
	}
}

Bool NewNPCNoticedObject::IsVisible() const
{
	return m_isVisible;
	/*if( m_isVisible )
		return true;

	CActor* noticedActor = m_actorHandle.Get();
	if( !noticedActor )
		return false;

	Float distanceSqrt = m_lastNoticedPosition.DistanceSquaredTo( noticedActor->GetWorldPosition() );

	return distanceSqrt <= VISIBILITY_TEST_DISTANCE_SQRT;
	*/
}

Vector	NewNPCNoticedObject::GetKnownPosition() const
{
	if( m_isVisible )
	{
		CActor* visibleActor = m_actorHandle.Get();
		if( visibleActor )
			return visibleActor->GetWorldPosition();
	}
	return m_lastNoticedPosition;
}