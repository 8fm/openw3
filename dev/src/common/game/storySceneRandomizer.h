#pragma once

#include "storySceneControlPart.h"

class CStorySceneRandomizer : public CStorySceneControlPart
{
	DECLARE_ENGINE_CLASS( CStorySceneRandomizer, CStorySceneControlPart, 0 )

private:
	TDynArray< CStorySceneLinkElement* >	m_outputs;

public:
	CStorySceneRandomizer();

	RED_INLINE const TDynArray< CStorySceneLinkElement* > & GetOutputs() const { return m_outputs; }

	CStorySceneLinkElement* GetRandomOutput( Int32& outputindex ) const;

public:
	virtual void OnPostLoad() { ValidateLinks(); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
#endif

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};

BEGIN_CLASS_RTTI( CStorySceneRandomizer );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY_EDIT( m_outputs, TXT("Random outputs") );
END_CLASS_RTTI();
