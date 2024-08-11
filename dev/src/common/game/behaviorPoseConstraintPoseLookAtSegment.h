
/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma once

struct SPoseLookAtSegmentData
{
	DECLARE_RTTI_STRUCT( SPoseLookAtSegmentData )

	SPoseLookAtSegmentData()
		: m_up( A_X )
		, m_front( A_Y )
		, m_weight( 1.f )
		, m_bendingMultiplier( 1.f )
		, m_responsiveness( 5.f )
		, m_propagateToChain( false )
		, m_onlyFirst( false )
		, m_angleMaxHor( 90.f )
		, m_angleMaxVer( 90.f )
		, m_angleThresholdDiffHor( 0.f )
		, m_angleThresholdDiffVer( 0.f )
		, m_maxAngleDiffVer( 30.f )
		, m_maxAngleDiffHor( 30.f )
		, m_maxAngleVerToRefPose( 0.f )
		, m_maxAngleHorToRefPose( 0.f )
		, m_segmentLevel( LL_Null )
	{}

	ELookAtLevel m_segmentLevel;
	String	m_boneNameFirst;
	String	m_boneNameLast;
	EAxis	m_up;
	EAxis	m_front;
	Float	m_weight;
	Float	m_bendingMultiplier;
	Float	m_responsiveness;
	Bool	m_propagateToChain;
	Bool	m_onlyFirst;
	Float	m_angleMaxHor;
	Float	m_angleMaxVer;
	Float	m_angleThresholdDiffHor;
	Float	m_angleThresholdDiffVer;
	Float	m_maxAngleDiffVer;
	Float	m_maxAngleDiffHor;
	Float	m_maxAngleVerToRefPose;
	Float	m_maxAngleHorToRefPose;
};

BEGIN_CLASS_RTTI( SPoseLookAtSegmentData );
	PROPERTY_EDIT( m_segmentLevel, TXT( "" ) );
	PROPERTY_EDIT( m_boneNameFirst, TXT( "" ) );
	PROPERTY_EDIT( m_boneNameLast, TXT( "" ) );
	PROPERTY_EDIT( m_weight, TXT("") );
	PROPERTY_EDIT( m_bendingMultiplier, TXT("") );
	PROPERTY_EDIT( m_responsiveness, TXT("") );
	PROPERTY_EDIT( m_propagateToChain, TXT("") );
	PROPERTY_EDIT( m_onlyFirst, TXT("") )
	PROPERTY_EDIT( m_up, TXT("Up direction") );
	PROPERTY_EDIT( m_front, TXT("Front direction") );
	PROPERTY_EDIT( m_angleMaxHor, TXT( "" ) );
	PROPERTY_EDIT( m_angleMaxVer, TXT( "" ) );
	PROPERTY_EDIT( m_angleThresholdDiffHor, TXT( "" ) );
	PROPERTY_EDIT( m_angleThresholdDiffVer, TXT( "" ) );
	PROPERTY_EDIT( m_maxAngleDiffHor, TXT( "" ) );
	PROPERTY_EDIT( m_maxAngleDiffVer, TXT( "" ) );
	PROPERTY_EDIT( m_maxAngleVerToRefPose, TXT( "" ) );
	PROPERTY_EDIT( m_maxAngleHorToRefPose, TXT( "" ) );
END_CLASS_RTTI();

struct SPoseLookAtSegment
{
	DECLARE_RTTI_STRUCT( SPoseLookAtSegment );

	SPoseLookAtSegment();
	SPoseLookAtSegment( Uint32 num, const SPoseLookAtSegmentData* data, const CSkeleton* skeleton );

private:
	const SPoseLookAtSegmentData*	m_data;

public:
	TDynArray< Int32 >				m_bones;
	Float							m_hAngle;
	Float							m_vAngle;
	Bool							m_limited;
	Uint32							m_num;

	Float							m_progress;
	Float							m_hPrevAngle;
	Float							m_vPrevAngle;

	Bool IsValid() const;

	RED_INLINE ELookAtLevel GetSegmentLevel()		{ return m_data ? m_data->m_segmentLevel : LL_Null; }
	RED_INLINE EAxis	GetFront() const			{ return m_data ? m_data->m_front : A_Y; }
	RED_INLINE EAxis	GetUp() const				{ return m_data ? m_data->m_up : A_X; }
	RED_INLINE Float	GetResponsiveness() const	{ return m_data ? m_data->m_responsiveness : 5.f; }
	RED_INLINE Bool	UsePropagation() const		{ return m_data ? m_data->m_propagateToChain : false; }
	RED_INLINE Bool	OnlyFirstBone() const		{ return m_data ? m_data->m_onlyFirst : false; }
	RED_INLINE Float	GetWeight() const			{ return m_data ? m_data->m_weight : 0.f; }
	RED_INLINE Float	GetMultiplier() const		{ return m_data ? m_data->m_bendingMultiplier : 0.f; }
	RED_INLINE Float	GetAngleMaxHor() const		{ return m_data ? m_data->m_angleMaxHor : 0.f; }
	RED_INLINE Float	GetAngleMaxVer() const		{ return m_data ? m_data->m_angleMaxVer : 0.f; }
	RED_INLINE Float	GetMaxAngleDiffHor() const	{ return m_data ? m_data->m_maxAngleDiffHor : 0.f; }
	RED_INLINE Float	GetMaxAngleDiffVer() const	{ return m_data ? m_data->m_maxAngleDiffVer : 0.f; }
	RED_INLINE Float	GetAngleThresholdDiffHor() const	{ return m_data ? m_data->m_angleThresholdDiffHor : 0.f; }
	RED_INLINE Float	GetAngleThresholdDiffVer() const	{ return m_data ? m_data->m_angleThresholdDiffVer : 0.f; }
	RED_INLINE Bool	UseRefPoseLimit() const		{ return m_data ? GetMaxAngleHorToRefPose() > 0.f || GetMaxAngleVerToRefPose() > 0.f : false; }
	RED_INLINE Float	GetMaxAngleHorToRefPose() const	{ return m_data ? m_data->m_maxAngleHorToRefPose : 0.f; }
	RED_INLINE Float	GetMaxAngleVerToRefPose() const	{ return m_data ? m_data->m_maxAngleVerToRefPose : 0.f; }
	Vector GetReferenceLookDir( const AnimQsTransform& boneLS ) const;
	Vector GetReferenceUpDir( const AnimQsTransform& boneLS ) const;
};

BEGIN_CLASS_RTTI( SPoseLookAtSegment );
END_CLASS_RTTI();
