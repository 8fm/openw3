/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/engine/camera.h"
#include "../../common/engine/staticCamera.h"

class CQuestCameraBlock : public CQuestGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestCameraBlock, CQuestGraphBlock )

protected:

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CGraphBlock interface
	virtual void OnRebuildSockets();
	virtual String GetBlockCategory() const { return TXT( "Camera" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 255, 0, 255 ); }

#endif

protected:
	CStaticCamera* FindStaticCamera( const CName& tag ) const;

public:
	virtual void OnStaticCameraStopped( InstanceBuffer& data, const CStaticCamera* camera ) const {}
};

BEGIN_CLASS_RTTI( CQuestCameraBlock )
	PARENT_CLASS( CQuestGraphBlock )
END_CLASS_RTTI()

class CQuestStaticCameraBlockListener : public IStaticCameraListener
{
	const CQuestCameraBlock*	m_owner;
	InstanceBuffer*				m_instanceData;

public:
	CQuestStaticCameraBlockListener( const CQuestCameraBlock* owner, InstanceBuffer& instanceData ) 
		: m_owner( owner ), m_instanceData( &instanceData ) {}

	virtual ~CQuestStaticCameraBlockListener() {}

	virtual void OnRunStart( const CStaticCamera* camera )
	{

	}

	virtual void OnRunEnd( const CStaticCamera* camera )
	{
		m_owner->OnStaticCameraStopped( *m_instanceData, camera );
	}
};
