#pragma once

class CBoidCone
{
public:
	inline CBoidCone( const Vector2 &position, const Vector2 &forwardVect, const Vector2& rightVect, Float tanHalfConeOpeningAngle, Float cosHalfConeOpeningAngle, Float range  );

	inline Bool		IsPointInCone( const Vector2 &point )const;
	inline Float	ComputeDistanceToCone( const Vector2 &point, Vector2 &normedDistVect, Vector2 &pushVector )const;
private:
	Vector2 m_position;
	Float	m_tanHalfConeOpeningAngle;
	Float	m_cosHalfConeOpeningAngle;
	Vector2 m_forwardVect;
	Vector2 m_rightVect;
	Float	m_range;
};


inline CBoidCone::CBoidCone( const Vector2 &position, const Vector2 &forwardVect, const Vector2& rightVect, Float tanHalfConeOpeningAngle, Float cosHalfConeOpeningAngle, Float range  )
	: m_position( position )
	, m_forwardVect( forwardVect )
	, m_rightVect( rightVect )
	, m_tanHalfConeOpeningAngle( tanHalfConeOpeningAngle )
	, m_cosHalfConeOpeningAngle( cosHalfConeOpeningAngle )
	, m_range( range )
{

}

inline Bool CBoidCone::IsPointInCone( const Vector2 &point )const
{
	const Vector3 distVect( point - m_position );
	const Float squaredDist	= distVect.SquareMag();
	const Float cosLimit	= m_cosHalfConeOpeningAngle;
	if (	squaredDist < m_range * m_range
		&&	squaredDist > NumericLimits< Float >::Epsilon() )
	{
		const Float dist = sqrt( squaredDist );

		const Vector3 normedDistVect	= distVect / dist;				// normalize diff
		const Float dot					= m_forwardVect.Dot( normedDistVect.AsVector2() );
		if ( dot >= cosLimit )
		{
			return true;
		}
	}
	return false;
}
inline Float CBoidCone::ComputeDistanceToCone( const Vector2 &P, Vector2 &normedDistVect, Vector2 &pushVector )const
{
	// N: the nearest point on the cone's axis
	const Vector2 N			= Vector( P ).NearestPointOnEdge( m_position, m_position + m_forwardVect * m_range );
	const Vector2 PPos		= m_position - P;
	const Float sqPPosLength= PPos.SquareMag();

	// d: the distance between N and the cone's position
	const Vector2 posN			= N - m_position;
	const Float d				= (posN).Mag();
	const Vector2 normedPosN	= posN / d;

	const Float dot = normedPosN.Dot(m_forwardVect);
	if ( dot < 0.0f ) // point is below the pointy side of the cone
	{
		normedDistVect		= P - m_position;
		const Float dist	= normedDistVect.Mag();
		normedDistVect		= normedDistVect / dist;
		pushVector			= Vector2( 0.0f, 0.0f );
		ASSERT( dist > 0.0f );
		return dist;
	}
	else if ( sqPPosLength > m_range * m_range ) // point is on top of the cone
	{
		//const Vector2 posP	= P - m_position;
		const Float dist			= sqrt( sqPPosLength );
		const Vector2 normedPPos	= PPos / dist;
		normedDistVect				= normedPPos;
		pushVector					= (m_rightVect * m_rightVect.Dot( -normedPPos )).Normalized() * 0.2f;
		ASSERT( dist > m_range );
		return dist - m_range;
	}
	else // Point on the side of the cone
	{
		// L: the oposite side of the triangle formed by the cone
		const Float L			= 2.0f * m_tanHalfConeOpeningAngle * m_range;
		// l: the distance between N and the side of the cone (orthogonally to the axis ) 
		const Float l			= (L * 0.5f) * (d / m_range); 
		const Vector2 PN		= N - P;
		const Float PNLength	= PN.Mag();
		normedDistVect			= PN / PNLength;
		pushVector				= m_forwardVect * 0.1f;
		return PNLength - l;
	}
}

