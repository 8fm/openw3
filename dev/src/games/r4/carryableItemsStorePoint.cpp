#include "build.h"

#include "../../common/core/gatheredResource.h"

#include "carryableItemsStorePoint.h"

IMPLEMENT_ENGINE_CLASS( CCarryableItemStorePointComponent );

IRenderResource*	CCarryableItemStorePointComponent::s_markerValid;
IRenderResource*	CCarryableItemStorePointComponent::s_markerInvalid;
IRenderResource*	CCarryableItemStorePointComponent::s_markerNoMesh; 
IRenderResource*	CCarryableItemStorePointComponent::s_markerSelection;

CGatheredResource resItems( TXT("gameplay\\globals\\carryable_items.csv"), RGF_Startup );	

void CCarryableItemStorePointComponent::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
	valueProperties.m_array = resItems.LoadAndGet< C2dArray >();
	valueProperties.m_valueColumnName = TXT("name");	
}

void CCarryableItemStorePointComponent::InitializePointMarkers()
{
	struct InitOnce
	{
		InitOnce()
		{		
			s_markerValid = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color( 56, 195, 255 ) );
			s_markerInvalid = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color::RED );
			s_markerNoMesh = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color::GRAY );
			s_markerSelection = CWayPointComponent::CreateAgentMesh( 0.4f, 1.8f, Color::WHITE, true );
		}
	};
	static InitOnce initOncePP;
}
IRenderResource* CCarryableItemStorePointComponent::GetMarkerValid()
{
	InitializePointMarkers();
	return s_markerValid;
}
IRenderResource* CCarryableItemStorePointComponent::GetMarkerInvalid()
{
	InitializePointMarkers();
	return s_markerInvalid;
}
IRenderResource* CCarryableItemStorePointComponent::GetMarkerNoMesh()
{
	InitializePointMarkers();
	return s_markerNoMesh;
}
IRenderResource* CCarryableItemStorePointComponent::GetMarkerSelection()
{
	InitializePointMarkers();
	return s_markerSelection;
}

void CCarryableItemStorePointComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CCarryableItemStorePointComponent_OnAttached );

	if( !GR4Game )
		return;
	m_itemSpawnDistance = GR4Game->GetCarryableItemsRegistry()->GetSpawnDistance( m_storedItemType );
	if( GGame->IsActive() && m_randomPrespawn )
	{
		THandle< CEntity > item = GR4Game->GetCarryableItemsRegistry()->CreateItem( m_storedItemType, CalculateNextItemPosition(), GetEntity()->GetWorldRotation() );
		if( item.Get() )
		{
			m_storedItems.PushBack( item );
		}
	}
}

void CCarryableItemStorePointComponent::PutItem( CEntity* item )
{
	if( !item )
		return;

	m_storedItems.PushBack( item );
	item->SetPosition( CalculateNextItemPosition() );
	item->SetRotation( GetEntity()->GetWorldRotation() );
}

Vector3 CCarryableItemStorePointComponent::CalculateNextItemPosition()
{
	return GetEntity()->GetWorldPosition() + GetEntity()->GetWorldForward() * m_itemSpawnDistance;
}

float CCarryableItemStorePointComponent::CalculateHeadingForPickItem()
{
	return GetEntity()->GetWorldRotation().Yaw;
}

float CCarryableItemStorePointComponent::CalculateHeadingForDropItem()
{
	return GetEntity()->GetWorldRotation().Yaw;
}