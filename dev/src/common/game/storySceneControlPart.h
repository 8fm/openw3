#pragma once

#include "storySceneLinkElement.h"

class CStoryScene;
class CStorySceneInput;

struct SBrokenSceneGraphInfo
{
	String									m_path;
	TDynArray< TPair< String, String > >	m_brokenLinks;
};

/// Basic control flow element of the story scene graph, subclassed to be either a section or a conditional flow control
class CStorySceneControlPart : public CStorySceneLinkElement
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CStorySceneControlPart, CStorySceneLinkElement );

private:
	String	m_comment;

public:
	//! Get the story scene
	virtual CStoryScene* GetScene() const;

	virtual String GetName() const { return GetFriendlyName(); }

	virtual String GetComment() const { return m_comment; }

	//! Get all connected control parts ( recursive )
	void CollectControlParts( TDynArray< CStorySceneControlPart* >& controlParts );
	void CollectControlParts( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts );

	void GetAllNextLinkedElements( TDynArray< const CStorySceneLinkElement*>& elements ) const;

	virtual void GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const {}

	//! Validate links to this element
	void ValidateLinks( SBrokenSceneGraphInfo* graphInfo = NULL );

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts );
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneControlPart )
	PARENT_CLASS( CStorySceneLinkElement )
	PROPERTY_EDIT( m_comment, TXT( "Comment for this scene part" ) );
END_CLASS_RTTI()
