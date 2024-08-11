/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CQuestStaticCameraStopBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_CLASS( CQuestStaticCameraStopBlock, CQuestGraphBlock, 0 )
private:
	Float m_blendTime;
public:
	CQuestStaticCameraStopBlock() : m_blendTime( 0.f ) { m_name = TXT("Activate Game Camera"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual String GetBlockCategory() const { return TXT( "Camera" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 255, 0, 255 ); }
#endif

protected:
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestStaticCameraStopBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_EDIT( m_blendTime, TXT("Camera blend in time") )
END_CLASS_RTTI()