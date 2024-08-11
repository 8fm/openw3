/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "crowdEntryPoint.h"
#include "crowdManager.h"
#include "../../common/engine/areaComponent.h"

IMPLEMENT_ENGINE_CLASS( CCrowdEntryPoint );

void CCrowdEntryPoint::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	CR6Game* r6Game = Cast< CR6Game >( GGame );

	CCrowdManager* crowd = r6Game->GetSystem< CCrowdManager >();
	if ( crowd )
	{
		crowd->RegisterEntryPoint( this );
	}
}

Box CCrowdEntryPoint::GetBoundingBox() const
{
	Box retBox( Vector::ZEROS, 0.f );

	const CAreaComponent* areaComponent = FindComponent< const CAreaComponent >();
	if ( areaComponent )
	{
		retBox = areaComponent->GetBoundingBox();
	}
	return retBox;
}

Box2 CCrowdEntryPoint::GetBoundingBox2() const
{
	const CAreaComponent* areaComponent = FindComponent< const CAreaComponent >();
	if ( areaComponent )
	{
		const Box& box = areaComponent->GetBoundingBox();
		return Box2( box.Min.AsVector2(), box.Max.AsVector2() ); 
	}
	
	return Box2( Vector::ZEROS.AsVector2(), Vector::ZEROS.AsVector2() );
}

Vector2 CCrowdEntryPoint::RandomPositionInside2() const
{
	Vector2 vec;
	Box2 boundingBox = GetBoundingBox2();

	vec.X = GEngine->GetRandomNumberGenerator().Get< Float >( boundingBox.Min.X , boundingBox.Max.X );
	vec.Y = GEngine->GetRandomNumberGenerator().Get< Float >( boundingBox.Min.Y , boundingBox.Max.Y );

	return vec;
}