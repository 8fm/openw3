/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneDialogsetCameraDescriptions.h"
#include "storySceneCameraDefinition.h"
#include "storySceneIncludes.h"
#include "../engine/skeletalAnimationSet.h"

class CStorySceneDialogsetInstance;
class CStorySceneAction;
class CStorySceneAnimationContainer;

class CStorySceneDialogsetSlot	: public CObject
								, public IDialogBodyAnimationFilterInterface
								, public IDialogMimicsAnimationFilterInterface
{
	DECLARE_ENGINE_CLASS( CStorySceneDialogsetSlot, CObject, 0 );

protected:
	Uint32							m_slotNumber;
	CName							m_slotName;
	EngineTransform					m_slotPlacement;
	CName							m_actorName;
	Bool							m_actorVisibility;
	TDynArray<CStorySceneAction*>	m_setupAction;

	CName							m_actorStatus;
	CName							m_actorEmotionalState;
	CName							m_actorPoseName;

	CName							m_actorMimicsEmotionalState;
	CName							m_actorMimicsLayer_Eyes;
	CName							m_actorMimicsLayer_Pose;
	CName							m_actorMimicsLayer_Animation;
	CName							m_forceBodyIdleAnimation;
	Float							m_forceBodyIdleAnimationWeight;
	Float							m_actorMimicsLayer_Pose_Weight;

	CGUID							m_ID;

protected:
	// TO REMOVE
	CName							m_actorState;

public:
	CStorySceneDialogsetSlot();

	virtual String GetFriendlyName() const { return String::Printf( TXT( "%d, %s, %s" ), m_slotNumber, m_slotName.AsString().AsChar(), m_slotPlacement.ToString().AsChar() ); }

	RED_INLINE Uint32								GetSlotNumber() const		{ return m_slotNumber; }
	RED_INLINE const CName&							GetSlotName() const			{ return m_slotName; }
	RED_INLINE const CName&							GetForceBodyIdleAnimation() const { return m_forceBodyIdleAnimation; }
	RED_INLINE Float								GetForceBodyIdleAnimationWeight() const { return m_forceBodyIdleAnimationWeight; }
	RED_INLINE const EngineTransform&				GetSlotPlacement() const	{ return m_slotPlacement; }
	RED_INLINE const CName&							GetActorName() const		{ return m_actorName; }
	RED_INLINE Bool									GetActorVisibility() const	{ return m_actorVisibility; }
	RED_INLINE const TDynArray<CStorySceneAction*>&	GetSetupActions() const		{ return m_setupAction; }
	RED_INLINE const CGUID&							GetID() const				{ return m_ID; }

	void GetMimicsLayers( CName& eyes, CName& pose, CName& animation ) const;

	const Float GetMimicsPoseWeight() const { return m_actorMimicsLayer_Pose_Weight; }

public: // IDialogBodyAnimationFilterInterface
	virtual const CName& GetBodyFilterStatus() const override			{ return m_actorStatus; }
	virtual const CName& GetBodyFilterEmotionalState() const override	{ return m_actorEmotionalState; }
	virtual const CName& GetBodyFilterPoseName() const override			{ return m_actorPoseName; }
	virtual const CName& GetBodyFilterTypeName() const override			{ return CName::NONE; }
	virtual const CName& GetBodyFilterActor() const	override			{ return m_actorName; }

public: //IDialogMimicsAnimationFilterInterf
	virtual const CName& GetMimicsFilterEmotionalState() const override	{ return m_actorMimicsEmotionalState; }
	virtual const CName& GetMimicsFilterActor() const override			{ return m_actorName; }

#ifndef NO_EDITOR
	RED_INLINE EngineTransform& GetSlotPlacementRef()				{ return m_slotPlacement; }

	RED_INLINE void SetSlotNumber( Uint32 number )				{ m_slotNumber = number; }
	RED_INLINE void SetSlotName( const CName& name )				{ m_slotName = name; }
	RED_INLINE void SetActorName( const CName& name )				{ m_actorName = name; }
	RED_INLINE void SetSlotPlacement( const EngineTransform& placement ) { m_slotPlacement = placement; }
	RED_INLINE void SetActorVisibility( const Bool val )			{ m_actorVisibility = val; }

	RED_INLINE void SetBodyFilterStatus( const CName& name ) 			{ m_actorStatus = name; }
	RED_INLINE void SetBodyFilterEmotionalState( const CName& name )	{ m_actorEmotionalState = name; }
	RED_INLINE void SetBodyFilterPoseName( const CName& name ) 		{ m_actorPoseName = name; }

	RED_INLINE void SetMimicsFilterEmotionalState( const CName& nam ) { m_actorMimicsEmotionalState = nam; }

	RED_INLINE void SetSetupActions( const TDynArray<CStorySceneAction*>&	setupActions ) { m_setupAction = setupActions; }
	RED_INLINE void SetNoSetupActions()								{ m_setupAction.Clear(); }
	RED_INLINE void AddSetupAction( CStorySceneAction * setupAction ) { m_setupAction.PushBack(setupAction); }

	virtual void OnPropertyPostChange( IProperty* prop );

	String	GetDescriptionString() const;

	void OnCloned();
	virtual void OnCreatedInEditor() override;
#endif

	virtual void OnPostLoad();

	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const;

private:
	void CacheMimicsLayerFromEmoState( Bool force );
};

