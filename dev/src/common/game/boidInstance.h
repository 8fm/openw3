#pragma once

#include "swarmUtils.h"
#include "boidNodeData.h"
#include "../engine/playedAnimation.h"

class CAnimNodeData;
class CBoidState;
class CBoidInstance 
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_Boids );

protected:
	Boids::BoidState				m_boidState;
	IBoidLairEntity*				m_lair;
	THandle< CSwarmRenderComponent > m_swarmRenderingComponent;
	Vector							m_position;
	EulerAngles						m_orientation;
	Float							m_scale;
	CName							m_animation;

	Float							m_animSpeedMultiplier;

	CEntity*						m_fxHolder;
	TDynArray< CFXState* >			m_activeEffects;

	/// Holds all information regarding the animation 
	const CAtomicBoidNode	*		m_currentAtomicBoidNode;
	const CAtomicBoidNode	*		m_pendingAtomicBoidNode;
	const CBoidState *				m_pendingBoidState;
	Uint32							m_currentStateCounter;

public:
	CBoidInstance( IBoidLairEntity* lair, const Vector& position, const EulerAngles& orientation, Float scale = 1.f );
	virtual ~CBoidInstance();

	void						OnTick(CLoopedAnimPriorityQueue &loopedAnimPriorityQueue, Float time, const Vector& position, const EulerAngles& orientation );
	void						SetBoidState( Boids::BoidState state, CLoopedAnimPriorityQueue &loopAnimPriorityQueue, Float time );

	Boids::BoidState			GetBoidState() const								{ return m_boidState; }
	const Vector&				GetPosition() const									{ return m_position; }
	const EulerAngles&			GetOrientation() const								{ return m_orientation; }
	const Float&				GetScale() const									{ return m_scale; }
	const Uint32				GetCurrentStateCounter() const						{ return m_currentStateCounter; }
	const CName					GetAnimation() const								{ return m_animation; }

	void						ActivateNextAnimNode(CLoopedAnimPriorityQueue &loopAnimPriorityQueue, Float time);
	Float						PlayAnimation(CName animName, Bool looped, Float allowedVariationOnSpeed, Bool startRandom);
	void						PlayEffect(CName effectName);
	void						StopEffect(CName effectName);	
};