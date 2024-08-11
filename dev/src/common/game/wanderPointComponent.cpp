#include "build.h"
#include "wanderPointComponent.h"

#include "../engine/pathlibWorld.h"
#include "../engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( SWanderPointConnection )
IMPLEMENT_ENGINE_CLASS( CWanderPointComponent )

IRenderResource*	CWanderPointComponent::s_markerValid;
IRenderResource*	CWanderPointComponent::s_markerInvalid;
IRenderResource*	CWanderPointComponent::s_markerNoMesh; 
IRenderResource*	CWanderPointComponent::s_markerSelection;

void CWanderPointComponent::OnPropertyPreChange( IProperty* property )
{
#ifndef NO_EDITOR
	if ( property->GetName().AsString() == TXT("connectedPoints") )
	{
		ChangeLinks( false );
	}
	
#endif

	TBaseClass::OnPropertyPreChange( property );
}

void CWanderPointComponent::OnPropertyPostChange( IProperty* property )
{
#ifndef NO_EDITOR
	if ( property->GetName().AsString() == TXT("connectedPoints") )
	{
		ChangeLinks( true );
		m_connectionsAvailable.ClearFast();
	}
	else if ( property->GetName().AsString() == TXT("wanderPointRadius") )
	{
		MarkCachedNavtestsDirty();
	}
#endif

	TBaseClass::OnPropertyPostChange( property );
}

Bool CWanderPointComponent::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName.AsString() == TXT("connectedPoints") )
	{
		TDynArray< EntityHandle > prevData;
		if ( readValue.AsType( prevData ) )
		{
			m_connectedPoints.Resize( prevData.Size() );
			for ( Uint32 i = 0, n = prevData.Size(); i != n; ++i )
			{
				m_connectedPoints[ i ].m_destination = prevData[ i ];
			}
			return true;
		}
	}
	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

void CWanderPointComponent::InitializePointMarkers()
{
	struct InitOnce
	{
		InitOnce()
		{		
			s_markerValid = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color::LIGHT_BLUE );
			s_markerInvalid = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color::RED );
			s_markerNoMesh = CWayPointComponent::CreateAgentMesh( 0.35f, 1.8f, Color::GRAY );
			s_markerSelection = CWayPointComponent::CreateAgentMesh( 0.4f, 1.8f, Color::WHITE, true );
		}
	};
	static InitOnce initOncePP;
}
IRenderResource* CWanderPointComponent::GetMarkerValid()
{
	InitializePointMarkers();
	return s_markerValid;
}
IRenderResource* CWanderPointComponent::GetMarkerInvalid()
{
	InitializePointMarkers();
	return s_markerInvalid;
}
IRenderResource* CWanderPointComponent::GetMarkerNoMesh()
{
	InitializePointMarkers();
	return s_markerNoMesh;
}
IRenderResource* CWanderPointComponent::GetMarkerSelection()
{
	InitializePointMarkers();
	return s_markerSelection;
}