BEGIN_CLASS_RTTI( CStorySceneDialogsetSlot );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_slotNumber, TXT( "Slot number" ) );
	PROPERTY_EDIT( m_slotName, TXT( "Slot name" ) );
	PROPERTY_EDIT( m_slotPlacement, TXT( "Slot relative placement" ) );	
	PROPERTY_CUSTOM_EDIT( m_actorName, TXT( "Name of actor occupying this slot" ), TXT( "DialogVoiceTag" ) );
	PROPERTY_EDIT( m_actorVisibility, TXT( "Default visibility of actor in this slot") );
	PROPERTY_CUSTOM_EDIT( m_actorStatus, TXT( "Status" ), TXT( "DialogBodyAnimation_Status" ) );
	PROPERTY_CUSTOM_EDIT( m_actorEmotionalState, TXT( "Emotional state" ), TXT( "DialogBodyAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_actorPoseName, TXT( "Pose name" ), TXT( "DialogBodyAnimation_Pose" ) );
	PROPERTY_CUSTOM_EDIT( m_actorMimicsEmotionalState, TXT( "Mimics emotional state name" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_actorMimicsLayer_Eyes, TXT( "Mimics layer eyes" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_actorMimicsLayer_Pose, TXT( "Mimics layer pose" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_actorMimicsLayer_Animation, TXT( "Mimics layer animation" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_EDIT( m_actorMimicsLayer_Pose_Weight, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_forceBodyIdleAnimation, TXT("Force body idle animation"), TXT("DialogAnimationSelection") );
	PROPERTY_EDIT( m_forceBodyIdleAnimationWeight, TXT("Force body idle animation weight") );	
	PROPERTY_RO( m_actorState, TXT( "Default emotional state for actor" ) );
	PROPERTY_RO( m_ID, TXT("ID") );
	PROPERTY_INLINED( m_setupAction, TXT( "Actions to perfrom by actor when switching to dialogset" ) )
END_CLASS_RTTI();

enum ESceneDialogsetCameraSet
{
	SDSC_None,
	SDSC_Personal,
	SDSC_Master,
	SDSC_All,
};

BEGIN_ENUM_RTTI( ESceneDialogsetCameraSet );
	ENUM_OPTION( SDSC_None );
	ENUM_OPTION( SDSC_Personal );
	ENUM_OPTION( SDSC_Master );
	ENUM_OPTION( SDSC_All );
END_ENUM_RTTI();

class CStorySceneDialogset : public CSkeletalAnimationSet
{
	DECLARE_ENGINE_RESOURCE_CLASS( CStorySceneDialogset, CResource, "w2dset", "Scene dialogset" );

private:
	CName	m_dialogsetName;
	CName	m_dialogsetTransitionEvent;
	Bool	m_isDynamic;

	TDynArray< EngineTransform >	m_characterTrajectories;				// Deprecated
	TDynArray< EngineTransform >	m_cameraTrajectories;					// Deprecated
	TDynArray< Vector >				m_cameraEyePositions;					// Deprecated

	TDynArray< SScenePersonalCameraDescription >	m_personalCameras;		// Deprecated
	TDynArray< SSceneMasterCameraDescription >		m_masterCameras;		// Deprecated
	TDynArray< SSceneCustomCameraDescription >		m_customCameras;		// Deprecated

	// New
	TDynArray< CStorySceneDialogsetSlot* >	m_slots;
	TDynArray< StorySceneCameraDefinition >	m_cameras;

public:
	CStorySceneDialogset();

	Bool	IsCameraNumberValid( Int32 cameraNumber ) const;
	Bool	IsSlotNumberValid( Int32 slotNumber ) const;

	void	GetCameraEyePositionsForSet( ESceneDialogsetCameraSet cameraSet, TDynArray< Vector >& eyePositions ) const;

public:
	RED_INLINE const TDynArray< EngineTransform >&	GetCharacterTrajectories() const		{ return m_characterTrajectories; }
	RED_INLINE TDynArray< EngineTransform >&	GetCameraTrajectories()					{ return m_cameraTrajectories; }
	RED_INLINE TDynArray< Vector >&			GetCameraEyePositions()					{ return m_cameraEyePositions; }
	RED_INLINE TDynArray< SScenePersonalCameraDescription >&	GetPersonalCameras()	{ return m_personalCameras; }
	RED_INLINE TDynArray< SSceneMasterCameraDescription >&	GetMasterCameras()		{ return m_masterCameras; }
	RED_INLINE TDynArray< SSceneCustomCameraDescription >&	GetCustomCameras()		{ return m_customCameras; }

	RED_INLINE Bool			IsDynamicDialogset() const	{ return m_isDynamic; }
	RED_INLINE const CName&	GetName() const				{ return m_dialogsetName; }

	RED_INLINE Uint32	GetNumberOfCameraTrajectories() const		{ return m_cameraTrajectories.Size(); }
	//RED_INLINE Uint32	GetNumberOfCharactersTrajectories() const	{ return m_characterTrajectories.Size(); }

	RED_INLINE const TDynArray< CStorySceneDialogsetSlot* >&	GetSlots() const { return m_slots; }
	RED_INLINE Uint32	GetNumberOfSlots() const { return m_slots.Size(); }
	void	GetSlotPlacements( TDynArray< EngineTransform >& placements ) const;


	RED_INLINE Uint32	GetNumberOfCameras() const { return m_cameras.Size(); }
	RED_INLINE const TDynArray< StorySceneCameraDefinition >&	GetCameraDefinitions() const { return m_cameras; }
	RED_INLINE TDynArray< StorySceneCameraDefinition >&	GetCameraDefinitions() { return m_cameras; }
	
	RED_INLINE void AddSlot( CStorySceneDialogsetSlot* slot ) { slot->SetParent( this ); m_slots.PushBack( slot ); }
	RED_INLINE void AddCamera( const StorySceneCameraDefinition& camera ) { m_cameras.PushBack( camera ); }
	RED_INLINE void SetName( const CName& name ) { m_dialogsetName = name; }

public:
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void OnSerialize( IFile& file );

public:
	void CalculateCameraEyePositions();

public:
	static String GetDefaultDialogsetPath();

public:
	// Methods for importing old dialogsets
	Bool ImportCharacterTrajectories( CSkeletalAnimation* trajectoryAnimation, Uint32 maxTrajectories = 24 );
	Bool ImportCameraTrajectories( CSkeletalAnimation* trajectoryAnimation, Uint32 maxTrajectories = 24 );
	void ImportFromOldDialogset( const CName& dialogsetName, Uint32 charactersInDialogset, Uint32 camerasInDialogset );
	static String GetDialogsetPathByName( const CName& dialogsetName );
	static String GetDialogsetPathById( const String& dialogsetId );

#ifndef NO_EDITOR
	void OnCloned();
	void ConvertSlots();
	void ConvertCameraDefinitions();
#endif
};

BEGIN_CLASS_RTTI( CStorySceneDialogset )
	PARENT_CLASS( CSkeletalAnimationSet )
	PROPERTY_EDIT( m_dialogsetName, TXT( "Dialogset name" ) );
	PROPERTY_EDIT( m_dialogsetTransitionEvent, TXT( "Name of event sent to actors when applying this dialogset" ) );
	PROPERTY_EDIT( m_isDynamic, TXT( "Allow dynamic setting of trajectories" ) );
	PROPERTY_INLINED( m_characterTrajectories, TXT( "Character slots positions" ) );
	PROPERTY_INLINED( m_cameraTrajectories, TXT( "Camera slots positions" ) );
	//PROPERTY( m_characterTrajectories );
	//PROPERTY( m_cameraTrajectories );
	PROPERTY( m_personalCameras );
	PROPERTY( m_masterCameras );
	PROPERTY( m_customCameras );
	PROPERTY( m_cameraEyePositions );

	PROPERTY_INLINED( m_slots, TXT( "Dialogset slots" ) );
	PROPERTY_INLINED( m_cameras, TXT( "Dialogset cameras" ) );
END_CLASS_RTTI();

RED_DECLARE_NAMED_NAME( CloseUp, "Close Up" );
RED_DECLARE_NAMED_NAME( MediumCloseUp, "Medium Close Up" );
RED_DECLARE_NAMED_NAME( ExtremeCloseUp, "Extreme Close Up" );

//////////////////////////////////////////////////////////////////////////

class CStorySceneDialogsetInstance : public CObject
{
	DECLARE_ENGINE_CLASS( CStorySceneDialogsetInstance, CObject, 0 )
	
protected:
	// Basic data
	CName									m_name;
	TDynArray< CStorySceneDialogsetSlot* >	m_slots;

	// Placement data
	TagList										m_placementTag;
	Float										m_safePlacementRadius;
	Bool										m_snapToTerrain;
	Bool										m_findSafePlacement;
	Bool										m_areCamerasUsedForBoundsCalculation;

	// Camera blends
	String										m_path;

public:
	CStorySceneDialogsetInstance()
		: m_snapToTerrain( false )
		, m_findSafePlacement( false )
		, m_safePlacementRadius( GetDefaultSafePlacementRadius() )
		, m_areCamerasUsedForBoundsCalculation( false )
	{}

	RED_INLINE const CName&										GetName() const { return m_name; }
	RED_INLINE const TagList&									GetPlacementTag() const { return m_placementTag; }
	RED_INLINE Bool												GetFindSafePlacement() const { return m_findSafePlacement; }
	RED_INLINE Float											GetSafePlacementRadius() const { return m_safePlacementRadius; }
	RED_INLINE Bool												AreCamerasUsedForBoundsCalculation() const { return m_areCamerasUsedForBoundsCalculation; }
	static RED_INLINE Float										GetDefaultSafePlacementRadius() { return 1.2f; }
	RED_INLINE const TDynArray< CStorySceneDialogsetSlot* >&	GetSlots() const { return m_slots; }
	RED_INLINE Bool												IsSnappingToTerrainAllowed() const { return m_snapToTerrain; }

	RED_INLINE void	SetName( const CName& name ) { m_name = name; }
	RED_INLINE void	SetPath( const String& path ) { m_path = path; }
	RED_INLINE void	SetPlacementTags( const TagList& placementTags ) { m_placementTag = placementTags; }
	RED_INLINE const String &	GetPath() { return m_path; }  const
	RED_INLINE void	AddSlot( CStorySceneDialogsetSlot* slot ) { slot->SetParent( this ); m_slots.PushBack( slot ); }
	RED_INLINE void	RemoveSlot( const CName& slotName ) { m_slots.Remove( GetSlotByName( slotName ) ); }
	

public:
	CStorySceneDialogsetSlot* GetSlotByActorName( const CName& actorName ) const;
	CStorySceneDialogsetSlot* GetSlotBySlotNumber( Uint32 slotNumber ) const;
	CStorySceneDialogsetSlot* GetSlotByName( const CName& slotName ) const;

	void GetAllActorNames( TDynArray< CName >& names ) const;
	void CollectRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const;
	void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const;

	static CStorySceneDialogsetInstance* CreateFromResource( CStorySceneDialogset* dialogsetResource, CObject* parent );

#ifndef NO_EDITOR
	void OnCloned();
#endif
};

BEGIN_CLASS_RTTI( CStorySceneDialogsetInstance );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_name, TXT( "Name of dialogset instance" ) );
	//PROPERTY_INLINED( m_slots, TXT( "Slot descriptions" ) );
	PROPERTY( m_slots );
	PROPERTY_EDIT( m_placementTag, TXT( "Tagged entity to get placement from" ) );
	PROPERTY_EDIT( m_snapToTerrain, TXT( "Allow slots to be snapped to terrain" ) );
	PROPERTY_EDIT( m_findSafePlacement, TXT( "Enables searching for safe placement" ) );
	PROPERTY_EDIT( m_safePlacementRadius, TXT( "Radius used to build convex that is used to find safe placement" ) );
	PROPERTY_EDIT( m_areCamerasUsedForBoundsCalculation, TXT( "Cameras positions will be included to scene denied are shape" ) );
	PROPERTY_RO( m_path, TXT( "File this dialogset has beed imported from" ) );
END_CLASS_RTTI();