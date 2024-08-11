/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestLayersHiderBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestLayersHiderBlock, CQuestGraphBlock, 0 )

private:
	String											m_world;
	TDynArray< String >								m_layersToShow;				//!< Layers we want this block to activate when it gets active
	TDynArray< String >								m_layersToHide;				//!< Layers we want this block to activate when it gets active

	Bool											m_syncOperation;			//!< Wait for the operation to finish
	Bool											m_purgeSavedData;			//!< Forget all the persistent data from hidden layers 

	TInstanceVar< TGenericPtr >						i_layerStreamingFence;

public:
	CQuestLayersHiderBlock();

	const String& GetWorld() const { return m_world; }
	const TDynArray< String >& GetLayersToShow() const { return m_layersToShow; }
	const TDynArray< String >& GetLayersToHide() const { return m_layersToHide; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual String GetCaption() const { return TXT("Hide Layers"); }

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 71, 199, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
#endif

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;

	// internal stuff
private: 
	RED_INLINE Bool IsCurrentWorld() const { return m_world.Empty() || ( GGame->GetActiveWorld() ? m_world.EqualsNC( GGame->GetActiveWorld()->GetDepotPath() ) : false ); }
};

BEGIN_CLASS_RTTI( CQuestLayersHiderBlock );
	PARENT_CLASS( CQuestGraphBlock );
	PROPERTY_CUSTOM_EDIT( m_world, TXT( "A game world. Leaving this field blank means: a world, which is active when this block gets executed. Please try not to leave this field empty." ), TXT("CSVWorldSelection") );
	PROPERTY_CUSTOM_EDIT( m_layersToShow, TXT( "Layers that should be affected by this block." ), TXT("LayerGroupList") );
	PROPERTY_CUSTOM_EDIT( m_layersToHide, TXT( "Layers that should be affected by this block." ), TXT("LayerGroupList") );
	PROPERTY_EDIT( m_syncOperation, TXT( "Wait for the operation to finish?" ) );
	PROPERTY_EDIT( m_purgeSavedData, TXT("Forget all the persistent data from hidden layers") );
END_CLASS_RTTI();

