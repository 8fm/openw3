/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questGraphBlock.h"
//////////////////////////////////////////////////////////////////////////

enum EQuestManageFastTravelOperation
{
	QMFT_EnableAndShow = 0,
	QMFT_EnableOnly,
	QMFT_ShowOnly,
};

BEGIN_ENUM_RTTI( EQuestManageFastTravelOperation );
	ENUM_OPTION( QMFT_EnableAndShow );
	ENUM_OPTION( QMFT_EnableOnly );
	ENUM_OPTION( QMFT_ShowOnly );
END_ENUM_RTTI();

class CQuestManageFastTravelBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestManageFastTravelBlock, CQuestGraphBlock, 0 )

private:
	EQuestManageFastTravelOperation		m_operation;
	Bool								m_enable;
	Bool								m_show;
	TDynArray< Int32 >	m_affectedAreas;
	TDynArray< CName >	m_affectedFastTravelPoints;

public:

	CQuestManageFastTravelBlock()
		: m_operation( QMFT_EnableAndShow )
		, m_enable( false )
		, m_show( false )
	{
		m_name = TXT("Manage fast travel");
	}

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Slanted; }
	virtual Color GetClientColor() const { return Color( 20, 141, 35 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CQuestManageFastTravelBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_operation, TXT( "Operation to do on fast travel points" ) )
	PROPERTY_EDIT( m_enable, TXT( "Enable/disable fast travel points" ) )
	PROPERTY_EDIT( m_show, TXT( "Show/hide fast travel points" ) )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_affectedAreas, TXT( "Affected areas" ), TXT("ScriptedEnum_EAreaName" ) )
	PROPERTY_EDIT( m_affectedFastTravelPoints, TXT( "Affected points" ) )
END_CLASS_RTTI()
