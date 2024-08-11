/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Information about noticed object
struct NewNPCNoticedObject
{
	static const Float		VISIBILITY_TEST_DISTANCE_SQRT;

	typedef Uint8 TFlags;

	THandle< CActor >	m_actorHandle;
	Vector				m_lastNoticedPosition;
	Bool				m_isVisible;
	EngineTime			m_lastNoticedTime;
	TFlags				m_flags;

	static const TFlags FLAG_DETECTION_VISION	= FLAG( 0 );
	static const TFlags FLAG_DETECTION_ABSOLUTE	= FLAG( 1 );
	static const TFlags FLAG_DETECTION_FORCED	= FLAG( 2 );	
	static const TFlags MASK_DETECTION			= FLAG_DETECTION_VISION | FLAG_DETECTION_ABSOLUTE | FLAG_DETECTION_FORCED;

	NewNPCNoticedObject() {}

	NewNPCNoticedObject( const THandle< CActor >& actorHandle, const EngineTime &time, TFlags flags )
		: m_actorHandle( actorHandle )
		, m_lastNoticedTime( time )
		, m_flags( flags )
	{
		UpdateLastNoticedPosition();
	}

	RED_INLINE void SetIsVisible( Bool visible ){ m_isVisible = visible; }
	

	String ToString() const;	

	void	UpdateLastNoticedPosition();
	Bool	IsVisible() const;
	Vector	GetKnownPosition() const;
};

typedef TDynArray< NewNPCNoticedObject > TNoticedObjects;