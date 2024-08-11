#pragma once

//////////////////////////////////////////////////////////////////////////

#include "questSpawnVehicleBlock.h"

//////////////////////////////////////////////////////////////////////////

class CNotStreamedBoatSpawnEventHandler : public ISpawnEventHandler
{
public:
    CNotStreamedBoatSpawnEventHandler( CName layerTag )
        : m_spawnLayerTag( layerTag )
    {}
    virtual ~CNotStreamedBoatSpawnEventHandler(){};

protected:
    void OnPostAttach( CEntity* entity ) override;

protected:
    const CName m_spawnLayerTag;
};

//////////////////////////////////////////////////////////////////////////

class CQuestSpawnNotStreamedBoatBlock : public CQuestGraphBlock
{
DECLARE_ENGINE_CLASS( CQuestSpawnNotStreamedBoatBlock, CQuestGraphBlock, 0 );

public:
    CQuestSpawnNotStreamedBoatBlock(void);
    virtual ~CQuestSpawnNotStreamedBoatBlock(void);

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

private:
    TDynArray<CName>    m_tagsToSet;
    CName               m_spawnLayerTag;
    CName               m_spawnPointTag;
    Bool                m_forceNonStreamed;
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CQuestSpawnNotStreamedBoatBlock );
    PARENT_CLASS( CQuestGraphBlock );

    PROPERTY_EDIT( m_spawnPointTag,     TXT( "Where do you want the vehicle to spawn ?")  )
    PROPERTY_EDIT( m_tagsToSet,         TXT("Tags to set for newly spawned boat") );
    PROPERTY_EDIT( m_spawnLayerTag,     TXT("Layer name to spawn to, if none spawns to dynamic layer") );
    PROPERTY_EDIT( m_forceNonStreamed,  TXT("Forces entity to be non streamed") );

END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
