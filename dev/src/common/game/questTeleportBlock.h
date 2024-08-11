/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestTeleportBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestTeleportBlock, CQuestGraphBlock, 0 )

private:
	//CName			m_locationTag;
	TagList			m_locationTag;
	Float			m_distance;
	Float			m_distanceToDestination;
	TagList			m_actorsTags;
	Bool			m_cameraCorrectionIfPlayer;

public:
	CQuestTeleportBlock() : m_cameraCorrectionIfPlayer( true ) { m_name = TXT("Teleport"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 192, 80, 77 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

public:
	Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );
};

BEGIN_CLASS_RTTI( CQuestTeleportBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_locationTag, TXT( "Where would you like to go today?" ), TXT( "TagListEditor" ) )
	PROPERTY_EDIT( m_distance, TXT( "How far behind the main actor should the others be teleported." ) )
	PROPERTY_EDIT( m_distanceToDestination, TXT( "Additional distance from location tag" ) )
	PROPERTY_CUSTOM_EDIT( m_actorsTags, TXT( "Who would you like to teleport?" ), TXT( "TagListEditor" ) )
END_CLASS_RTTI()