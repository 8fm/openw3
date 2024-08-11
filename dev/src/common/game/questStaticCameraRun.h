/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questCamera.h"

class CQuestStaticCameraRunBlock : public CQuestCameraBlock
{
	DECLARE_ENGINE_CLASS( CQuestStaticCameraRunBlock, CQuestCameraBlock, 0 )

protected:
	CName	m_cameraTag;

public:
	CQuestStaticCameraRunBlock() { m_name = TXT("Run Static Camera"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
#endif

protected:
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestStaticCameraRunBlock )
	PARENT_CLASS( CQuestCameraBlock )
	PROPERTY_EDIT( m_cameraTag, TXT( "Camera tag" ) )
END_CLASS_RTTI()
