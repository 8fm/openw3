#include "build.h"
#include "triggerManager.h"
#include "negativeAreaComponent.h"
#include "layer.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CNegativeAreaComponent );

CNegativeAreaComponent::CNegativeAreaComponent()
{
}

Color CNegativeAreaComponent::CalcLineColor() const
{
	return Color(255,0,0,0);
}

void CNegativeAreaComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	PC_SCOPE( CNegativeAreaComponent );

	TBaseClass::OnUpdateTransformComponent( context, prevLocalToWorld );
	InvalidateTouchingAreas();
}
#ifndef NO_EDITOR
void CNegativeAreaComponent::OnEditorEndVertexEdit()
{
	TBaseClass::OnEditorEndVertexEdit();
	InvalidateTouchingAreas();
}

Bool CNegativeAreaComponent::OnEditorVertexInsert( Int32 edge, const Vector& wishedPosition, Vector& allowedPosition, Int32& outInsertPos )
{
	const Bool ret = TBaseClass::OnEditorVertexInsert( edge, wishedPosition, allowedPosition, outInsertPos );
	InvalidateTouchingAreas();
	return ret;
}

Bool CNegativeAreaComponent::OnEditorVertexDestroy( Int32 vertexIndex )
{
	const Bool ret = TBaseClass::OnEditorVertexDestroy( vertexIndex );
	RED_UNUSED( ret );
	InvalidateTouchingAreas();
	return true;
}

void CNegativeAreaComponent::OnEditorNodeMoved( Int32 vertexIndex, const Vector& oldPosition, const Vector& wishedPosition, Vector& allowedPosition )
{
	TBaseClass::OnEditorNodeMoved( vertexIndex, oldPosition, wishedPosition, allowedPosition );
	InvalidateTouchingAreas();
}
#endif

void CNegativeAreaComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	TBaseClass::OnSpawned( spawnInfo );
	InvalidateTouchingAreas();
}

#ifndef NO_DATA_VALIDATION
void CNegativeAreaComponent::OnCheckDataErrors( Bool isInTemplate ) const
{
	TBaseClass::OnCheckDataErrors( isInTemplate );
}
#endif	// NO_DATA_VALIDATION

void CNegativeAreaComponent::InvalidateTouchingAreas()
{
	CLayer* parentLayer = GetLayer();
	if (NULL != parentLayer)
	{
		const LayerEntitiesArray& entities = parentLayer->GetEntities();
		for (Uint32 i=0; i<entities.Size(); ++i)
		{
			CEntity* entity = entities[i];
			if (NULL != entity)
			{
				const TDynArray<CComponent*>& components = entity->GetComponents();
				for (Uint32 j=0; j<components.Size(); ++j)
				{
					CAreaComponent* ac = Cast<CAreaComponent>(components[j]);
					if (NULL != ac && !ac->IsA<CNegativeAreaComponent>() && ac->GetBoundingBox().Touches(GetBoundingBox()))
					{
						if ( ac->GetClippingTags().Empty() || TagList::MatchAny( ac->GetClippingTags(), GetClippingTags() ) )
						{
							ac->InvalidateAreaShape();
						}
					}
				}
			}
		}
	}
}