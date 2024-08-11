/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "timeline.h"
#include "dialogEditorCameraCtrl.h"
#include "dialogTimelineEventsLinker.h"
#include "..\..\common\game\storySceneEvent.h"
#include "../../common/game/storySceneSectionVariant.h"

class CEdPropertiesPage;
class CEdSceneEditorScreenplayPanel;
class CEdSceneEditor;
struct SStorySceneActorAnimationState;
class IStorySceneElementInstanceData;

//////////////////////////////////////////////////////////////////////////

// forward declarations
namespace DialogTimelineItems
{
	class CTimelineItemBlocking;
	class CTimelineItemEvent;
	class CTimelineItemBlend;
	class CTimelineItemCameraBlend;
	class CTimelineItemCameraBlendEvent;
	class CTimelineItemInterpolationEvent;
	class CTimelineItemDebugCommentEvent;
}

class CEdDialogTimeline : public CEdTimeline
{
	wxDECLARE_EVENT_TABLE();

protected:
	const static Int32    					TIMELINE_PADLOCK_BTN_SIZE;

	CEdSceneEditor*							m_mediator;

	Bool									m_debugItems;
	Bool									m_showVoiceMarkers;

	const CStorySceneSection*									m_section;
	TDynArray< DialogTimelineItems::CTimelineItemBlocking* >	m_elements;
	TDynArray< TPair< Int32, SEtorySceneEventGenericCreationData* > >			m_genericObjsData;

protected:
	wxToolBar*								m_timelineToolbar;
	Int32									m_timelinePlayTool;
	wxBitmap								m_timelinePlayIcon;
	wxBitmap								m_timelinePauseIcon;

	Gdiplus::Bitmap*						m_lockedBitmap;
	Gdiplus::Bitmap*						m_unlockedBitmap;

	DialogTimelineEventsLinker				m_eventsLinker;

public:
	const static String TIMELINE_DEFAULT_CAMERA_TRACK;
	const static String PROP_TRACK;
	const static String EFFECT_TRACK;	
	const static String LIGHT_TRACK;	
	const static String	CUTSCENE_LINES_TRACK;
	const static String	CAMERA_LIGHTS_TRACK;
	

	CEdDialogTimeline( wxPanel* parentPanel, CEdSceneEditor* parentScriptEditor, CEdPropertiesPage* propertiesPage = NULL );
	virtual ~CEdDialogTimeline();

	void SetSection( const CStorySceneSection* section );
	const CStorySceneSection* GetSection() const;
	CStorySceneSectionVariantId GetSectionVariantId() const;
	void RefreshSection();

	void GetMarkedLockedEvents( TDynArray< CStorySceneEvent*>& marked, TDynArray< CStorySceneEvent*>& locked );

	CWorld* GetPreviewWorld() const;
	wxToolBar* GetToolbar()					{ return m_timelineToolbar; }

	virtual void PaintCanvas( Int32 width, Int32 height );

	const CAnimatedComponent* GetBodyComponent( const CName& actor ) const;
	const CAnimatedComponent* GetHeadComponent( const CName& actor ) const;

	void RemoveItems( const TDynArray< ITimelineItem* > items );

	Bool IsTimelineEditingEnabled() const;

public:
	void RefreshPlayPauseIcon( Bool flag );
	void SetShowVoiceMarkers( Bool showVoiceMarkers );
	Bool GetShowVoiceMarkers();
	void SelectOnlyOneItem( const CStorySceneEvent* e );
	void SetSelection( TDynArray< ITimelineItem* >& items, Bool goToEvent = true ) override;
	void CancelSelection();	
	void GetSelectedEvents( TDynArray<CStorySceneEvent*>& events );
	Bool IsTrackLocked( const String& trackName ) const;
	void GetLockedTracks( TDynArray<String>& tracks ) const;
	CName GetSelectedEntityName() const;

	void ToggleDebugItems();
	void ToggleCameraTool( Bool previewMode, Bool freeMode, Bool editMode );

