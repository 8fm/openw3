#pragma once

#include "storySceneCameraSetting.h"
#include "storySceneInstanceBuffer.h"

/// This is a basic class representing linkable elements of the scene system
/// This element remembers the links in the both directions, we have all the other
/// link elements that links to us via the m_linkedElements member, and the direct
/// link from us to the other link element in the m_nextLinkElement.

class CStorySceneLinkElement : public CObject
{
	DECLARE_ENGINE_CLASS( CStorySceneLinkElement, CObject, 0 )

protected:
	TDynArray< CStorySceneLinkElement* >		m_linkedElements;		//!< Elements linking to this part
	CStorySceneLinkElement*						m_nextLinkElement;		//!< Link to the next element

#ifndef NO_EDITOR
	Uint32										m_selectedLinkedElement;
#endif

public:
	//! Get all links
	RED_INLINE const TDynArray< CStorySceneLinkElement* >& GetLinkedElements() const { return m_linkedElements; }

	// SCENE_TOMSIN_TODO - should be const
	//! Get link to next element
	RED_INLINE CStorySceneLinkElement* GetNextElement() const { return m_nextLinkElement; }

public:
	virtual void OnPropertyPostChange( IProperty* property );

public:
	//! Callback called when some other scene object is linked to this element
	virtual void OnConnected( CStorySceneLinkElement* linkedToElement );

	//! Callback called when other scene object is unlinked from this element
	virtual void OnDisconnected( CStorySceneLinkElement* linkedToElement );

public:
	CStorySceneLinkElement();

	//! Serialization
	virtual void OnSerialize( IFile& file );

	//! Connect scene part to this link
	void ConnectToElement( CStorySceneLinkElement* linkableElement );

	//! Remove all links in this
	void ResetLinks();

	//! Reset next element
	void ResetNextElement();

protected:
	//! Add a link to another scene element
	void LinkElement( CStorySceneLinkElement* linkElement );

	//! Remove a link
	void UnlinkElement( CStorySceneLinkElement* linkElement );

public:
	// SCENE_TOMSIN_TODO - remove this
	//! Can modify camera
	virtual Bool CanModifyCamera() const { return false; }

	// SCENE_TOMSIN_TODO - remove this
	//! Get camera settings for this element
	virtual Bool GetCameraSettings( StorySceneCameraSetting& cameraSettings ) const { return false; }

protected:
	//! Call it before any element's change! Returns true if you can modify item.
	Bool MarkModified_Pre();

	//! Call it after any element's change!
	void MarkModified_Post();

#ifndef NO_EDITOR
public:
	virtual Bool SupportsInputSelection() const;
	virtual Bool SupportsOutputSelection() const;
	virtual void ToggleSelectedInputLinkElement();
	virtual void ToggleSelectedOutputLinkElement();
	virtual Uint32 GetSelectedInputLinkElement() const;
	virtual Uint32 GetSelectedOutputLinkElement() const;
#endif
};

BEGIN_CLASS_RTTI( CStorySceneLinkElement )
	PARENT_CLASS( CObject );
	PROPERTY( m_linkedElements );
	PROPERTY( m_nextLinkElement );
END_CLASS_RTTI()