#ifndef NO_EDITOR
void CWanderPointComponent::MarkCachedNavtestsDirty()
{
	m_wanderAreaIsAvailable = DS_NOT_CALCULATED;
	m_connectionsAvailable.ClearFast();

	for( Uint32 i=0, n = m_connectedPoints.Size(); i < n; ++i )
	{
		CEntity* ent = m_connectedPoints[i].m_destination.Get();
		if( ent && ent->IsAttached() )
		{
			CWanderPointComponent* wc = ent->FindComponent< CWanderPointComponent >();
			if ( wc )
			{
				wc->m_connectionsAvailable.ClearFast();
			}
		}
	}
}
void CWanderPointComponent::WaypointGenerateEditorFragments( CRenderFrame* frame )
{
	CWorld* world = GetEntity()->GetLayer()->GetWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		PathLib::AreaId areaId = PathLib::INVALID_AREA_ID;

		const Color COLOR_NOTINITIALIZED = Color::LIGHT_YELLOW;
		const Color COLOR_INVALID = Color::LIGHT_RED;
		const Color COLOR_OK = Color::LIGHT_BLUE;
		const Color COLOR_PATHFIND = Color::GREEN;

		const Vector& worldPos = GetWorldPositionRef();
		if ( m_wanderPointRadius > 0.f )
		{
			Color radiusColor = COLOR_NOTINITIALIZED;
			if ( m_wanderAreaIsAvailable == DS_NOT_CALCULATED )
			{

				{
					if ( pathlib->IsLocationLoaded( worldPos.AsVector3(), areaId ) )
					{
						if ( pathlib->TestLocation( areaId, worldPos.AsVector3(), m_wanderPointRadius, PathLib::CT_DEFAULT ) )
						{
							m_wanderAreaIsAvailable = DS_OK;
						}
						else
						{
							m_wanderAreaIsAvailable = DS_INVALID;
						}
					}
				}
			}

			if ( m_wanderAreaIsAvailable != DS_NOT_CALCULATED )
			{
				radiusColor = (m_wanderAreaIsAvailable == DS_OK) ? COLOR_OK : COLOR_INVALID;
			}

			frame->AddDebugWireframeTube( worldPos, worldPos + Vector( 0.f, 0.f, 1.5f ), m_wanderPointRadius, m_wanderPointRadius, Matrix::IDENTITY, radiusColor, radiusColor, false, 8 );
		}

		if( m_connectedPoints.Size() > 0 )
		{
			if ( m_connectionsAvailable.Size() != m_connectedPoints.Size() )
			{
				m_connectionsAvailable.ResizeFast( m_connectedPoints.Size() );
				for( Uint32 i=0, n = m_connectedPoints.Size(); i < n; ++i )
				{
					m_connectionsAvailable[ i ] = DS_NOT_CALCULATED;
				}
			}

			for( Uint32 i=0, n = m_connectedPoints.Size(); i < n; ++i )
			{
				CEntity* ent = m_connectedPoints[i].m_destination.Get();
				if( ent && ent->IsAttached() )
				{
					const Vector& entPos = ent->GetWorldPositionRef();
					Color connectionColor = COLOR_NOTINITIALIZED;

					CWanderPointComponent* wc = ent->FindComponent< CWanderPointComponent >();
					if ( wc )
					{
						Bool forcePathfind = m_connectedPoints[ i ].m_forcePathfinding;
						if ( m_connectionsAvailable[ i ] == DS_NOT_CALCULATED && m_wanderAreaIsAvailable == DS_OK && !forcePathfind )
						{
							if ( pathlib->IsLocationLoaded( entPos.AsVector3(), areaId ) )
							{
								struct CustomTester : public PathLib::SCustomCollisionTester
								{
									CustomTester( const Vector3& p1, const Vector3& p2, Float r1, Float r2 )
										: m_p1( p1 )
										, m_p2( p2 )
										, m_r1( r1 )
										, m_r2( r2 )
										, m_len( Max( (p2.AsVector2() - p1.AsVector2()).Mag(), 0.01f ) )
									{}
									Bool IntersectLine( const Vector2& point1, const Vector2& point2 ) override
									{
										const Vector2& p1 = m_p1.AsVector2();
										const Vector2& p2 = m_p2.AsVector2();

										Float r1, r2;
										MathUtils::GeometryUtils::ClosestPointsLineLine2D( point1, point2, p1, p2, r1, r2 );
										Vector2 i1 = point1 + (point2 - point1) * r1;
										Vector2 i2 = p1 + (p2 - p1) * r2;

										Float radius = m_r1 + (m_r2 - m_r1) * r2;

										return (i1 - i2).SquareMag() < radius * radius;
									}
									Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) override
									{
										const Vector2& p1 = m_p1.AsVector2();
										const Vector2& p2 = m_p2.AsVector2();

										Vector2 i1, i2;
										MathUtils::GeometryUtils::ClosestPointLineRectangle2D( p1, p2, rectMin, rectMax, i1, i2 );
										Float lineDist = (i1 - p1).SquareMag();
										Float ratio = (lineDist < NumericLimits< Float >::Epsilon()) ?
											 0.f :
											sqrt( lineDist ) / m_len;
										Float radius = m_r1 + (m_r2 - m_r1) * ratio;
										return (i1 - i2).SquareMag() < radius * radius;
									}
									void ComputeBBox( Box& outBBox ) override
									{
										Box b1( m_p1, m_r1 );
										Box b2( m_p2, m_r2 );
										outBBox = b1;
										outBBox.AddBox( b2 );
										outBBox.Max.Z += 2.f;
									}

									Vector3 m_p1;
									Vector3 m_p2;
									Float	m_r1;
									Float	m_r2;
									Float	m_len;

								} customTester( worldPos.AsVector3(), entPos.AsVector3(), m_wanderPointRadius, wc->m_wanderPointRadius );
								if ( pathlib->CustomCollisionTest( worldPos, customTester ) )
								{
									 m_connectionsAvailable[ i ] = DS_OK;
								}
								else
								{
									m_connectionsAvailable[ i ] = DS_INVALID;
								}
							}
						}
						
						Bool drawSideLines = true;
						if ( forcePathfind )
						{
							connectionColor = COLOR_PATHFIND;
							drawSideLines = false;
						}
						else if ( m_connectionsAvailable[ i ] != DS_NOT_CALCULATED )
						{
							connectionColor = ( m_connectionsAvailable[ i ] == DS_OK ) ? COLOR_OK : COLOR_INVALID;
						}
						else if ( m_wanderAreaIsAvailable == DS_INVALID )
						{
							connectionColor = COLOR_INVALID;
						}

						if ( drawSideLines )
						{
							Vector2 lineHeading = (entPos.AsVector2() - worldPos.AsVector2()).Normalized();
							Vector2 lineL = MathUtils::GeometryUtils::PerpendicularL( lineHeading );
							Vector2 lineR = MathUtils::GeometryUtils::PerpendicularR( lineHeading );

							Vector v1L = worldPos + lineL * m_wanderPointRadius;
							Vector v2L = entPos + lineL * wc->m_wanderPointRadius;
							Vector v1R = worldPos + lineR * m_wanderPointRadius;
							Vector v2R = entPos + lineR * wc->m_wanderPointRadius;

							v1L.Z += 0.2f;
							v2L.Z += 0.2f;
							v1R.Z += 0.2f;
							v2R.Z += 0.2f;

							frame->AddDebugLine( v1L, v2L, connectionColor, false );
							frame->AddDebugLine( v1R, v2R, connectionColor, false );
						}
					}

					frame->AddDebugLineWithArrow( worldPos, entPos, 0.5f, 1, 1, connectionColor, true );
				}

			}
		}
	}

	TBaseClass::WaypointGenerateEditorFragments( frame );
}
void CWanderPointComponent::EditorOnTransformChanged()
{
	MarkCachedNavtestsDirty();

	TBaseClass::EditorOnTransformChanged();
}
void CWanderPointComponent::EditorPreDeletion()
{
	RemoveAllConnections();

	TBaseClass::EditorPreDeletion();
}

