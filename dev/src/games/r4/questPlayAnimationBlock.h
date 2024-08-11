/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/game/questGraphBlock.h"
//////////////////////////////////////////////////////////////////////////

enum EPropertyAnimationOperation
{
	PAO_Play,
	PAO_Stop,
	PAO_Rewind,
	PAO_Pause,
	PAO_Unpause,
};

BEGIN_ENUM_RTTI( EPropertyAnimationOperation );
	ENUM_OPTION( PAO_Play );
	ENUM_OPTION( PAO_Stop );
	ENUM_OPTION( PAO_Rewind );
	ENUM_OPTION( PAO_Pause );
	ENUM_OPTION( PAO_Unpause );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

class CQuestPlayAnimationBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestPlayAnimationBlock, CQuestGraphBlock, 0 )

private:
	CName						m_entityTag;
	CName						m_animationName;
	EPropertyAnimationOperation	m_operation;
	Uint32						m_playCount;
	Float						m_playLengthScale;
	EPropertyCurveMode			m_playPropertyCurveMode;
	float						m_rewindTime;

public:

	CQuestPlayAnimationBlock();

	//! CObject interface
	virtual void OnSerialize( IFile& file ) { TBaseClass::OnSerialize( file ); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 232, 182, 64 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CQuestPlayAnimationBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_entityTag,				TXT( "Entity tag to to play animation on" ) );
	PROPERTY_EDIT( m_animationName,			TXT( "Animation to play on entity (needs to be defined in CGameplayEntity properties)" ) );
	PROPERTY_EDIT( m_operation,				TXT( "Operation to perform" ) );
	PROPERTY_EDIT( m_playCount,				TXT( "Count (applies only to 'Play' operation)" ) );
	PROPERTY_EDIT( m_playLengthScale,		TXT( "Length scale (applies only to 'Play' operation)" ) );
	PROPERTY_EDIT( m_playPropertyCurveMode,	TXT( "Property curve mode (applies only to 'Play' operation)" ) );
	PROPERTY_EDIT( m_rewindTime,			TXT( "Time position to rewind animation to (applies only to 'Rewind' operation)" ) );
END_CLASS_RTTI()