	CStorySceneEvent* CreateEvent( const CClass* c, Float absTime, const CName& actorId, const void* extraData = nullptr );

	Bool ConvertOldCameraBlendsToInterpolationEvents();
	DialogTimelineItems::CTimelineItemInterpolationEvent* ConvertOldCameraBlendToInterpolationEvent( DialogTimelineItems::CTimelineItemCameraBlend* cameraBlendV1 );
	DialogTimelineItems::CTimelineItemInterpolationEvent* ConvertOldCameraBlendToInterpolationEvent( DialogTimelineItems::CTimelineItemCameraBlendEvent* cameraBlendV2 );

public: // CDropTarget
	virtual wxDragResult OnDragOver( wxCoord x, wxCoord y, wxDragResult def );
	virtual Bool OnDropText( wxCoord x, wxCoord y, String &text );

public: // From dialog editor - TO REFACTOR!
	EScenePreviewCameraMode GetCameraMode();

	const CStoryScenePlayer* GetPlayer() const;

	DialogTimelineItems::CTimelineItemEvent* DetachFromList( DialogTimelineItems::CTimelineItemEvent* camInst );
	void CreateCustomCameraInstance( CName cameraName );
	DialogTimelineItems::CTimelineItemEvent* CreateCustomCameraEvent( const StorySceneCameraDefinition & definition, DialogTimelineItems::CTimelineItemBlend* blend = NULL, Float absTime = -1.f );
	DialogTimelineItems::CTimelineItemEvent* CreateOverridePlacementEvent( const String& trackName, const CName& actorName, const EngineTransform* transform = NULL, DialogTimelineItems::CTimelineItemBlend* blend = NULL );

	void DrawLink( const wxRect& start, const wxRect& end, const wxColor& c, Float progress );

	void GoToPrevCameraEvent();
	void GoToNextCameraEvent();
	void GoToPrevFrame();
	void GoToNextFrame();

public: // DIALOG_TOMSIN_TODO - tego uzywaja eventy
	void OnBlockingItemDurationChange( DialogTimelineItems::CTimelineItemBlocking* item, Float durationChange );
	void OnPauseItemDurationChange( DialogTimelineItems::CTimelineItemBlocking* item, Float durationChange );
	void OnItemInternalChanged( DialogTimelineItems::CTimelineItemBlocking* item );
	void GetDisableDialogLookatEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEvents, Bool mimic ) const;
	void GetKeyPoseMarkersEventsPositions( CName actorName, CName animName, TDynArray< Float >& dialogAnimEvents, Bool mimic ) const;
	void GetKeyPoseDurationsEventsPositions( CName actorName, CName animName, TDynArray< TPair< Float, Float > >& dialogAnimEventsTrans, TDynArray< TPair< Float, Float > >& dialogAnimEventsPoses, Bool mimic ) const;
	Bool CalculateEventItemRect( const ITimelineItem* item, wxRect& rect );
	Bool GetVoiceDataPositions( CStorySceneLine* lineElement, TDynArray< TPair< Float, Float > >* voiceMaxAmpRelPos, Float* voiceStartRelPos  = NULL, Float* voiceEndRelPos = NULL );
	const DialogTimelineItems::CTimelineItemEvent* FindItemEvent( const CGUID& eventGUID ) const;
	DialogTimelineItems::CTimelineItemEvent* FindItemEvent( const CGUID& eventGUID );
	const DialogTimelineItems::CTimelineItemEvent* FindItemEvent( const CStorySceneEvent* e ) const;
	DialogTimelineItems::CTimelineItemEvent* FindItemEvent( const CStorySceneEvent* e );
	CStorySceneEvent* FindEvent( const CGUID& eventGUID );
	DialogTimelineItems::CTimelineItemBlend* FindDurationBlendItemByEventClass( CClass* eventClass, Float position );
	virtual void OnTrackSelectionChanged();
	Bool UseLocColors() const;
	wxColor FindLocColor( Uint32 stringId ) const;
	const IStorySceneElementInstanceData* FindElementInstance( const CStorySceneElement* element ) const;
	
	Float GetAnimationDurationFromEvent( const CStorySceneEventAnimClip& animClip ) const;
	void RequestRebuild();

	// operations of event instances
	Float GetEventInstanceStartTime( const CStorySceneEvent& e ) const;
	void SetEventInstanceStartTime( const CStorySceneEvent& e, Float startTime ) const;
	Float GetEventInstanceDuration( const CStorySceneEvent& e ) const;
	void SetEventInstanceDuration( const CStorySceneEvent& e, Float duration ) const;
	Float GetEventInstanceScalingFactor( const CStorySceneEvent& e ) const;

	void LoadTracksStateFromConfig( const String& sceneDepotPath );
	void SaveTracksStateToConfig( const String& parentCofigPath );

