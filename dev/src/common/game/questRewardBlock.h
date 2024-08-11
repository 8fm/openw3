/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

struct LatentScriptData;

///////////////////////////////////////////////////////////////////////////////

class CQuestRewardBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestRewardBlock, CQuestGraphBlock, 0 )

protected:
	// editable data
	CName												m_rewardName;
	CName												m_targetEntityTag;
	String												m_caption;

public:
	CQuestRewardBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual void OnPropertyPostChange( IProperty* property );

	//! CGraphBlock interface
	virtual String GetCaption() const { return m_caption; }
	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 255, 255, 0 ); }
	virtual String GetBlockCategory() const { return TXT( "Gameplay" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

#endif

	//! Returns the name of a script function the block executes
	RED_INLINE const CName& GetRewardName() const { return m_rewardName; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;

private:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void UpdateCaption();
#endif
};

BEGIN_CLASS_RTTI( CQuestRewardBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_rewardName, TXT("Name of the reward"), TXT("RewardSelection") )
	PROPERTY_EDIT( m_targetEntityTag , TXT("Tag of entity to recieve item") )
	PROPERTY( m_caption );
END_CLASS_RTTI()