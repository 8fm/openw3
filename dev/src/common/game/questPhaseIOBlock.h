/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */
#pragma once
class CQuestPhaseIOBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestPhaseIOBlock, CQuestGraphBlock )

protected:
	CName			m_socketID;

public:
	CQuestPhaseIOBlock(void);

	// Returns the ID assigned to the socket that will allow to access
	// this input from a scope block
	RED_INLINE const CName& GetSocketID() const { return m_socketID; }

#ifndef NO_EDITOR
	// exposed for deduplication purposes
	RED_INLINE void SetSocketID( const CName& id ) { m_socketID = id; }
	virtual void OnPropertyPostChange( IProperty* property );
#endif

	//! CGraphBlock interface
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Arrow; }
	virtual Color GetClientColor() const { return Color( 255, 73, 73 ); }
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
	virtual void OnDestroyed();
	virtual String GetBlockCategory() const { return TXT( "Complexity management" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }
#endif

};

BEGIN_CLASS_RTTI( CQuestPhaseIOBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_socketID, TXT( "Socket ID" ) )
END_CLASS_RTTI()

