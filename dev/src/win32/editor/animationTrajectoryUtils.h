
#pragma once

class CSkeletalAnimationTrajectoryParam;
struct AnimationTrajectoryData;

class AnimationTrajectoryBuilder
{
public:
	static Bool CreateTrajectoryFromUncompressedAnimation( const CSkeletalAnimationSetEntry* uncompressedAnimation, const CSkeleton* skeleton, Int32 boneIndex, Int32 syncFrame, Int32 boneToExtract, AnimationTrajectoryData& out );
	static Bool CreateTrajectoryFromCompressedAnimation( const CSkeletalAnimationSetEntry* compressedAnimation, const CSkeleton* skeleton, Int32 boneIndex, Float syncFrame, Int32 boneToExtract, AnimationTrajectoryData& out );
};

class AnimationTrajectoryVisualizer
{
public:
	static void DrawTrajectoryLS( CRenderFrame *frame, const AnimationTrajectoryData& data );
	static void DrawTrajectoryMS( CRenderFrame *frame, const AnimationTrajectoryData& data );
	static void DrawTrajectoryMSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld );
	static void DrawTrajectoryLSinWS( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld );
	static void DrawTrajectoryMSinWSWithPtr( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration );
	static void DrawTrajectoryMSinWSWithPtrO( CRenderFrame *frame, const AnimationTrajectoryData& data, const Matrix& localToWorld, Float time, Float duration );

private:
	static void InternalDrawTrajectoryWSWithPtr( CRenderFrame *frame, const TDynArray< Vector >& LS, const TDynArray< Vector >& MS, Uint32 syncFrame, const Matrix& localToWorld, Float time, Float duration, const Color& color );
};
