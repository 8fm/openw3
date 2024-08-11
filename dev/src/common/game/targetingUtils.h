/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

///////////////////////////////////////////////////////////////////////////////

struct STargetSelectionWeights
{
	DECLARE_RTTI_STRUCT( STargetSelectionWeights );

	STargetSelectionWeights()
		: m_angleWeight( 0.0f )
		, m_distanceWeight( 0.0f )
		, m_distanceRingWeight( 0.0f )
	{}

	Float	m_angleWeight;
	Float	m_distanceWeight;
	Float	m_distanceRingWeight;
};

BEGIN_CLASS_RTTI( STargetSelectionWeights );
	PROPERTY( m_angleWeight );
	PROPERTY( m_distanceWeight );
	PROPERTY( m_distanceRingWeight );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct STargetSelectionData
{
	DECLARE_RTTI_STRUCT( STargetSelectionData );

	STargetSelectionData()
		: m_sourcePosition( Vector::ZEROS )
		, m_headingVector( Vector::ZEROS )
		, m_closeDistance( 6.0f ) // const value taken from scripts logic
		, m_softLockDistance( 0.0f )
	{}

	Vector	m_sourcePosition;
	Vector	m_headingVector;
	Float   m_closeDistance;
	Float	m_softLockDistance;
};

BEGIN_CLASS_RTTI( STargetSelectionData );
	PROPERTY( m_sourcePosition );
	PROPERTY( m_headingVector );
	PROPERTY( m_closeDistance );
	PROPERTY( m_softLockDistance );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CTargetingUtils
{

public:

	static Float CalcSelectionPriority( CNode* node, const STargetSelectionWeights& selectionWeights, const STargetSelectionData& selectionData );
};