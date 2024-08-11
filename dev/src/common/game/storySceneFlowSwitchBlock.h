#pragma once

#include "storySceneGraphBlock.h"
#include "storySceneLinkHub.h"
#include "storySceneFlowSwitch.h"

class CStorySceneFlowSwitchBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneFlowSwitchBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneFlowSwitch*	m_switch;
	String					m_description;

public:
	CStorySceneFlowSwitch* GetSwitch() const { return m_switch; }

	virtual CStorySceneControlPart* GetControlPart() const override;
	virtual void SetControlPart( CStorySceneControlPart* part ) override;

public:
	CStorySceneFlowSwitchBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Recreate layout of the block
	virtual void OnRebuildSockets();

	//! Return block shape to draw
	virtual EGraphBlockShape GetBlockShape() const;

	//! Determine block color that is used to fill the client area
	virtual Color GetClientColor() const;

	//! Get block caption
	virtual String GetCaption() const;

	virtual void OnDestroyed();

	virtual void OnPropertyPostChange( IProperty* property );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneFlowSwitchBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY_EDIT( m_description, TXT( "Description of a flow switch" ) );
	PROPERTY_INLINED_RO( m_switch, TXT( "Property of a flow switch structure" ) );	
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CStorySceneLinkHubBlock : public CStorySceneGraphBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneLinkHubBlock, CStorySceneGraphBlock, 0 )

private:
	CStorySceneLinkHub*		m_hub;

public:
	virtual CStorySceneControlPart* GetControlPart() const override;
	virtual void SetControlPart( CStorySceneControlPart* part ) override;

public:
	CStorySceneLinkHubBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();

	virtual EGraphBlockShape GetBlockShape() const override { return GBS_Default; }
	virtual Color GetClientColor() const override { return TBaseClass::GetClientColor(); }
	virtual String GetCaption() const override { return String( TXT("Hub") ); }
	virtual void OnDestroyed() override;
#endif

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnPropertyPostChange( IProperty* property );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneLinkHubBlock );
	PARENT_CLASS( CStorySceneGraphBlock );
	PROPERTY_INLINED_RO( m_hub, TXT( "Property of a flow hub" ) );
END_CLASS_RTTI();
