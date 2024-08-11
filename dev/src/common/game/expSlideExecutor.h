
#pragma once

#include "expSlider.h"
#include "expSingleAnimExecutor.h"

class ExpSlideExecutor : public ExpSingleAnimExecutor
{
protected:
	const IExploration*				m_exploration;
	const IExplorationDesc*			m_explorationDesc;
	Vector							m_edgeOffsetUserMS;
	Vector							m_offsetExpOwnerMS;
	Bool							m_alignTransToEdge;
	Bool							m_alignRotToEdge;
	Float							m_alignRotToEdgeExceeding;		
	Bool							m_blockCloseToEnd; // blocks being close to end, moves inside edge
	Float							m_minDistBeyondEdge;

	Vector							m_offset;
	Vector							m_offsetWithMotion;
	Bool							m_side;
	Vector							m_pointOnEdge;
	Matrix							m_edgeMat;
	Vector							m_collisionPoint;
	Float							m_targetYaw;
	Float							m_targetYawOffset;

	Float							m_timeStartTrans;
	Float							m_timeEndTrans;
	Float							m_timeStartRot;
    Float							m_timeEndRot;
	Float							m_timeStartToCollision;
	Float							m_timeEndToCollision;
	Float							m_timeSyncTrans;
	Float							m_timeSyncRot;

#ifdef EXPLORATION_DEBUG
	Bool							m_isLeftBone;
	Vector							m_pointBoneOnEdge;
	Vector							m_pointBoneAfterSlide;
	Vector							m_pointEntAfterSlide;
#endif

	ExpSimpleSlider					m_slider;

public:
	ExpSlideExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.f, Bool swapSide = false, Bool alignWhenCloseToEnd = false, Bool blockCloseToEnd = false, Bool alignTowardsInside = false );
	~ExpSlideExecutor();

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

	virtual void GenerateDebugFragments( CRenderFrame* frame );

	void UseYawOffset(Float _offset) { m_targetYawOffset = _offset; }
	void SetMinDistBeyondEdge(Float _minDistBeyondEdge) { m_minDistBeyondEdge = _minDistBeyondEdge; }

#ifdef EXPLORATION_DEBUG
protected:
	virtual Bool UpdateAnimation( Float dt, Float& timeRest, ExpExecutorUpdateResult& result );
#endif

protected:
	void CalcPointOnEdge( Vector& p, Float& yaw, Matrix& edgeMat, Bool& closeToEnd ) const;
	void UpdatePointOnEdge( Vector& p, Float& yaw, Matrix& edgeMat ) const;
	void EditBoneOffset( const AnimQsTransform& motionEx, AnimQsTransform& offset, AnimQsTransform& offsetWithMotion ) const;
	Bool CalcSide() const;
};
