/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/game/behTreeNodeAtomicAction.h"

class CBehTreeNodeDebugLogInstance;

class CBehTreeNodeDebugLogDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDebugLogDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeDebugLogInstance, DebugLog )
	DECLARE_AS_R6_ONLY

protected:
	CBehTreeValCName	m_channel;
	CBehTreeValString	m_text;

public:
	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = nullptr ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDebugLogDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
	PROPERTY_EDIT( m_channel, TXT("Log channel to write to") )
	PROPERTY_EDIT( m_text, TXT("Text to log") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDebugLogInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	String		m_text;

public:
	typedef CBehTreeNodeDebugLogDefinition Definition;

	CBehTreeNodeDebugLogInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
};