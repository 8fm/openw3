#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneSection.h"

class CStorySceneGraphSocket;
class CStorySceneControlPart;

/// Block in the scene graph that represents the section of the scene
class CStorySceneSectionBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneSectionBlock, CStorySceneGraphBlock, 0 )

protected:
	CStorySceneSection*		m_section;			//!< Linked section

public:
	//! Get the section represented by this block
	RED_INLINE CStorySceneSection* GetSection() const { return m_section; }

	//! Get the control part represented by this block
	virtual CStorySceneControlPart* GetControlPart() const { return m_section;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_section = Cast< CStorySceneSection >( part ); }

public:
	CStorySceneSectionBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get the name of the block
	virtual String GetBlockName() const;

	virtual Color GetClientColor() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

#endif

	//! Loaded from disk
	virtual void OnPostLoad();

	//! Bind to section data
	void SetSection( CStorySceneSection* newValue );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Callbacks
	virtual void OnChoiceLineChanged( CStorySceneChoiceLine* choiceLine );
#endif

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual void OnPasted( Bool wasCopied );
	virtual void OnDestroyed();
#endif

protected:
	//! Called when we need to rename sockets related to choice elements
	void RenameChoiceSockets();
};

BEGIN_CLASS_RTTI( CStorySceneSectionBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY( m_section );
END_CLASS_RTTI();