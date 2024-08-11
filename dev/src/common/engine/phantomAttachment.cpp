/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "phantomAttachment.h"
#include "apexClothWrapper.h"
#include "clothComponent.h"
#include "phantomComponent.h"

IMPLEMENT_ENGINE_CLASS( CPhantomAttachment );

CPhantomAttachment::~CPhantomAttachment()
{
}

Bool CPhantomAttachment::Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info )
{
	CPhantomComponent* phantomComponent = Cast< CPhantomComponent >( child );
	// Child component should be skinned mesh component
	if ( !phantomComponent )
	{
		WARN_ENGINE( TXT("Unable to create attachment because '%ls' is not a CPhantomAttachment"), child->GetName().AsChar() );
		return false;
	}

	if( parent->QueryPhysicalCollisionTriggerCallback() == NULL )
	{
		WARN_ENGINE( TXT("Unable to create attachment because parent '%ls' haeven't got implemented IPhysicalCollisionTriggerCallback interface"), parent->GetName().AsChar() );
		return false;
	}
	
	// Initialize base attachment
	if ( !TBaseClass::Init( parent, child, info ))
	{
		return false;
	}

	if ( phantomComponent->IsAttached() )
	{
 		phantomComponent->Deactivate();
 		phantomComponent->Activate( parent );
	}

	// Created
	return true;
}


void CPhantomAttachment::CalcAttachedLocalToWorld( Matrix& out ) const
{
	CHardAttachment::CalcAttachedLocalToWorld( out );

	if( !m_takeVertexSimulationPosition ) return;

	CClothComponent* clothComponent = Cast< CClothComponent >( GetParent() );
	if( !clothComponent ) return;

#ifdef USE_APEX
	CApexClothWrapper* wrapper = clothComponent->GetClothWrapper();
	if( !wrapper ) return;

	Vector simulationPosition = wrapper->GetFirstSimulationPosition();
	out.SetTranslation( simulationPosition );
#endif
}