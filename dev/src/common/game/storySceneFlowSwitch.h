#pragma once

#include "storySceneControlPart.h"

class IQuestCondition;

class CStorySceneFlowSwitchCase : public CObject
{
	DECLARE_ENGINE_CLASS( CStorySceneFlowSwitchCase, CObject, 0 )
public:
	IQuestCondition*			m_whenCondition;	//<! condition that should be pass
	CStorySceneLinkElement*		m_thenLink;			//<! path to go when condition evaluates to true

	CStorySceneFlowSwitchCase();

	virtual void OnPropertyPostChange( IProperty* property );

};
BEGIN_CLASS_RTTI( CStorySceneFlowSwitchCase );
PARENT_CLASS( CObject );
PROPERTY_INLINED( m_whenCondition, TXT( "Conditions that needs to be fulfilled" ) )
	PROPERTY( m_thenLink );	
END_CLASS_RTTI();

class CStorySceneFlowSwitch: public CStorySceneControlPart
{
	DECLARE_ENGINE_CLASS( CStorySceneFlowSwitch, CStorySceneControlPart, 0 )

private:
	TDynArray< CStorySceneFlowSwitchCase* > m_cases;
	CStorySceneLinkElement*					m_defaultLink;	//<! when all cases evaluate to false this link is used	

	void NotifyAboutSocketsChange();

public:
	CStorySceneFlowSwitch();
	void OnConnected( CStorySceneLinkElement* linkedToElement );
	void OnDisconnected( CStorySceneLinkElement* linkedToElement );
	RED_INLINE const TDynArray< CStorySceneFlowSwitchCase* >& GetCases() const { return m_cases; }
	RED_INLINE CStorySceneLinkElement* GetDefaultLink( ) { return m_defaultLink; }
	RED_INLINE const CStorySceneLinkElement* GetDefaultLink( ) const { return m_defaultLink; }
	virtual void OnPropertyPostChange( IProperty* property );	
	CStorySceneLinkElement* ChoosePathToFollow( Int32& chosenPath ) const;

private:
	void CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts ) override;
};


BEGIN_CLASS_RTTI( CStorySceneFlowSwitch );
PARENT_CLASS( CStorySceneControlPart );
PROPERTY_INLINED( m_cases,  TXT( "Cases" ) );
PROPERTY( m_defaultLink );	
END_CLASS_RTTI();