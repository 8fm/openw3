
#pragma once

#include "storySceneEventDuration.h"
#include "../../common/engine/behaviorIncludes.h"
#include "../core/engineQsTransform.h"

//////////////////////////////////////////////////////////////////////////

struct SSSBoneTransform
{
	DECLARE_RTTI_STRUCT( SSSBoneTransform )

	CName				m_bone;
	EngineTransform		m_transform;
};

BEGIN_CLASS_RTTI( SSSBoneTransform );
	PROPERTY_EDIT( m_bone, TXT("Bone name") );
	PROPERTY_EDIT( m_transform, TXT("Transform") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SSSTrackTransform
{
	DECLARE_RTTI_STRUCT( SSSTrackTransform )

	CName				m_track;
	Float				m_value;
};

BEGIN_CLASS_RTTI( SSSTrackTransform );
	PROPERTY_EDIT( m_track, TXT("Track name") );
	PROPERTY_EDIT( m_value, TXT("Value") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventPoseKey	: public CStorySceneEventDuration
								, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventPoseKey, CStorySceneEventDuration );

	static Uint32 ID;

protected:
	Uint32						m_id;
	CName						m_actor;
	EInterpolationType			m_weightBlendType;
	Float						m_weight;
	Float						m_blendIn;
	Float						m_blendOut;
	Bool						m_linkToDialogset;
	Bool						m_useWeightCurve;
	SCurveData					m_weightCurve;

#ifndef NO_EDITOR
	CName							m_presetName;
	Int32							m_presetVersion;

	// FK
	TDynArray< SSSBoneTransform >	m_bones;

	// IK
	TDynArray< Int32 >				m_cachedBonesIK;
	TEngineQsTransformArray			m_cachedTransformsIK;

	// Hands
	TDynArray< SSSBoneTransform >	m_bonesHands;

	// Mimics
	TDynArray< SSSTrackTransform >	m_tracks;
	
#endif
	Int32							m_version;
protected:
	// Final
	TDynArray< Int32 >				m_cachedBones;
	TEngineQsTransformArray			m_cachedTransforms;
	TDynArray< Int32 >				m_cachedTracks;
	TDynArray< Float >				m_cachedTracksValues;

private:
#ifndef NO_EDITOR
	TDynArray< Float >			m_editorCachedHandTracks;
	TDynArray< Int32 >			m_editorCachedIkEffectorsID;
	TDynArray< Vector >			m_editorCachedIkEffectorsPos;
	TDynArray< Float >			m_editorCachedIkEffectorsWeight;
	TDynArray< Float >			m_editorCachedMimicSliders;
#endif

public:
	CStorySceneEventPoseKey();
	CStorySceneEventPoseKey( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );
	CStorySceneEventPoseKey( const CStorySceneEventPoseKey& other );

	virtual CStorySceneEventPoseKey* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	virtual CName GetSubject() const override { return m_actor; }

private:
	Float CalcWeight( const CStorySceneInstanceBuffer& instanceBuffer, const SStorySceneEventTimeInfo& timeInfo ) const;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() override	{ return nullptr; }
	virtual SCurveData* GetCurveData() override					{ return &m_weightCurve; }
	virtual void OnCurveChanged() override						{}
	virtual Bool UseCurveData() const override					{ return UsesWeightCurve(); }
	virtual void  OnPostLoad();
#ifndef NO_EDITOR
	friend class CStorySceneEventPoseKeyPresetData;
public:	
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;

	Bool IsLinkedToDialogset() const;
	Bool IsMimic() const;
	Bool IsBody() const;

	void CacheBones( const CStoryScenePlayer* previewPlayer );
	void CacheBones( const CEntity* e );
	void CacheBones_FK( const CAnimatedComponent* ac );
	void CacheBones_IK( const CAnimatedComponent* ac );
	void CacheBones_Hands( const CAnimatedComponent* ac );
	void CacheBones_AddTransformLS( Int32 boneIdx, const RedQsTransform& transLS );
	void CacheTracks( const CEntity* e );
	void CacheTracks( const CStoryScenePlayer* previewPlayer );

	void SetBoneTransformLS_FK( const CName& boneName, const EngineTransform& transform );
	void ResetBone_FK( const CName& boneName );
	void SetBoneTransformLS_Hands( const CName& boneName, const EngineTransform& transform );
	void ResetBone_Hands( const CName& boneName );
	void SetBonesTransformLS_IK( const TDynArray< Int32 >& bones, const TEngineQsTransformArray& transLS );

	void SetTrackValuesRaw( const CStoryScenePlayer* previewPlayer, const TDynArray< Float >& buffer );

	void SetBlends( Float blendIn, Float blendOut );
	void GetBlends( Float& blendIn, Float& blendOut );
	void FitBlends( Float prevDuration, Float newDuration );
	void FixBlends( const CStoryScenePlayer& scenePlayer );

	void InitializeWeightCurve( const CStoryScenePlayer& scenePlayer );

public:
	Uint32 GetNumOfFkBones() const { return m_bones.Size(); }
	void GetFkBonesData( const Uint32 boneNum, CName& outBoneName, EngineTransform& outTransform ) const;

	void CacheHandTrackVals( Int32 index, Float val );
	Bool LoadHandTrackVals( Int32 index, Float& val ) const;

	void CacheIkEffector( Int32 id, Vector val, Float w );
	Bool LoadIkEffector( Int32 id, Vector& val, Float& w ) const;

#endif

	Bool UsesWeightCurve() const;
};

BEGIN_CLASS_RTTI( CStorySceneEventPoseKey )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_blendIn, TXT("") );
	PROPERTY_EDIT( m_blendOut, TXT("") );
	PROPERTY_EDIT( m_weightBlendType, TXT("") );
	PROPERTY_EDIT_RANGE( m_weight, TXT( "Pose wight factor" ), 0.f, 1.f );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
	PROPERTY_EDIT( m_linkToDialogset, TXT("") );
	PROPERTY( m_version )
#ifndef NO_EDITOR
	PROPERTY_EDIT( m_bones, TXT("Bones") );
	PROPERTY_EDIT( m_bonesHands, TXT("Bones") );
	PROPERTY_RO( m_cachedBonesIK, TXT("") );
	PROPERTY_RO( m_cachedTransformsIK, TXT("") );
	PROPERTY_RO( m_presetName, TXT("") );
	PROPERTY_RO( m_presetVersion, TXT("") );
#endif
	PROPERTY_RO( m_cachedBones, TXT("") );
	PROPERTY_RO( m_cachedTransforms, TXT("") );
#ifndef NO_EDITOR
	PROPERTY( m_editorCachedHandTracks );
	PROPERTY_RO( m_editorCachedIkEffectorsID, String::EMPTY );
	PROPERTY_RO( m_editorCachedIkEffectorsPos, String::EMPTY );
	PROPERTY_RO( m_editorCachedIkEffectorsWeight, String::EMPTY );
	PROPERTY_EDIT( m_tracks, TXT("Tracks") );
#endif
	PROPERTY_RO( m_cachedTracks, TXT("") );
	PROPERTY_RO( m_cachedTracksValues, TXT("") );
#ifndef NO_EDITOR
	PROPERTY_RO( m_editorCachedMimicSliders, String::EMPTY );
#endif
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
