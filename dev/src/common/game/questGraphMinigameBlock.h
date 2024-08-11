#pragma once
#include "questgraphblock.h"
#include "minigame.h"


class CQuestGraphMinigameBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestGraphMinigameBlock, CQuestGraphBlock, 0 )

protected:
	CMinigame*								m_minigame;				
	TInstanceVar< TGenericPtr >				i_minigameData;
public:
	CQuestGraphMinigameBlock() : m_minigame( nullptr )
	{
		m_name = TXT("Minigame");
	}
	~CQuestGraphMinigameBlock()
	{}

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;

	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;


#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT( "Minigame" ); }
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 96, 191, 132 ); }
	virtual String GetBlockCategory() const { return TXT( "Game systems control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual void OnRebuildSockets();
#endif
};

BEGIN_CLASS_RTTI( CQuestGraphMinigameBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_INLINED( m_minigame, TXT("Minigame to be played") )
END_CLASS_RTTI()