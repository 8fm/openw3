/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "cutsceneModifier.h"
#include "../core/importer.h"
#include "skeletalAnimationSet.h"

//////////////////////////////////////////////////////////////////////////

#ifndef NO_LOG

#define CS_LOG( format, ... )		RED_LOG( Cutscene, format, ## __VA_ARGS__ )
#define CS_WARN( format, ... )		RED_LOG( Cutscene, format, ## __VA_ARGS__ )
#define CS_ERROR( format, ... )		RED_LOG( Cutscene, format, ## __VA_ARGS__ )

#else

#define CS_LOG( format, ... )	
#define CS_WARN( format, ... )	
#define CS_ERROR( format, ... )

#endif

//////////////////////////////////////////////////////////////////////////

class CCutsceneTemplate;
class ICutsceneWorldInterface;
class ICutsceneSceneInterface;

class ICutsceneProvider
{
public:
	virtual CEntity* GetCutsceneActor( const CName& voiceTag ) = 0;
	virtual CEntity* GetCutsceneActorExtContext( const String& name, CName voiceTag ) = 0;
	virtual SCutsceneActorDef* GetActorDefinition( const String& actorName ) const = 0;	
	virtual Bool	 IsActorOptional( CName id ) const { return false; };
	virtual Bool	 ForceSpawnActor( const String& actor ) { return false; }
};

class CSimpleCutsceneProvider : public ICutsceneProvider
{
	THashMap< String, CEntity* >	m_actorMap;
	TDynArray<String>				m_forceSpawn;
	
public:
	CSimpleCutsceneProvider( const THashMap< String, CEntity* >& map ) : m_actorMap( map ) {}

	virtual Bool ForceSpawnActor( const String& actor ) 
	{ 
		return m_forceSpawn.Exist( actor ); 
	}

	virtual CEntity* GetCutsceneActor( const CName& voiceTag )
	{
		return nullptr;
	}

	virtual CEntity* GetCutsceneActorExtContext( const String& name, CName voiceTag )
	{
		CEntity* entity = NULL;
		m_actorMap.Find( name, entity );
		return entity;
	}

	virtual SCutsceneActorDef* GetActorDefinition( const String& actorName ) const
	{
		return NULL;
	}

	void AddForceSpawnActor( const String& actor )
	{
		m_forceSpawn.PushBack( actor );
	}
};

struct CutsceneImporterParams : public IImporter::ImportOptions::ImportParams
{
	Int32 m_maxPartFrames;

	CutsceneImporterParams() : m_maxPartFrames( -1 ) {}
};

class CCutsceneTemplate : public CSkeletalAnimationSet
{
	DECLARE_ENGINE_RESOURCE_CLASS( CCutsceneTemplate, CSkeletalAnimationSet, "w2cutscene", "Cutscene template" );	

public:

	static const CName CUTSCENE_ANIMATION_NAME;

	struct FactoryInfo : public CResource::FactoryInfo< CCutsceneTemplate >
	{				
		struct ActorImportData
		{
			String	m_animation;
			String	m_component;
		};

		TDynArray< ActorImportData >	m_importData;
		CutsceneImporterParams*			m_params;
	};

	CCutsceneTemplate();
	virtual ~CCutsceneTemplate();

#ifndef NO_RESOURCE_IMPORT
	// Create new cutscene template
	static CCutsceneTemplate* Create( const FactoryInfo& data );
#endif

	// Create new cutscene instance
 	CCutsceneInstance* CreateInstance( CLayer* layer, const Matrix& point, String& errors, ICutsceneProvider* provider, ICutsceneWorldInterface* worldInterface, Bool doInit = true, ICutsceneSceneInterface* sceneInterface = nullptr );

	// Create new cutscene instance
	CCutsceneInstance* CreateInstance( CLayer* layer, const Matrix& point, String& errors, const THashMap< String, CEntity*>& actors, Bool doInit = true, const Char* forceSpawnActor = nullptr );

	// Unregister cuscene instance
	void UnregisterInstance( CCutsceneInstance* instance );

	static ECutsceneActorType ExtractActorTypeFromTemplate( const CEntityTemplate* templ );

	// Handle missing property
	virtual void OnSerialize( IFile& file );

#ifndef NO_EDITOR
	void FillSkeletonsInAnimationsBasingOnTemplates();
#endif

public:
	virtual void OnPostLoad();
	virtual void OnSave();

#ifndef NO_RESOURCE_COOKING
	virtual void OnCook( class ICookerFramework& cooker ) override;
#endif

	Bool IsValid() const;
	Bool Validate( String& errors, IFeedbackSystem* fsys = NULL );
	void SetValid( Bool flag );