protected:

	DialogTimelineItems::CTimelineItemEvent* CreateCustomCameraEvent( const Vector& cameraPosition, 
		const EulerAngles& cameraRotation, Float cameraZoom, Float cameraFov,
		Float dofFocusDistFar, Float dofBlurDistFar,
		Float dofIntensity, Float dofFocusDistNear, Float dofBlurDistNear,
		const ApertureDofParams& apertureDofParams,
		DialogTimelineItems::CTimelineItemBlend* blend = NULL, Float absTime = -1.f );

protected:
	Float FillWithSectionContents( const CStorySceneSection* section );
	virtual void ShowItemMenu();
	Bool IsCameraEventItem( const ITimelineItem* item );

protected:
	virtual Bool CanChangeDefaultTrack() const { return false; }
	virtual Bool CanResizeOverEnd() const { return true; }
	virtual Bool CanUseSelectionItemMenu() const  { return true; }

	virtual void SerializeItem( ITimelineItem* item, IFile& file );
	virtual ITimelineItem* DeserializeItem( IFile& file );
	CStorySceneEvent* DeserializeEvent( IFile& file, Float& absStartTime );

	virtual void ItemPropertyChanged( TDynArray< ITimelineItem* >& items, const CName& propertyName ) override;
	virtual void SelectionChanged();

	virtual Bool IsCanvasMenuEnabled() const override;
	virtual Bool IsTrackMenuEnabled() const override;
	virtual Bool IsTrackButtonMenuEnabled() const override;
	virtual Bool IsItemMenuEnabled() const override;

	virtual void FillTrackMenu( const String& name, wxMenu* menu );
	virtual void FillItemMenu( ITimelineItem* item, wxMenu* menu, Bool& addDefaults );

	virtual void EditProperties( ITimelineItem* item, CEdPropertiesPage& propertiesPage ) const;
	virtual void RecreateTracks();

	virtual void SortTracks();
	virtual DialogTrack* CreateTrack( String name, Uint32 depth, bool isGroup ) const;

	virtual void OnStateSelected( wxPoint globalPos );
	virtual void OnTrackDoubleClicked();
	virtual void OnLeftMouseDoubleClick( Int32 x, Int32 y );

	virtual Float			Snap( Float value );
	virtual Float			ResizeItem( ITimelineItem* item, Float duration );
	virtual const wxCursor	GetResizeCursor() const;

	virtual void OnDrawItem( const ITimelineItem* item, const wxRect& rect );

	virtual String GetActorTrackName( const CName& actorId );

