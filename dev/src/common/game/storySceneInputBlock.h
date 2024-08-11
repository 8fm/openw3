#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneInputStructureListener.h"

class CStorySceneGraphSocket;
class CStorySceneControlPart;

/// Block in the scene graph that represents the section of the scene
class CStorySceneInputBlock : public CStorySceneGraphBlock, public IStorySceneInputStructureListener
{
	DECLARE_ENGINE_CLASS( CStorySceneInputBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneInput*		m_input;			//!< Linked input

public:
	//! Get the section represented by this block
	RED_INLINE CStorySceneInput* GetInput() const { return m_input; }
	void SetInput( CStorySceneInput *input );

	//! Get the control part represented by this block
	virtual CStorySceneControlPart* GetControlPart() const { return m_input;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_input = Cast< CStorySceneInput >( part ); }

public:
	CStorySceneInputBlock() : m_input( NULL ) {}

	//! Loaded from disk
	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get the name of the block
	virtual String GetBlockName() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	virtual void OnDestroyed();

#endif

protected:
	// Callbacks
	virtual void OnNameChanged( CStorySceneSection* sender );
	virtual void OnLinksChanged( CStorySceneInput* sender ) {}
};

BEGIN_CLASS_RTTI( CStorySceneInputBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY( m_input );
END_CLASS_RTTI();
