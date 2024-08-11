#pragma once

#include "storySceneControlPart.h"

class IQuestCondition;

class CStorySceneFlowCondition : public CStorySceneControlPart
{
	DECLARE_ENGINE_CLASS( CStorySceneFlowCondition, CStorySceneControlPart, 0 )

private:
	CStorySceneLinkElement*		m_trueLink;			//!< Path to go when condition evaluates to true
	CStorySceneLinkElement*		m_falseLink;		//!< Path to go when condition evaluates to false
	IQuestCondition*			m_questCondition;
#ifndef NO_EDITOR
	Bool						m_trueFalseEditor;
#endif

public:
	//! Get the next control part when condition evaluates to true
	RED_INLINE CStorySceneLinkElement* GetTrueLink() const { return m_trueLink; }

	//! Get the next control part when condition evaluates to false
	RED_INLINE CStorySceneLinkElement* GetFalseLink() const { return m_falseLink; }

	Bool IsFulfilled() const;

	RED_INLINE const IQuestCondition*		GetCondition() const	{ return m_questCondition; }

public:
	CStorySceneFlowCondition();
	void InitializeWithDefaultCondition();

	virtual void OnConnected( CStorySceneLinkElement* linkedToElement );
	virtual void OnDisconnected( CStorySceneLinkElement* linkedToElement );

	virtual void OnPostLoad();


#ifndef NO_EDITOR
public:
	virtual Bool SupportsOutputSelection() const override;
	virtual void ToggleSelectedOutputLinkElement() override;
	virtual Uint32 GetSelectedOutputLinkElement() const override;
#endif

private:
	virtual void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};

BEGIN_CLASS_RTTI( CStorySceneFlowCondition );
	PARENT_CLASS( CStorySceneControlPart );
	PROPERTY( m_trueLink );
	PROPERTY( m_falseLink );
	PROPERTY_INLINED( m_questCondition, TXT( "Conditions that needs to be fulfilled" ) )
END_CLASS_RTTI();
