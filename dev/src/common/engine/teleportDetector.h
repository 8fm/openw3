/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CTeleportDetector
{
	DECLARE_CLASS_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_Engine, __alignof( CTeleportDetector ) );

public:
	enum EEnabledMode
	{
		EM_SceneMode = FLAG( 1 ),
		EM_CutsceneMode = FLAG( 2 )
	};

private:
	TDynArray< AnimQsTransform >		m_previousBonesTransLS;
	TDynArray< Int32 >					m_bonesIndices;
	TDynArray< AnimVector4 >			m_bonesDirectionsLS;

	//extra case for pelvis translation without l2w change
	Int32								m_pelvisIndex;
	AnimVector4							m_pelvisPreviousDirectionsLS;
	AnimQsTransform						m_pelvisPreviousTransLS;
	Vector								m_pelvisPreviousPosWS;
	Vector								m_pelvisPreviousPosMS;
	Float								m_pelvisTranslationThresholdSqr;
	Bool								m_pelvisChangedMS;
	Bool								m_pelvisChangedWS;
	//end of extra

	Float								m_angleDetectionThreshold;
	Uint32								m_enabledFlags;
	Uint32								m_requestedFlagsToReset;
	Bool								m_poseChanged;
	Bool								m_forceUpdate;					// f.ex. from gameplay side

#ifndef NO_EDITOR
	TDynArray< Bool >					m_bonesChangedFlag;
	TDynArray< Float >					m_bonesChangedAngle;
	Uint64								m_lastUpdateTick;
#endif

public:
	CTeleportDetector( const CSkeleton* sk, const Matrix& l2w );

	void				CheckTeleport( const AnimQsTransform* bonesLS, const Uint32 numBones, const TDynArray< Matrix >& bonesMS, const Matrix& currentL2w );
	RED_INLINE Bool		IsEnabled() const { return m_enabledFlags > 0; }
	void				SetEnabled( Uint32 enabledModeFlag );
	void				RequestSetDisabled( Uint32 enabledModeFlag );
	RED_INLINE Bool		DoesPoseChanged() const { return m_poseChanged; }
	RED_INLINE Bool		DoesPelvisChangedMS() const { return m_pelvisChangedMS; }
	RED_INLINE Bool		DoesPelvisChangedWS() const { return m_pelvisChangedWS; }
	RED_INLINE Bool		SetForceUpdateOneFrame() { return m_forceUpdate = true; }

#ifndef NO_EDITOR_FRAGMENTS
	void OnGenerateEditorFragments( CRenderFrame* frame, const TDynArray< Matrix >& poseWS ) const;
#endif

#ifndef NO_EDITOR
	RED_INLINE Uint64 GetLastTick() const { return m_lastUpdateTick; }
#endif

private: //helpers
	void ResetEnabled( Uint32 enabledModeFlag );
	void CachePelvisData( const CSkeleton* sk, const Matrix& l2w, const CTeleportDetectorData* data );
	void DetectPelvisTeleport( const AnimQsTransform* bonesLS, const Uint32 numBones, const TDynArray< Matrix >& bonesMS, const Matrix& currentL2w, const Float angleThreshold );
};
