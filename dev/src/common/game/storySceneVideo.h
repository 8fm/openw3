/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "storySceneControlPart.h"
#include "storySceneGraphBlock.h"
#include "storySceneSection.h"
#include "storySceneElement.h"

class CStorySceneVideoSection : public CStorySceneSection
{
	DECLARE_ENGINE_CLASS( CStorySceneVideoSection, CStorySceneSection, 0 )
protected:
	String	m_videoFileName;
	String	m_eventDescription;
	Bool	m_suppressRendering;
	TDynArray< String > m_extraVideoFileNames;

public:
	CStorySceneVideoSection();

	const String& GetVideoName() const { return m_videoFileName; }
	const String& GetEventDescription() const { return m_eventDescription; }
	Bool ShouldSuppressRendering() const { return m_suppressRendering; }
	const TDynArray< String >& GetExtraVideoFileNames() { return m_extraVideoFileNames; }

	virtual void OnSectionCreate();

	//virtual Bool HasFadeOut() const						{ return true; }
	//
	virtual Bool HasFadeIn() const						{ return true; }

	virtual Bool UsesSetting() const override			{ return false; }

	virtual Bool ShouldForceDialogset() const override	{ return false; }

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};

BEGIN_CLASS_RTTI( CStorySceneVideoSection )
	PARENT_CLASS( CStorySceneSection )
	PROPERTY_EDIT( m_videoFileName, TXT( "Video File" ) );
	PROPERTY_EDIT( m_eventDescription, TXT( "Event description" ) );
	PROPERTY_EDIT( m_suppressRendering, TXT( "Suppress rendering" ) );
	PROPERTY_EDIT( m_extraVideoFileNames, TXT( "Extra video files" ) );
END_CLASS_RTTI()



class CStorySceneVideoBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneVideoBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneVideoSection*	m_sceneVideo;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get the name of the block
	virtual String GetBlockName() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const { return Color( 240, 220, 70 ); }

#endif

public:
	virtual CStorySceneControlPart* GetControlPart() const { return m_sceneVideo;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_sceneVideo = Cast< CStorySceneVideoSection >( part ); }
};

BEGIN_CLASS_RTTI( CStorySceneVideoBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY( m_sceneVideo );
END_CLASS_RTTI();


class CStorySceneVideoElement : public CStorySceneElement
{
	DECLARE_ENGINE_CLASS( CStorySceneVideoElement, CStorySceneElement, 0 )

private:
	String						m_description;

public:
	//! Start playing of this dialog element
	virtual IStorySceneElementInstanceData* OnStart( CStoryScenePlayer* player ) const;

	//! Get list of element that can be scheduled via this element
	virtual void OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const;

	virtual Bool IsPlayable() const { return true; }

	virtual Bool CanBeDeleted() const { return false; }

	virtual Bool CanBeSkipped() const { return true; }

	RED_INLINE void SetDescriptionText( String newValue ) { m_description = newValue; }
	RED_INLINE String GetDescriptionText() const			{ return m_description; }
};

BEGIN_CLASS_RTTI( CStorySceneVideoElement )
	PARENT_CLASS( CStorySceneElement )
	PROPERTY_EDIT( m_description, TXT( "Video description" ) );
END_CLASS_RTTI()

class CStorySceneVideoElementInstance : public IStorySceneElementInstanceData
{
public:
	CStorySceneVideoElementInstance( const CStorySceneVideoElement* element, CStoryScenePlayer* player )
		: IStorySceneElementInstanceData( element, player )
		, m_hasVideoStarted( false )
		, m_hasSuppressedRender( false )
	{
	}

	virtual Bool IsBlocking() const { return true; }

	virtual Bool ShouldConfirmSkip() const { return true; }

	virtual String GetName() const { return String( TXT("Video") ); }

protected:
	virtual void OnPlay() override;
	virtual Bool OnTick( Float timeDelta ) override;
	virtual void OnStop() override;

	Bool m_hasVideoStarted;

	// HACK : OnEnded apparently can come multiple times, which causes CRenderCommand_SuppressSceneRendering to go multiple times, which
	// throws off everything. So we track whether we've suppressed it.
	Bool m_hasSuppressedRender;

private:
	void OnStarted();
	void OnEnded();
	void CheckSubtitles();
};