	Matrix GetActorPositionInTime( const String& actorName, Float time, const Matrix* offset = nullptr, Matrix* pelvisPos = nullptr ) const;
	Matrix CalcActorCurrentPelvisPositionInTime( const String& actorName, Float time, const Matrix* offset = NULL ) const;
	Bool SampleActorPoseInTime( const String& actorName, Float time, SBehaviorGraphOutput& pose ) const;
	void GetActorsInitialPositions( TDynArray< Matrix >& pos, const Matrix* offset = NULL, Bool onlyActors = false, TDynArray< String >* names = NULL ) const;
	void GetActorsFinalPositions( TDynArray< Matrix >& pos, const Matrix* offset = NULL, Bool onlyActors = false, TDynArray< String >* names = NULL ) const;
	void GetActorsPositionsAtTime( Float time, TDynArray< Matrix >& pos, const Matrix* offset = NULL, Bool onlyActors = false, TDynArray< String >* names = NULL ) const;
#ifdef USE_HAVOK_ANIMATION
	Bool GetCameraInitialPose( TDynArray< hkQsTransform >& pose );
	Bool GetCameraFinalPose( TDynArray< hkQsTransform >& pose );
	Bool GetCameraPoseAtTime( Float time, TDynArray< hkQsTransform >& pose );
#else
	Bool GetCameraInitialPose( TDynArray< RedQsTransform >& pose );
	Bool GetCameraFinalPose( TDynArray< RedQsTransform >& pose );
	Bool GetCameraPoseAtTime( Float time, TDynArray< RedQsTransform >& pose );
#endif
	Float				GetDuration() const;
	const TagList&		GetPointTag() const;
	const String		GetLastLevelLoaded() const;
	void				SetLastLevelLoaded( String );

	void				GetActorsName( TDynArray< String >& names ) const;
	void				GetTypedActorNames( TDynArray< String >& names, ECutsceneActorType actorType ) const;
	CName				GetAnimationName( Uint32 num ) const;
	void				GetActorsVoicetags( TDynArray< CName >& voiceTags );
	Bool				HasActor( CName actorId, const TagList* tags = nullptr ) const;

	SCutsceneActorDef*			GetActorDefinition( const String& actorName );
	const SCutsceneActorDef*	GetActorDefinition( const String& actorName ) const;
	const SCutsceneActorDef*	GetActorDefinition( const CName& voicetag )	const;
	void						UnloadActorDefinitiones();

	void	GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates );

	Bool HasCameraSwitches() const;
	Bool HasBoundingBoxes() const;
	void GetBoundingBoxes( TDynArray< Box >& boxes ) const;

	Bool HasFadeBefore() const;
	Bool HasFadeAfter() const;
	Float GetFadeBeforeDuration() const;
	Float GetFadeAfterDuration() const;
	Bool IsBlackScreenOnLoading() const { return false; } //m_blackscreenWhenLoading; }

	Int32 GetCameraActorNum() const;
	// outCameraComponent isn't the final component that will actually be used when playing the cutscene. But it has the
	// same setup as the camera's component.
	Bool GetCameraAnimation( const CSkeletalAnimation*& outAnimation, const CAnimatedComponent*& outCameraComponent, Int32& outBoneIndex ) const;

	Bool IsStreamable() const;

	Float GetCameraBlendInDuration() const { return m_cameraBlendInTime; }
	Float GetCameraBlendOutDuration() const { return m_cameraBlendOutTime; }

	static String	GetActorName( const CName& animation );
	static String	GetComponentName( const CName& animation );
	static Bool	IsRootComponent( const CName& animation );
	static Bool	IsMimicComponent( const CName& animation );

	Bool	CheckActorsPosition() const { return m_checkActorsPosition; }
	Bool	GetTagPointMatrix( const TagList& tag, Matrix& mat ) const;

	// Get positioning of cutscene in current world.
	Bool	GetCsPointMatrix( Matrix& mat ) const { return GetTagPointMatrix( m_point, mat ); }

	RED_INLINE const String& GetReverbName() const { return m_reverbName; }
	RED_INLINE const StringAnsi& GetBurnedAudioTrackName() const { return m_burnedAudioTrackName; }
	RED_INLINE const TDynArray< CName >& GetBanksDependency() const { return m_banksDependency; }

public:
	Bool SaveFileWithThisCutscene( const String& fileName );
	Bool ReleaseFileWithThisCutscene( const String& fileName );
	Bool IsFileUsedThisCutscene( const String& fileName ) const;
	const TDynArray< String >& GetFilesWithThisCutscene() const;

public:
	// IEventsContainer interface
	virtual void AddEvent( CExtAnimEvent* event );
	virtual void RemoveEvent( CExtAnimEvent* event );
	virtual void GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events );
	virtual const CResource* GetParentResource() const;

	void GetEventsByTime( Float prevTime, Float currTime, TDynArray< CAnimationEventFired >& events );
	template < class EventType >
	void GetEventsOfType( TDynArray< EventType* >& list ) const;
	
