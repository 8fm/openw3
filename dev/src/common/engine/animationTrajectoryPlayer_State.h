
#pragma once

class CBehaviorManualSlotInterface;
class CRenderFrame;

class AnimationTrajectoryPlayer_State
{
public:
	virtual ~AnimationTrajectoryPlayer_State() {}

	virtual Bool UpdatePoint( const Matrix& l2w, const Vector& pointWS ) = 0;

	virtual Bool Update( Float& dt ) = 0;
	virtual Bool PlayAnimationOnSlot( CBehaviorManualSlotInterface& slot ) = 0;

	virtual Float GetTime() const = 0;
	virtual Bool IsBeforeSyncTime() const = 0;

	virtual void GenerateFragments( CRenderFrame* frame ) const = 0;
};
