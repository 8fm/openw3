#pragma once

#include "storySceneControlPart.h"

class CStorySceneGraphSocket;
class CStorySceneControlPart;

/// Base class for a graph block for scene graph
class CStorySceneGraphBlock : public CGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CStorySceneGraphBlock, CGraphBlock )

protected:
	Bool	m_isActive;

public:
	//! Get control part ( section, control flow ) that is represented by this block
	virtual CStorySceneControlPart* GetControlPart() const = 0;
	virtual void SetControlPart( CStorySceneControlPart* part ) = 0;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TBaseClass::GetCaption().Empty() ? TXT(" ") : TBaseClass::GetCaption(); }
	virtual String GetComment() const { return ( GetControlPart() != NULL ) ? GetControlPart()->GetComment() : String::EMPTY; }
	virtual void OnDestroyed();
#endif

	virtual void SetActive( Bool activate ) { m_isActive = activate; }
	virtual Bool IsActivated() const { return m_isActive; }
	virtual Float GetActivationAlpha() const { return 1.0f; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CStorySceneGraphBlock );
	PARENT_CLASS( CGraphBlock );
END_CLASS_RTTI();