public:
	CFXDefinition* AddEffect( const String& effectName );
	Bool AddEffect( CFXDefinition* effect );
	Bool RemoveEffect( CFXDefinition* effect );
	CFXDefinition* FindEffect( const CName& effectName ) const;
	Bool HasEffect( const CName& effectName ) const;
	RED_INLINE const TDynArray< CFXDefinition* >& GetEffects() const { return m_effects; }
	TDynArray<Float> GetCameraCuts() const;
	Bool HasCameraCut( Float prevTime, Float currTime ) const;

	const TDynArray< CName >& GetEntToHideTags() const { return m_entToHideTags;}

protected:
	SCutsceneActorDef CreateActorDef( const String& actorName, const TDynArray< SCutsceneActorDef >& prevDef ) const;	
protected:
	TDynArray< CFXDefinition* >		m_effects;				// Effects defined at this cutscene template

	TDynArray< ICutsceneModifier* > m_modifiers;

	TDynArray< SCutsceneActorDef >	m_actorsDef;

	TDynArray< CName >				m_entToHideTags;

	TagList							m_point;
	String							m_lastLevelLoaded;

	Float							m_fadeBefore;
	Float							m_fadeAfter;
	Bool							m_blackscreenWhenLoading;

	Float							m_cameraBlendInTime;
	Float							m_cameraBlendOutTime;

	Bool							m_isValid;
	Bool							m_streamable;

	Bool							m_checkActorsPosition;

	TDynArray< String >				m_usedInFiles;

	TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter > m_animEvents;

	// Sound related
	String							m_reverbName;
	StringAnsi						m_burnedAudioTrackName;
	TDynArray< CName >				m_banksDependency;

	// Resources to preload tralalala
#ifndef NO_EDITOR
	TDynArray< THandle< CResource > >			m_resourcesToPreloadManually;
#endif

	TDynArray< String >				m_resourcesToPreloadManuallyPaths;

	//!< Reference count. Incremented with each CreateInstance. Decremented, when instance is deleted.
	Red::Threads::CAtomic< Int32 >	m_instanceReferenceCount;		
};

BEGIN_CLASS_RTTI( CCutsceneTemplate );
	PARENT_CLASS( CSkeletalAnimationSet );
	PROPERTY_INLINED( m_modifiers, TXT("Modifires") );
	PROPERTY_EDIT( m_point, TXT("Cutscene root point") );
	PROPERTY_EDIT( m_lastLevelLoaded, TXT("Last Level this cutscene was loaded in.") );
	PROPERTY_RO( m_actorsDef, TXT("Cutscene actors def") );
	PROPERTY( m_isValid );
	PROPERTY_EDIT( m_fadeBefore, TXT("How long should fade end after beginning") );
	PROPERTY_EDIT( m_fadeAfter, TXT("How long should fade start before end") );
	PROPERTY_EDIT( m_cameraBlendInTime, TXT("Camera blend in duration") );
	PROPERTY_EDIT( m_cameraBlendOutTime, TXT("Camera blend out duration") );
	PROPERTY_EDIT( m_blackscreenWhenLoading, TXT("") );
	PROPERTY_EDIT( m_checkActorsPosition, TXT("") );
	PROPERTY_EDIT( m_entToHideTags, TXT("") )
	PROPERTY( m_usedInFiles );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_resourcesToPreloadManually, TXT( "Resources to preload" ) );
#endif
	PROPERTY( m_resourcesToPreloadManuallyPaths );
	PROPERTY_CUSTOM_EDIT( m_reverbName, TXT( "Name of reverb definition" ), TXT( "SoundReverbEditor" ) );
	PROPERTY_CUSTOM_EDIT( m_burnedAudioTrackName, TXT( "Audio track burned, cutscene will be ticked from track position" ), TXT( "AudioEventBrowser" ) );
	PROPERTY_CUSTOM_EDIT_ARRAY( m_banksDependency, TXT( "" ), TXT( "SoundBankEditor" ) )

	//PROPERTY( m_animEvents );
	PROPERTY_RO( m_streamable, TXT("") );
	PROPERTY_INLINED( m_effects, TXT("Effects") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

template < class EventType >
void CCutsceneTemplate::GetEventsOfType( TDynArray< EventType* >& list ) const
{
	for( TDynArray< CExtAnimEvent* >::const_iterator eventIter = m_animEvents.Begin();
		eventIter != m_animEvents.End(); ++eventIter )
	{
		CExtAnimEvent* event = *eventIter;

		if( IsType< EventType >( event ) )
		{
			list.PushBack( static_cast< EventType* >( event ) );
		}
	}
}
