/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventAnimClip.h"
#include "storySceneEventInterfaces.h"

class CStorySceneEventMimicsAnim	: public CStorySceneEventAnimClip
									#ifndef NO_EDITOR
									, public IDialogMimicsAnimationFilterInterface
									#endif
									, public ISSDragAndDropMimicsAnimInterface
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventMimicsAnim, CStorySceneEventAnimClip )

	RED_ADD_INTERFACE_SUPPORT_1( ISSDragAndDropMimicsAnimInterface )

private:
	CName		m_animationName;
	Bool		m_fullEyesWeight;

#ifndef NO_EDITOR
protected:
	CName		m_filterOption;
	String		m_friendlyName;
#endif

public:
	CStorySceneEventMimicsAnim();
	CStorySceneEventMimicsAnim( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventMimicsAnim* Clone() const override;

	// DIALOG_TOMSIN_TODO - wyszyscic bazel z public i  protected
protected:
	virtual const CName& GetAnimationSlotName() const override	{ return CNAME( CUSTOM_GEST_SLOT ); }
	virtual const CAnimatedComponent* GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const override;
	virtual Bool IsBodyOrMimicMode() const { return false; } // Body = true, Mimics = false

	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::MimicsAnimation& event ) const override;

public: // ISSDragAndDropMimicsAnimInterface
	virtual void Interface_SetDragAndDropMimicsAnimation( const TDynArray< CName >& animData, Float animDuration );

public:
	RED_INLINE void SetAnimationName( const CName& animName ) { m_animationName = animName; }
	virtual const CName& GetAnimationName() const override		{ return m_animationName; }

#ifndef NO_EDITOR
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif

#ifndef NO_EDITOR
public:
	virtual const CName& GetMimicsFilterActor() const						{ return m_actor; }
	virtual const CName& GetMimicsActionFilter() const						{ return m_filterOption;}

	const void SetMimicksActionFilter( CName name )							{ m_filterOption = name; }
	const String& GetAnimationFriendlyName() const							{ return m_friendlyName; }
	const void SetAnimationFriendlyName( const String& name )				{  m_friendlyName = name; }
	
	void SetAnimationState( const TDynArray< CName >& data );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventMimicsAnim )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT( "Mimics animation name" ), TXT( "DialogMimicAnimationSelection" ) )
	PROPERTY_EDIT( m_fullEyesWeight, TXT("") );
#ifndef NO_EDITOR
	PROPERTY_CUSTOM_EDIT( m_filterOption, TXT( "Filter name" ), TXT( "DialogMimicsAnimation_ActionType" ) );
	PROPERTY_CUSTOM_EDIT( m_friendlyName, TXT( "Animation friendly name" ), TXT( "DialogMimicsAnimation_FriendlyName" ) );
#endif
END_CLASS_RTTI()