protected:
	CStorySceneEvent* CreateEvent( Int32 eventId, const String& trackName, Float positionRel, CName actor, DialogTimelineItems::CTimelineItemBlocking* selected, const void* extraData = NULL );
	DialogTimelineItems::CTimelineItemEvent* CreateTimelineItemEvent( CStorySceneEvent* event, DialogTimelineItems::CTimelineItemBlocking* elementItem );
	DialogTimelineItems::CTimelineItemBlocking* CreateBlockingTimelineItem ( CStorySceneElement* element, Float startTime, Uint32 level );
	void CreateSelectedAnimationPelvisPlacement();

	void GatherAllEventItems( TDynArray< DialogTimelineItems::CTimelineItemEvent* >& eventItems ) const;
	Uint32 GetElementEvents( const DialogTimelineItems::CTimelineItemBlocking* element, TDynArray< DialogTimelineItems::CTimelineItemEvent* >& events ) const;

	void PerformClipRoll( Float offset );

	virtual void OnStateChanged( States prevState, States newState ) override;
	void OnCheckAutoLink();

	CName GetTrackActor( const String& trackName ) const;
	DialogTimelineItems::CTimelineItemBlocking* CEdDialogTimeline::FindSceneElement( Float time ) const;

	Bool ShowMenuWithAnimationBodyExtraData( const CName& actorName, TDynArray< CName >& state, bool unfiltered ) const;
	void ShowMenuWithAnimationMimicsExtraData( const CName& actorName, TDynArray< CName >& state ) const;
	Bool ShowMenuForDialogsetBodyData( const CName& actorName, SStorySceneActorAnimationState* state = NULL ) const;
	Bool ShowMenuForAnimationData( CStorySceneEventAnimation* animationEvt, Bool selectAllFilters, Bool unfiltered ) const;
	Bool ShowMenuForAnimationData( CStorySceneEventMimicsAnim* animationEvt, Bool selectAllFilters ) const;
	Bool ShowMenuForAnimationData( CStorySceneEventChangePose* animationEvt, Bool selectAllFilters ) const;
	Bool ShowMenuForActorsBodyFilterData( CName& inOutStatus, CName& inOutEmoState, CName& inOutPose, CName* inOutType = NULL, Bool selectAllFilters = false ) const;
	Bool ShowMenuForActorsMimicsFilterData( CName& inOutEmoState, Bool selectAllFilters = false ) const;

protected:
	// UI events
	void OnNewEvent( wxCommandEvent& event );
	void OnSplitAnimationEvent( wxCommandEvent& event );
	void OnAddToCustomCameraList( wxCommandEvent& event );
	void OnDetachFromCustomCameraList( wxCommandEvent& event );
	void OnGroupEvents( wxCommandEvent& event );
	void OnUngroupEvents( wxCommandEvent& event );
	void OnLinkEvent( wxCommandEvent& event );
	void OnUnlinkEvent( wxCommandEvent& event );
	void OnMuteEvent( wxCommandEvent& event );
	void OnCreatePause( wxCommandEvent& event );
	void OnAlignStart( wxCommandEvent& event );
	void OnAlignStartToEnd( wxCommandEvent& event );
	void OnAlignEnd( wxCommandEvent& event );
	void OnAlignToCamera( wxCommandEvent& event );
	void OnAlignEndToSectionsEnd( wxCommandEvent& event );
	void OnSetClippingFront( wxCommandEvent& event );
	void OnSetClippingBack( wxCommandEvent& event );
	void OnCutAnimClip( wxCommandEvent& event );
	void OnNormalEventCheckedChanged( wxCommandEvent& event );
	void OnMarkEventCheckedChanged( wxCommandEvent& event );
	void OnLockEventCheckedChanged( wxCommandEvent& event );
	void OnSetAsBackgroundLine( wxCommandEvent& event );
	void OnSetAsNonBackgroundLine( wxCommandEvent& event );
	void OnGoToMarker( wxCommandEvent& event );
	void OnCopyLightFromCamera( wxCommandEvent& event );
	void OnPasteLightToCamera( wxCommandEvent& event );
	void OnToggleEntityHelperIsInteractive( wxCommandEvent& event );

	void OnAddPlacementOverrideBlend( wxCommandEvent& event );

	void OnCreateEnhancedCameraBlend( wxCommandEvent& event );
	void OnAddToCameraEnhancedBlend( wxCommandEvent& event );
	void OnRemoveFromCameraEnhancedBlend( wxCommandEvent& event );

	void OnRemoveCameraBlend( wxCommandEvent& event ); // old style camera blends

	void OnRemoveFromCameraBlendNew( wxCommandEvent& event );
	void OnCameraBlendNewPlot( wxCommandEvent& event );

	void OnInterpolationEventCreate( wxCommandEvent& event );
	void OnInterpolationEventDestroy( wxCommandEvent& event );
	void OnInterpolationEventAttachKey( wxCommandEvent& event );
	void OnInterpolationEventDetachKey( wxCommandEvent& event );
	void OnInterpolationEventPlot( wxCommandEvent& event );

	void OnDebugCommentEventCreate( wxCommandEvent& event );
	void OnSetCurrentTime( wxCommandEvent& event );

	void OnCopyTransformToDialogsetSlotFromPlacementEvent( wxCommandEvent& event );
	void OnCopyLightGoToCenterSS( wxCommandEvent& event );
	void OnToggleTimePause( wxCommandEvent& event );
	void OnLimitMinSet( wxCommandEvent& event );
	void OnLimitMaxSet( wxCommandEvent& event );
	void OnLimitMinReset( wxCommandEvent& event );
	void OnLimitMaxReset( wxCommandEvent& event );
	void OnArrowLeft( wxCommandEvent& event );
	void OnArrowRight( wxCommandEvent& event );
	void OnArrowUp( wxCommandEvent& event );
	void OnArrowDown( wxCommandEvent& event );
	void OnCreatePlacementForAnimPelvis( wxCommandEvent& event );

	// Input events
	void OnMouseWheel( wxMouseEvent& event );
	virtual void OnItemSelected( ITimelineItem* item ) override {}

