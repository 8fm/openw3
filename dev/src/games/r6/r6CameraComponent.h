/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once


#include "..\..\common\engine\cameraComponent.h"



class CR6CameraComponent : public CCameraComponent
{
	DECLARE_ENGINE_CLASS( CR6CameraComponent, CCameraComponent, 0 );

private:

	/// @brief Time for the camera to be fully blended in (in seconds)
	Float m_blendInTime;

	/// @brief	Is this camera the default one for its entity
	///			(if no other is chosen, this one will always be selected)
	Bool m_isDefault;


public:

	CR6CameraComponent();


	RED_INLINE Float GetBlendInTime() { return m_blendInTime; }
	RED_INLINE Bool IsDefault() { return m_isDefault; }

};





BEGIN_CLASS_RTTI( CR6CameraComponent );

	PARENT_CLASS( CCameraComponent );

	PROPERTY_EDIT( m_blendInTime, TXT( "Camera blend-in time represented in seconds." ) );
	PROPERTY_EDIT( m_isDefault, TXT( "If this is true, the camera is always selected as default if no other camera is selected. Refers to entity-scope." ) );

END_CLASS_RTTI();
