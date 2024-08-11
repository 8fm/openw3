#pragma once

#include "../../common/game/questGraphBlock.h"

enum EVehicleType
{	
	EVT_Horse,
	EVT_Boat,
	EVT_Undefined
};
BEGIN_ENUM_RTTI( EVehicleType )
	ENUM_OPTION( EVT_Horse );
	ENUM_OPTION( EVT_Boat );
	ENUM_OPTION( EVT_Undefined );
END_ENUM_RTTI()

class CQuestSpawnVehicleBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestSpawnVehicleBlock, CQuestGraphBlock, 0 )

public:

	CQuestSpawnVehicleBlock();
	virtual ~CQuestSpawnVehicleBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String	GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool	CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual Color	GetClientColor() const { return Color( 192, 135, 97 ); }
	virtual String	GetCaption() const;

	//! CGraphBlock interface
	virtual void OnRebuildSockets();

	//! CGraphBlock interface
	virtual EGraphBlockShape GetBlockShape() const { return GBS_LargeCircle; }

#endif
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
protected:
	EVehicleType	m_vehicleType;
	CName			m_spawnPointTag;
};

BEGIN_CLASS_RTTI( CQuestSpawnVehicleBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_vehicleType, TXT( "Choose which vehicle to summon" ) )
	PROPERTY_EDIT( m_spawnPointTag, TXT( "Where do you want the vehicle to spawn ?")  )
END_CLASS_RTTI()