public:
	virtual void OnItemPropertyChanged( wxCommandEvent& event );

private:
	virtual void OnDeleteItem( wxCommandEvent& event );

	DialogTimelineItems::CTimelineItemInterpolationEvent* InterpolationEventCreate( const TDynArray< ITimelineItem* >& keys );
	void InterpolationEventAttachKey( DialogTimelineItems::CTimelineItemInterpolationEvent* tiInterpolationEvent, DialogTimelineItems::CTimelineItemEvent* tiKeyEvent );
	void InterpolationEventDetachKey( DialogTimelineItems::CTimelineItemInterpolationEvent* tiInterpolationEvent, DialogTimelineItems::CTimelineItemEvent* tiKeyEvent );

	virtual void RemoveItemImpl( ITimelineItem* item ) override;

	void GetInterpolationEventsInvalidatedByDeletion( const TDynArray< DialogTimelineItems::CTimelineItemEvent* >& events, TDynArray< DialogTimelineItems::CTimelineItemEvent* >& outInterpolationEvents );
	void GetBlendEventsInvalidatedByDeletion( const TDynArray< DialogTimelineItems::CTimelineItemEvent* >& events, TDynArray< DialogTimelineItems::CTimelineItemEvent* >& outBlendEvents );

	DialogTimelineItems::CTimelineItemDebugCommentEvent* DebugCommentEventCreate( DialogTimelineItems::CTimelineItemBlocking* tiElement, Float startPos, const String& trackName );
	void AddGenericEventsEntries( TDynArray< TPair< Int32, SEtorySceneEventGenericCreationData* > >& genericObjs, wxMenu* menu, const String& category );
	Int32 GetIdOfGenericObject( const CClass* cls ) const;

	virtual void StoreLayout() override;
	virtual void RestoreLayout() override;

	virtual void HandleRepositionLeftEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers ) override;
	virtual void HandleRepositionRightEdge( ITimelineItem* item, Float newTimePos, TimelineKeyModifiers keyModifiers ) override;
};

RED_INLINE const CStorySceneSection* CEdDialogTimeline::GetSection() const
{
	return m_section;
}
