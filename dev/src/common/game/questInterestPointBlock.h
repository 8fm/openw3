/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../game/interestPointComponent.h"


class CQuestInterestPointBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestInterestPointBlock, CQuestGraphBlock, 0 )

private:
	CInterestPoint*		m_interestPoint;
	CName						m_positionTag;
	Float						m_duration;

public:
	CQuestInterestPointBlock() { m_name = TXT("Interest point emitter"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Rounded; }
	virtual Color GetClientColor() const { return Color( 155, 187, 89 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestInterestPointBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_interestPoint, TXT( "Interest point" ) )
	PROPERTY_EDIT( m_positionTag, TXT( "Tag of the point where the interest will be emitted." ) )
	PROPERTY_EDIT( m_duration, TXT( "For how long should the emission last?" ) )
END_CLASS_RTTI()
