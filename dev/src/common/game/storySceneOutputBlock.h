#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneOutput.h"

class CStorySceneOutputBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneOutputBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneOutput* m_output;

public:
	CStorySceneOutputBlock() : m_output( NULL ) {}

	void SetOutput( CStorySceneOutput* output ) { m_output = output; }
	CStorySceneOutput* GetOutput() const { return m_output; }

	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get the name of the block
	virtual String GetBlockName() const;

	//! Rebuild block layout
	virtual void OnRebuildSockets();

	//! Get title bar color
	virtual Color GetTitleColor() const;

	//! Get client color
	virtual Color GetClientColor() const;

	virtual void OnDestroyed();

#endif

	virtual CStorySceneControlPart* GetControlPart() const { return m_output;  }
	virtual void SetControlPart( CStorySceneControlPart* part ) { m_output = Cast< CStorySceneOutput >( part ); }
};

BEGIN_CLASS_RTTI( CStorySceneOutputBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY_INLINED( m_output, TXT( "Output") )
END_CLASS_RTTI();
