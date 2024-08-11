/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "gameplayEntity.h"


class CDeniedAreaSaveable : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CDeniedAreaSaveable, CGameplayEntity, 0 )

public:
	CDeniedAreaSaveable();
	virtual ~CDeniedAreaSaveable();

	void SetEnabled( Bool enabled );

	virtual void OnAttached( CWorld* world );
	virtual void OnPostRestoreState();

private:
	Bool	m_isEnabled;
};

BEGIN_CLASS_RTTI( CDeniedAreaSaveable )
	PARENT_CLASS( CGameplayEntity )
	PROPERTY_EDIT_SAVED( m_isEnabled, TXT( "Is denied area enabled" ) )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////////////////

class CEnableDeniedAreaRequest : public IEntityStateChangeRequest
{
	DECLARE_ENGINE_CLASS( CEnableDeniedAreaRequest, IEntityStateChangeRequest, 0 );

private:
	Bool			m_enable;

public:
	CEnableDeniedAreaRequest( Bool enable = true );

	RED_INLINE void SetEnabled( Bool enable ) { m_enable = enable; }

	virtual void Execute( CGameplayEntity* entity );
	virtual String OnStateChangeRequestsDebugPage() const;
};
BEGIN_CLASS_RTTI( CEnableDeniedAreaRequest );
	PARENT_CLASS( IEntityStateChangeRequest );
	PROPERTY_EDIT_SAVED( m_enable, TXT("Enable/disable?") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////

class CDeniedAreaBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CDeniedAreaBlock, CQuestGraphBlock, 0 )

public:
	CDeniedAreaBlock();
	virtual ~CDeniedAreaBlock() {}

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual String GetCaption() const { return TXT( "Denied area" ); }

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 96, 191, 132 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputNamee, CQuestThread* parentThread ) const;

private:
	CName	m_entityTag;
	Bool	m_enabled;
};

BEGIN_CLASS_RTTI( CDeniedAreaBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_entityTag, TXT( "Tag of a denied area entity" ) );
	PROPERTY_EDIT( m_enabled, TXT( "Enable/disable denied area" ) );
END_CLASS_RTTI()
