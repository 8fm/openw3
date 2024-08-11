/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class IEditorPreviewCameraProvider
{
public:
	struct Info
	{
		Vector		m_cameraPostion;
		EulerAngles	m_cameraRotation;
		Float		m_cameraFov;
		Float		m_lightPosition;
		String		m_envPath;
	};

	virtual Info GetPreviewCameraInfo() const =0;
};
