
#include "build.h"
#include "questStaticCameraRun.h"

IMPLEMENT_ENGINE_CLASS( CQuestStaticCameraRunBlock )

void CQuestStaticCameraRunBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CStaticCamera* cam = FindStaticCamera( m_cameraTag );
	if ( cam )
	{
		Bool ret = cam->Run();
		ASSERT( ret );
	}
	else
	{
		ThrowErrorNonBlocking( data, CNAME( Out ), TXT("Couldn't find static camera with tag '%ls'"), m_cameraTag.AsString().AsChar() );
		return;
	}

	ActivateOutput( data, CNAME( Out ) );
}