void CWanderPointComponent::EditorPostDuplication( CNode* originalNode )
{
	if ( !originalNode )
		return;

	TBaseClass::EditorPostDuplication( originalNode );

	//reset all connections on this component
	RemoveAllConnections();

	CEntity* originalEntity = Cast< CEntity >( originalNode );
	if ( originalEntity )
	{
		CWanderPointComponent* originalComponent = originalEntity->FindComponent<CWanderPointComponent>();
		if ( originalComponent )
		{
			Bool forcePathfind = false;

			//figure out pathfinding param 1st (this loop guarantees that either all connections are true or our bool remains false
			for( auto it = originalComponent->m_connectedPoints.Begin(), end = originalComponent->m_connectedPoints.End(); it != end; ++it )
			{
				forcePathfind = (*it).m_forcePathfinding;
				
				//break at the 1st false
				if (!forcePathfind)
					break;
			}

			//add a two-way connection between this and the original
			originalComponent->AddConnection( GetEntity(), forcePathfind );
			AddConnection( originalEntity, forcePathfind );
		}
	}
}
#endif		// !NO_EDITOR

void CWanderPointComponent::ChangeLinks( Bool generate, Bool twoSideRemoval )
{
	// Report errors and enforce auto connnections
	for( Uint32 i = 0; i < m_connectedPoints.Size(); ++i )
	{
		CEntity* entity = m_connectedPoints[i].m_destination.Get();
		if( entity )
		{
			CWanderPointComponent* wanderPointComponent = entity->FindComponent< CWanderPointComponent >();
			if( wanderPointComponent )
			{
				if( generate )
				{
					wanderPointComponent->AddConnection( this->GetEntity(), m_connectedPoints[ i ].m_forcePathfinding );
				}
				else
				{
					wanderPointComponent->RemoveConnection( GetEntity(), twoSideRemoval );
				}
			}
			else
			{
				HALT( "Wander point %s is connected to %s, which is not a wander point!", this->m_name.AsChar(), m_connectedPoints[i].m_destination.Get()->GetName().AsChar() );
			}
		}
	}
}

Bool CWanderPointComponent::IsConnectedTo( CEntity* wanderPoint, Int32& index )
{
	if( wanderPoint )
	{
		for( Uint32 i = 0; i < m_connectedPoints.Size(); ++i )
		{
			if( m_connectedPoints[i].m_destination.Get() == wanderPoint )
			{
				index = i;
				return true;
			}
		}
	}

	return false;
}

Bool CWanderPointComponent::AddConnection( CEntity* wanderPoint, Bool forcePathfind )
{
	Int32 index = -1;
	if( !IsConnectedTo( wanderPoint, index ) )
	{
		SWanderPointConnection c;
		c.m_destination.Set( wanderPoint );
		c.m_forcePathfinding = forcePathfind;
		m_connectedPoints.PushBack( c );
		return true;
	}

	return false;
}

Bool CWanderPointComponent::RemoveConnection( CEntity* wanderPoint, Bool bothSides )
{
	Int32 index = -1;
	if( wanderPoint && IsConnectedTo( wanderPoint, index ) )
	{
		if( bothSides )
		{
			CEntity* entity = m_connectedPoints[index].m_destination.Get();
			
			if ( entity )
			{
				CWanderPointComponent* wpComponent = entity->FindComponent<CWanderPointComponent>();
				
				if ( wpComponent )
				{
					wpComponent->RemoveConnection( GetEntity(), false );
				}
			}
		}

		m_connectedPoints.RemoveAtFast( index );
		return true;
	}

	return false;
}


void CWanderPointComponent::RemoveAllConnections()
{
	if ( GetConnectionsSize() == 0 )
		return;

	for( auto it = m_connectedPoints.Begin(), end = m_connectedPoints.End(); it != end; ++it )
	{
		CEntity* targetEnt = (*it).m_destination.Get();
		if ( targetEnt )
		{
			//delete the incoming connection on the target entity
			CWanderPointComponent* wpComponent = targetEnt->FindComponent<CWanderPointComponent>();
			
			if (wpComponent)
			{
				wpComponent->RemoveConnection( GetEntity(), false );
			}
		}
	}

	//this clears all outgoing connections to target entities
	m_connectedPoints.ClearFast();
}
