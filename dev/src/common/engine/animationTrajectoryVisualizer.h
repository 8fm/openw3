
#pragma once

struct AnimationTrajectoryData;
class CRenderFrame;

class AnimationTrajectoryVisualizer
{
public:
	static void DrawTrajectoryLS( CRenderFrame *frame, const AnimationTrajectoryData& data );
	static void DrawTrajectoryMS( CRenderFrame *frame, const AnimationTrajectoryData& data );
	static void DrawTrajectoryMSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld );
	static void DrawTrajectoryLSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld );
	static void DrawTrajectoryMSinWSWithPtr( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration );
	static void DrawTrajectoryMSinWSWithPtrO( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration );

	static void InternalDrawTrajectoryWSWithPtr( CRenderFrame *frame, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS, Uint32 syncFrame, const Matrix& localToWorld, Float time, Float duration, const Color& color );
};

