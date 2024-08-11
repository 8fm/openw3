/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "areaTestComponent.h"
#include "negativeAreaComponent.h"
#include "game.h"
#include "areaComponent.h"
#include "triggerAreaComponent.h"
#include "renderFrame.h"
#include "world.h"
#include "layer.h"
#include "materialInstance.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CAreaTestComponent );

CAreaTestComponent::CAreaTestComponent()
	: m_traceDistance(0.0f)
	, m_extents(0,0,0)
	, m_searchRadius(0.0f)
{
}

void CAreaTestComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Register in editor fragment list
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_AreaShapes );
}

void CAreaTestComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// Unregister from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_AreaShapes );
}

#ifndef NO_EDITOR_FRAGMENTS
void CAreaTestComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments(frame, flag);

	if (flag == SHOW_AreaShapes)
	{
		// not used during game
		if (GGame->IsActive())
		{
			return;
		}

		// calculate the search area size
		Box localBox( -m_extents, m_extents );
		const Box worldBox = GetLocalToWorld().TransformBox(localBox);

		// scanning box
		Box scanBox( localBox );		
		scanBox.AddBox( Box(Vector::ZEROS, m_searchRadius) );
		scanBox.AddBox( Box(Vector(m_traceDistance,0,0)-m_extents, Vector(m_traceDistance,0,0)+m_extents ));	
		const Box worldScanBox = GetLocalToWorld().TransformBox(scanBox);

		// find the areas the search area (on the same layer only)
		TDynArray<CAreaComponent*> areasToTest;
		{
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
								if (NULL != ac && !ac->IsA<CNegativeAreaComponent>() && ac->GetBoundingBox().Touches(worldScanBox))
								{
									areasToTest.PushBack(ac);
								}
							}
						}
					}
				}
			}
		}

		// draw the general shape
		const Vector end = GetWorldPosition() + GetWorldForward() * m_traceDistance;
		if (m_extents.SquareMag3() > 0.0f)
		{
			// box
			const Box drawBox(GetWorldPosition() - m_extents, GetWorldPosition() + m_extents);
			frame->AddDebugBox(drawBox, Matrix::IDENTITY, Color::GRAY, false );

			// trace line
			if (m_traceDistance > 0.0f)
			{
				frame->AddDebugSphere(GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color::GRAY, false);
				frame->AddDebugLine(GetWorldPosition(), end, Color::GRAY, false);
				frame->AddDebugSphere(end, 0.1f, Matrix::IDENTITY, Color::GRAY, false);
			}
		}
		else
		{
			frame->AddDebugSphere(GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color::GRAY, false);
		}

		// end position
		if (!areasToTest.Empty())
		{
			// generate the bounding box
			if (m_extents.SquareMag3() > 0.0f)
			{
				// overlap test
				if (m_traceDistance == 0.0f)
				{
					for (Uint32 i=0; i<areasToTest.Size(); ++i)
					{
						const CAreaComponent* ac = areasToTest[i];				
						if (ac->TestBoxOverlap(worldBox))
						{
							// we have collision
							const Box drawBox(GetWorldPosition() - m_extents, GetWorldPosition() + m_extents);
							frame->AddDebugBox(drawBox, Matrix::IDENTITY, Color::RED, false );
							break;
						}
					}
				}
				else
				{
					// trace test all crap
					for (Uint32 i=0; i<areasToTest.Size(); ++i)
					{
						const CTriggerAreaComponent* tac = Cast<CTriggerAreaComponent>( areasToTest[i] );
						if ( NULL != tac )
						{
							// trace segment
							Float entryTime = FLT_MAX;
							Float exitTime = -FLT_MAX;
							if ( tac->TestBoxTrace( GetWorldPosition(), end, m_extents, entryTime, exitTime ) )
							{
								// draw entry shape
								if ( entryTime < FLT_MAX )
								{
									const Vector pos = Lerp( entryTime, GetWorldPosition(), end );
									const Box drawBox(pos - m_extents, pos + m_extents);
									frame->AddDebugBox(drawBox, Matrix::IDENTITY, Color::GREEN, false );
									frame->AddDebugText( pos, String::Printf( TXT("S: %1.3f"), entryTime), 0, 0, true );
								}

								// draw exit shape
								if ( exitTime > -FLT_MAX )
								{
									const Vector pos = Lerp( exitTime, GetWorldPosition(), end );
									const Box drawBox(pos - m_extents, pos + m_extents);
									frame->AddDebugBox(drawBox, Matrix::IDENTITY, Color::GREEN, false );
									frame->AddDebugText( pos, String::Printf( TXT("E: %1.3f"), entryTime), 0, 0, true );
								}
							}
						}
					}
				}					
			}
		}
		else if ( m_traceDistance == 0.0f )
		{
			Int32 colState = 0;

			if (!areasToTest.Empty())
			{
				colState = 1;

				for (Uint32 i=0; i<areasToTest.Size(); ++i)
				{
					const CAreaComponent* ac = areasToTest[i];				
					if (ac->TestPointOverlap(GetWorldPosition()))
					{
						colState = 2;
					}
				}
			}

			if (colState == 1)
			{
				frame->AddDebugSphere(GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color::GREEN, false);
			}
			else if (colState == 2)
			{
				frame->AddDebugSphere(GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color::RED, false);
			}
		}

		// Closest point
		if ( m_searchRadius > 0.0f )
		{
			for (Uint32 i=0; i<areasToTest.Size(); ++i)
			{
				const CAreaComponent* ac = areasToTest[i];

				Vector closestPoint;
				Float closestDistance = FLT_MAX;

				if ( ac->FindClosestPoint( GetWorldPosition(), m_searchRadius, closestPoint, closestDistance ) )
				{
					frame->AddDebugSphere(GetWorldPosition(), 0.1f, Matrix::IDENTITY, Color::RED, false);
					frame->AddDebugSphere(closestPoint, 0.2f, Matrix::IDENTITY, Color::GREEN, false);
					frame->AddDebugLine(GetWorldPosition(), closestPoint, Color::RED, false);

					const String text = String::Printf( TXT("Distance: %f"), closestDistance );
					frame->AddDebugText(GetWorldPosition(), text, 0, 0, true );
				}
			}
		}
	}
}

#endif