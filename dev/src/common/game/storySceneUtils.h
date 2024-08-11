#pragma once

namespace StorySceneUtils 
{
	void ConvertStateToAnimationFilters( const CName& state, CName& status, CName& emoState, CName& pose );

	Matrix CalcWSFromSS( const EngineTransform& transformSS, const EngineTransform& sceneLocalToWorld );
	Vector CalcWSFromSS( const Vector& transformSS, const EngineTransform& sceneLocalToWorld );
	EngineTransform CalcSSFromWS( const EngineTransform& transformWS, const EngineTransform& sceneLocalToWorld );

	Vector CalcEyesPosLS( CActor* actor, const CName& idleAnim );

	Matrix CalcL2WForAttachedObject( const CAnimatedComponent* ac, CName boneName, Uint32 attachmentFlags );

	Bool CalcTrackedPosition( const SStorySceneLightTrackingInfo& info, const CStoryScenePlayer* player, Vector& outPos, EulerAngles& outRot );

	Color GetEnvLightVal( CLayer* layer, Int32 index );

	Bool DoTraceZTest( const CActor* actor, const Vector& inPos, Vector& outPos );

#ifndef NO_EDITOR
	void ShouldFadeOnLoading( const CStoryScene* scene, TDynArray< CName >& names, TDynArray< Bool >& values );
#endif
}
