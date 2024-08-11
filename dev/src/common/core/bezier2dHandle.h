// Copyright © 2014 CD Projekt Red. All Rights Reserved.

#pragma once

#include "math.h"

/*
Bezier 2d handle.

Tangents are weighted, i.e. their magnitude is meaningful.
*/
class Bezier2dHandle
{
public:
	Bezier2dHandle();
	Bezier2dHandle( Vector2 incomingTangent, Vector2 outgoingTangent );
	~Bezier2dHandle();

	// compiler generated cctor is ok
	// compiler generated op= is ok

	Vector2 GetIncomingTangent() const;
	Vector2 GetOutgoingTangent() const;

	void SetIncomingTangent( Vector2 incomingTangent );
	void SetOutgoingTangent( Vector2 outgoingTangent );

private:
	Vector2 m_incomingTangent;							// Incoming tangent (direction and magnitude). ControlPointPrev = ControlPointCurr - m_incomingTangent.
	Vector2 m_outgoingTangent;							// Outgoing tangent (direction and magnitude). ControlPointNext = ControlPointCurr + m_outgoingTangent.

	DECLARE_RTTI_SIMPLE_CLASS( Bezier2dHandle )
};

BEGIN_CLASS_RTTI( Bezier2dHandle )
	PROPERTY( m_incomingTangent )
	PROPERTY( m_outgoingTangent )
END_CLASS_RTTI();

Vector2 GetIncomingTangentControlPoint( const Bezier2dHandle& bezierHandle, Vector2 bezierHandlePosition );
Vector2 GetOutgoingTangentControlPoint( const Bezier2dHandle& bezierHandle, Vector2 bezierHandlePosition );

// =================================================================================================
// implementation
// =================================================================================================

/*
Ctor - doesn't initialize handle.
*/
RED_INLINE Bezier2dHandle::Bezier2dHandle()
{}

/*
Ctor - initializes handle with specified tangents.
*/
RED_INLINE Bezier2dHandle::Bezier2dHandle( Vector2 incomingTangent, Vector2 outgoingTangent )
: m_incomingTangent( incomingTangent )
, m_outgoingTangent( outgoingTangent )
{}

/*
Dtor.
*/
RED_INLINE Bezier2dHandle::~Bezier2dHandle()
{}

/*
Gets incoming tangent.
*/
RED_INLINE Vector2 Bezier2dHandle::GetIncomingTangent() const
{
	return m_incomingTangent;
}

/*
Gets outgoing tangent.
*/
RED_INLINE Vector2 Bezier2dHandle::GetOutgoingTangent() const
{
	return m_outgoingTangent;
}

/*
Sets incoming tangent.
*/
RED_INLINE void Bezier2dHandle::SetIncomingTangent( Vector2 incomingTangent )
{
	m_incomingTangent = incomingTangent;
}

/*
Sets outgoing tangent.
*/
RED_INLINE void Bezier2dHandle::SetOutgoingTangent( Vector2 outgoingTangent )
{
	m_outgoingTangent = outgoingTangent;
}

/*
Gets control point associated with incoming tangent.
*/
RED_INLINE Vector2 GetIncomingTangentControlPoint( const Bezier2dHandle& bezierHandle, Vector2 bezierHandlePosition )
{
	return bezierHandlePosition - bezierHandle.GetIncomingTangent();
}

/*
Gets control point associated with outgoing tangent.
*/
RED_INLINE Vector2 GetOutgoingTangentControlPoint( const Bezier2dHandle& bezierHandle, Vector2 bezierHandlePosition )
{
	return bezierHandlePosition + bezierHandle.GetOutgoingTangent();
}
