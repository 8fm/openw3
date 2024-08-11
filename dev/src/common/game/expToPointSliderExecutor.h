
#pragma once

#include "expSlider.h"
#include "expSingleAnimExecutor.h"

class ExpToPointSliderExecutor : public ExpSingleAnimExecutor
{
protected:
	const IExploration*				m_exploration;
	const IExplorationDesc*			m_explorationDesc;
	Bool							m_alignRotToEdge;
	Float							m_alignRotToEdgeExceeding;		

	Bool							m_hasValidEndPoint;
	Vector							m_endPoint;
	Float							m_targetYaw;
    Float                           m_timeStartTrans;
    Float                           m_timeEndTrans;

	ExpSimpleSlider					m_slider;

public:
	ExpToPointSliderExecutor( const IExploration* e, const IExplorationDesc* desc, const ExecutorSetup& setup, const CName& animName, Float blendIn = 0.f, Float blendOut = 0.f, Float earlyEndOffset = 0.f );
	~ExpToPointSliderExecutor();

	virtual void Update( const ExpExecutorContext& context, ExpExecutorUpdateResult& result );

    void InitializeSlide();

private:
	void CalcYaw( Float& yaw );
};
