
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "meshComponent.h"

/// Mesh that shows and hides based on the streaming status
class CImpostorMeshComponent : public CMeshComponent
{
	DECLARE_ENGINE_CLASS( CImpostorMeshComponent, CMeshComponent, 0 )

protected:
	String				m_layerGroupName;
	CLayerGroup*		m_groupToHide;

public:
	virtual void OnAttached( CWorld* world );
	virtual Float GetStreamingTransparency() const;
};

BEGIN_CLASS_RTTI( CImpostorMeshComponent );
	PARENT_CLASS( CMeshComponent );
	PROPERTY_EDIT( m_layerGroupName, TXT("Name of the group, that streams in/out and changing transparency of the ImpostorMeshComponent") );
END_CLASS_RTTI();