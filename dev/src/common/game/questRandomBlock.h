/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "questGraphBlock.h"

class CQuestRandomBlock : public CQuestGraphBlock
{
	friend class CUndoQuestGraphRandomBlockOutput;

	DECLARE_ENGINE_CLASS( CQuestRandomBlock, CQuestGraphBlock, 0 );
protected:
	TDynArray< CName >	m_randomOutputs;

	TInstanceVar< Int32 >								i_lastChoice;

public:
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	
	virtual String GetCaption() const { return TXT("Random"); }
	virtual String GetBlockName() const { return TXT("Random"); }
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 192, 80, 77 ); }
	virtual String GetBlockCategory() const { return TXT( "Flow control" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	
	void AddOutput();
	void RemoveOutput();
	Bool CanAddOutput();
	Bool CanRemoveOutput();
#endif
};

BEGIN_CLASS_RTTI( CQuestRandomBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY( m_randomOutputs )
END_CLASS_RTTI();

