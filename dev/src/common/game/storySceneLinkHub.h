
#pragma once

#include "storySceneControlPart.h"

class CStorySceneLinkHub : public CStorySceneControlPart
{
	DECLARE_ENGINE_CLASS( CStorySceneLinkHub, CStorySceneControlPart, 0 )

private:
	Uint32 m_numSockets;

public:
	CStorySceneLinkHub();

public:
	Uint32 GetNumSockets() { return m_numSockets; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
#endif

#ifndef NO_EDITOR
	virtual Bool SupportsInputSelection() const override;
	virtual void ToggleSelectedInputLinkElement() override;
#endif

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};

BEGIN_CLASS_RTTI( CStorySceneLinkHub );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY_EDIT_RANGE( m_numSockets, TXT("Num of input sockets"), 0, 15 )
END_CLASS_RTTI();
