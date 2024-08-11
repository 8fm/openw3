#pragma once

#include "expIntarface.h"
#include "../core/dynarray.h"

class IExpTransition;
class CEntity;

class ExpOracle : public Red::System::NonCopyable
{
public:
	virtual ~ExpOracle(){}
	virtual void FilterExplorations( SExplorationQueryToken & token, const IExplorationList& list, ExpRelativeDirection direction, const CEntity* entity ) const = 0;

	virtual IExpExecutor* CreateTransition( const IExploration* form, const IExploration* to, ExecutorSetup& setup ) const = 0;

	virtual Bool RequiresMoveTo( const SExplorationQueryToken & token, Vector & outToLoc, Float & outReqDist ) const = 0 ;
};

//////////////////////////////////////////////////////////////////////////

class EdExplorationDesc : public IExplorationDesc
{
public:
	Bool	m_horizontal;
	ExpDoubleSided	m_doubleSided;
	CName	m_boneLeft;
	CName	m_boneRight;
    Vector	m_edgeOffset;
	Vector	m_offsetMS;
    Float	m_edgeYawOffset;
	Bool	m_useEdgeOffsetX;
	Bool	m_useEdgeOffsetY;
	Bool	m_useEdgeOffsetZ;
	Bool	m_useMotionExX;
	Bool	m_useMotionExY;
	Bool	m_useMotionExZ;
	Float	m_edgeStartPointOffset;
	Float	m_edgeEndPointOffset;
	Float	m_edgeGranularity;
	Bool	m_alignTransToEdge;
	Bool	m_alignRotToEdge;
	Float	m_alignRotToEdgeExceeding;
	Float	m_blendIn;
	Float	m_blendOut;
	Float	m_earlyEndOffset; // offset to the end - animation starts to blend out at this point (or sooner if this is longer than blend out)
	Float	m_testDist;
	Float	m_testDistSpeed; // border speed (see below)
	Float	m_testDistAboveSpeed; // distance when entity is above this speed
	Float	m_testDistMove; // move when withing this distance
	Float	m_testConeAngleHalf;
	ExpZComparision	m_zTest;
	Float	m_zParam;
	Float	m_zMaxDiff; // maximum difference in z (to disallow jumping to edge from 80 meters below, if 0 - it is allowed)
	CName	m_raiseBehaviorEventAtEnd;
	CName	m_callScriptEventAtEnd;

	EdExplorationDesc()
		: m_horizontal( true )
		, m_doubleSided( EXPDS_DOUBLE )
		, m_edgeOffset( Vector::ZERO_3D_POINT )
		, m_offsetMS( Vector::ZERO_3D_POINT )
        , m_edgeYawOffset( 0.f )
		, m_useEdgeOffsetX( true )
		, m_useEdgeOffsetY( true )
		, m_useEdgeOffsetZ( true )
		, m_useMotionExX( false )
		, m_useMotionExY( false )
		, m_useMotionExZ( false )
		, m_edgeStartPointOffset( 0.f )
		, m_edgeEndPointOffset( 0.f )
		, m_edgeGranularity( 0.f )
		, m_alignTransToEdge( true )
		, m_alignRotToEdge( true )
		, m_alignRotToEdgeExceeding( 0.0f )
		, m_blendIn( 0.f )
		, m_blendOut( 0.f )
		, m_earlyEndOffset( 0.f )
		, m_testDist( 1.f )
		, m_testDistSpeed( 0.f )
		, m_testDistAboveSpeed( 0.f )
		, m_testDistMove( 0.f )
		, m_testConeAngleHalf( 45.f )
		, m_zTest( EXPZCMP_ANY )
		, m_zParam( 0.0f )
		, m_zMaxDiff( 0.0f )
	{

	}

public:
	virtual ExpZComparision GetZComparision() const
	{
		return m_zTest;
	}
	virtual Float GetZComparisionParam() const
	{
		return m_zParam;
	}
	virtual Float GetZMaxDifference() const
	{
		return m_zMaxDiff;
	}

	virtual Bool IsHorizontal() const
	{
		return m_horizontal;
	}

	virtual Bool IsDoubleSided( Bool forPlayer ) const
	{
		return forPlayer ? ( m_doubleSided == EXPDS_DOUBLE || m_doubleSided == EXPDS_DOUBLE_FOR_PLAYER ) : ( m_doubleSided == EXPDS_DOUBLE || m_doubleSided == EXPDS_DOUBLE_FOR_AI );
	}

	virtual const CName& GetEdgeBone( Bool leftSide ) const
	{
		return leftSide ? m_boneLeft : m_boneRight;
	}

	virtual const Vector& GetEdgeOffset() const
	{
		return m_edgeOffset;
	}

	virtual const Vector& GetOffsetInModelSpace() const
	{
		return m_offsetMS;
	}

    virtual Float GetEdgeYawOffset() const
    {
        return m_edgeYawOffset;
    }

