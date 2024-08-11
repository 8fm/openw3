#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneRandomizer.h"

class CStorySceneRandomBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneRandomBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneRandomizer *		m_controlPart;

public:
	//! Get flow condition represented by this block
	RED_INLINE CStorySceneRandomizer* GetRandomPart() const { return m_controlPart; }
	void SetRandomPart( CStorySceneRandomizer* randomPart );
	
	//! Get scene control part ( section, control flow ) that is represented by this block
	virtual CStorySceneControlPart* GetControlPart() const { return m_controlPart;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_controlPart = Cast< CStorySceneRandomizer >( part ); }

	virtual String GetCaption() const { return TXT("Randomizer"); }

public:
	CStorySceneRandomBlock() : m_controlPart(NULL) {}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Recreate layout of the block
	virtual void OnRebuildSockets();
	virtual void OnDestroyed();

#endif
};

BEGIN_CLASS_RTTI( CStorySceneRandomBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY_INLINED_RO( m_controlPart, TXT( "Property of a random part structure" ) );
END_CLASS_RTTI();