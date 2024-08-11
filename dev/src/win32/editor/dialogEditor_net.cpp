
#include "build.h"
#include "dialogEditor.h"

void CEdSceneEditor::OnNetwork_SetCamera( const Vector& pos, const EulerAngles& rot, Float fov )
{
	m_camera.SetCameraFromNet( pos, rot, fov );
}
