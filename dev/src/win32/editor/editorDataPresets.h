#pragma once
#include "controlRigPresetsPanel.h"


class CDataPresets : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CDataPresets, CResource, "redPresets", "Red data presets" );
public:
	CObject* m_data;
};

BEGIN_CLASS_RTTI( CDataPresets )
	PARENT_CLASS( CResource )
	PROPERTY_INLINED( m_data , TXT("") )
END_CLASS_RTTI()



class CStoryScenePresets : public CObject
{
	DECLARE_ENGINE_CLASS( CStoryScenePresets, CObject, 0 )

public:
	TDynArray<CStorySceneEventPoseKeyPresetData>	m_posePresets;
};

BEGIN_CLASS_RTTI( CStoryScenePresets )
	PARENT_CLASS( CObject )
	PROPERTY_INLINED( m_posePresets, TXT("") )
END_CLASS_RTTI()