	virtual void UseEdgeOffsetAxis( Bool& x, Bool& y, Bool& z ) const
	{
		x = m_useEdgeOffsetX;
		y = m_useEdgeOffsetY;
		z = m_useEdgeOffsetZ;
	}

	virtual void UseMotionOffsetAxis( Bool& x, Bool& y, Bool& z ) const
	{
		x = m_useMotionExX;
		y = m_useMotionExY;
		z = m_useMotionExZ;
	}

	virtual void GetEdgePointsOffset( Float& p1, Float& p2 ) const
	{
		p1 = m_edgeStartPointOffset;
		p2 = m_edgeEndPointOffset;
	}

	virtual Bool UseEdgeGranularity( Float& gran ) const
	{
		gran = m_edgeGranularity;
		return m_edgeGranularity > 0.f;
	}

	virtual Bool AlignTransToEdge() const
	{
		return m_alignTransToEdge;
	}

	virtual Bool AlignRotToEdge() const
	{
		return m_alignRotToEdge;
	}

	virtual Float AlignRotToEdgeExceeding() const
	{
		return m_alignRotToEdgeExceeding;
	}

	virtual void GetBlendsAndEnd( Float& in, Float& out, Float& earlyEndOffset ) const
	{
		in = m_blendIn;
		out = m_blendOut;
		earlyEndOffset = m_earlyEndOffset;
	}

	virtual void GetDistAndAngleToTest( CEntity const * entity, Float& dist, Float& distMoveTo, Float& coneAngleHalf, Bool ignoreSpeed = false ) const;

	virtual void GetEventsAtEnd( CName& raiseBehaviorEventAtEnd, CName& callScriptEventAtEnd ) const
	{
		raiseBehaviorEventAtEnd = m_raiseBehaviorEventAtEnd;
		callScriptEventAtEnd = m_callScriptEventAtEnd;
	}
};

class ExpOracleHardCoded : public ExpOracle
{
	const TDynArray< EdExplorationDesc* >& m_descriptions;

public:
	ExpOracleHardCoded();

public:
	virtual void FilterExplorations( SExplorationQueryToken & token, const IExplorationList& list, ExpRelativeDirection direction, const CEntity* entity ) const;

	virtual IExpExecutor* CreateTransition( const IExploration* form, const IExploration* to, ExecutorSetup& setup ) const;

	IExpExecutor* CreateTransitionJump( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const;
	IExpExecutor* CreateTransitionLedge( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const;
	IExpExecutor* CreateTransitionLedge_Old( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const;
	IExpExecutor* CreateTransitionLedge_New( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo ) const;
	IExpExecutor* CreateTransitionLadder( const IExploration* from, const IExploration* to, ExecutorSetup& setup, const IExplorationDesc* descTo, Bool above, Bool frontSide, Float sideOfApproach, Float distTo  ) const;
	virtual Bool RequiresMoveTo( const SExplorationQueryToken & token, Vector & outToLoc, Float & outReqDist ) const;

private:
	const IExplorationDesc* GetDesc( Int32 id ) const;

	void CalcAbove( const SExplorationQueryToken & token, Bool& above ) const;

	void CalcDistAndSideTo( const SExplorationQueryToken & token, const ExecutorSetup& setup, Float& dist, Float& sideOfApproach, Bool& frontSide, Bool& above ) const;

	void CalcDistAndAngle( const SExplorationQueryToken & token, Float& dist, Float& angle, Float& desAngle, Bool& frontSide, Vector& location, Vector& normal, Float locationOffEnds = 0.0f ) const;
	
	void CalcLocation( const SExplorationQueryToken & token, Vector& location, Vector& normal, Float offEnds, Bool getLocationInForwardDirection ) const;

	void CalcHeightLocationAndNormal( const SExplorationQueryToken & token, Float distFromEdge, Float& height, Float* outHeightBehind, Vector & pointOnEdge, Vector & normal, Float locationOffEnds = 0.0f ) const;

	void CalcDistAngleHeightAndLocation( const SExplorationQueryToken & token, Float& dist, Float& angle, Float& desAngle, Bool& frontSide, Vector& location, Float locationOffEnds, Float distFromEdgeForHeight, Float& height ) const;

	void CalcHeightUsingLocationAndNormal( const SExplorationQueryToken & token, Vector const & pointOnEdge, Vector const & normal, Float distFromEdge, Float& height, Float* outHeightBehind ) const;

	void CalcDistAndAngleUsingLocationAndNormal( const SExplorationQueryToken & token, Vector const & pointOnEdge, Vector const & normal, Float& dist, Float& angle, Float& desAngle, Bool& frontSide ) const;

	void AdjustPointOnEdgeToLadderStep( const SExplorationQueryToken & token, Vector & pointOnEdge ) const;

	void UpdateExtraDataIn( SExplorationQueryToken & token, Float expHeight ) const;
};
