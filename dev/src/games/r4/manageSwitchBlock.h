/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questGraphBlock.h"
//////////////////////////////////////////////////////////////////////////

enum ESwitchOperation
{
	SO_TurnOn,
	SO_TurnOff,
	SO_Toggle,
	SO_Reset,
	SO_Enable,
	SO_Disable,
	SO_Lock,
	SO_Unlock,
};

BEGIN_ENUM_RTTI( ESwitchOperation );
	ENUM_OPTION( SO_TurnOn );
	ENUM_OPTION( SO_TurnOff );
	ENUM_OPTION( SO_Toggle );
	ENUM_OPTION( SO_Reset );
	ENUM_OPTION( SO_Enable );
	ENUM_OPTION( SO_Disable );
	ENUM_OPTION( SO_Lock );
	ENUM_OPTION( SO_Unlock );
END_ENUM_RTTI();

class CManageSwitchBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CManageSwitchBlock, CQuestGraphBlock, 0 )

private:
	CName											m_switchTag;
	TDynArray< ESwitchOperation >	m_operations;
	Bool											m_force;
	Bool											m_skipEvents;

public:

	CManageSwitchBlock();

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
	virtual Color GetClientColor() const { return Color( 60, 60, 189 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CManageSwitchBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_switchTag, TXT("Tag of the switch") )
	PROPERTY_EDIT( m_operations, TXT("Operations to perform on switch") );
	PROPERTY_EDIT( m_force, TXT("Force even if switch is disabled (applicable only for turning on/off)") );
	PROPERTY_EDIT( m_skipEvents, TXT("Skip events associated with with switch (applicable only for turning on/off)") );
END_CLASS_RTTI()
