/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "questCamera.h"

class CQuestStaticCameraSwitchBlock : public CQuestCameraBlock
{
	DECLARE_ENGINE_CLASS( CQuestStaticCameraSwitchBlock, CQuestCameraBlock, 0 )

protected:
	CName	m_nextCameraTag;

public:
	CQuestStaticCameraSwitchBlock() { m_name = TXT("Switch Static Camera"); }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }
#endif

protected:
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;
};

BEGIN_CLASS_RTTI( CQuestStaticCameraSwitchBlock )
	PARENT_CLASS( CQuestCameraBlock )
	PROPERTY_EDIT( m_nextCameraTag, TXT( "Next camera tag" ) )
END_CLASS_RTTI()