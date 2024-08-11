
#include "build.h"
#include "questStaticCameraSwitch.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CQuestStaticCameraSwitchBlock )

void CQuestStaticCameraSwitchBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetTagManager() )
	{
		CStaticCamera* nextCamera = Cast< CStaticCamera >( world->GetTagManager()->GetTaggedEntity( m_nextCameraTag ) );
		if ( nextCamera )
		{
			Bool ret = nextCamera->Run();
			ASSERT( ret );
		}
		else
		{
			ThrowErrorNonBlocking( data, CNAME( Out ), TXT("Couldn't find static camera with tag '%ls'"), m_nextCameraTag.AsString().AsChar() );
			return;
		}
	}

	ActivateOutput( data, CNAME( Out ) );
}