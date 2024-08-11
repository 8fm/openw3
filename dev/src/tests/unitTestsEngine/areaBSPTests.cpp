/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/areaBSP.h"
#include "../../common/core/math.h"

TEST( Polygon, planeBasedPolygonCreation )
{
	CAreaBSP::Polygon poly( Plane( Vector( 0, 0, 1.0f ), 0 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector( -1000.f, -1000.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector(  1000.f, -1000.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector(  1000.f,  1000.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector( -1000.f,  1000.f, 0, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 0, 0, 1.0f ), 2 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector( -1000.f, -1000.f, 2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector(  1000.f, -1000.f, 2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector(  1000.f,  1000.f, 2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector( -1000.f,  1000.f, 2, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 0, 0, -1.0f ), 2 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector(  1000.f, -1000.f, -2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector( -1000.f, -1000.f, -2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector( -1000.f,  1000.f, -2, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector(  1000.f,  1000.f, -2, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 0, 1.0f, 0 ), 0 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector( -1000.f, 0,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector(  1000.f, 0,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector(  1000.f, 0, -1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector( -1000.f, 0, -1000.f, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 0, -1.0f, 0 ), 2 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector(  1000.f, -2.0f,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector( -1000.f, -2.0f,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector( -1000.f, -2.0f, -1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector(  1000.f, -2.0f, -1000.f, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 1.0f, 0, 0 ), 2 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector( 2.0f, -1000.f,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector( 2.0f, -1000.f, -1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector( 2.0f,  1000.f, -1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector( 2.0f,  1000.f,  1000.f, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( -1.0f, 0, 0 ), 2 ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[0], Vector( -2.0f, -1000.f, -1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[1], Vector( -2.0f, -1000.f,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[2], Vector( -2.0f,  1000.f,  1000.f, 0 ) ) );
	EXPECT_TRUE( Vector::Equal3( poly.m_points[3], Vector( -2.0f,  1000.f, -1000.f, 0 ) ) );

	poly = CAreaBSP::Polygon( Plane( Vector( 0.707f, 0.707f, 0 ), 0 ) );
	EXPECT_TRUE( Vector::Near3( poly.m_points[0], Vector(  499.849f, -499.849f,  707.f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( poly.m_points[1], Vector(  499.849f, -499.849f, -707.f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( poly.m_points[2], Vector( -499.849f,  499.849f, -707.f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( poly.m_points[3], Vector( -499.849f,  499.849f,  707.f, 0 ) ) );
}

TEST( CAreaBSP, simpleConvexArea )
{
	// the vertices need to be specified CCW in order to receive a solid area inside
	TDynArray< Vector > vertices;
	vertices.PushBack( Vector( -1, -1, 0 ) );
	vertices.PushBack( Vector( 1, -1, 0 ) );
	vertices.PushBack( Vector( -1, 1, 0 ) );

	CAreaBSP bsp;
	bsp.Compile( vertices, 2.0f );

	EXPECT_TRUE( bsp.IsValid() );
	EXPECT_EQ( (Uint32)1, bsp.GetConvexes().Size() );

	// the area should have 6 vertices
	const TDynArray< Vector, MC_RenderData >& convexVertices = bsp.GetConvexes()[0].m_vertices;
	EXPECT_EQ( (Uint32)6, convexVertices.Size() );
	EXPECT_TRUE( Vector::Near3( convexVertices[0], Vector( -1.f,  1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( convexVertices[1], Vector(  1.f, -1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( convexVertices[2], Vector( -1.f, -1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( convexVertices[3], Vector( -1.f, -1.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( convexVertices[4], Vector(  1.f, -1.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( convexVertices[5], Vector( -1.f,  1.f, 2.0f, 0 ) ) );
}

TEST( CAreaBSP, simpleConcaveArea )
{
	// the vertices need to be specified CCW in order to receive a solid area inside
	TDynArray< Vector > vertices;
	vertices.PushBack( Vector( -1, -1, 0 ) );
	vertices.PushBack( Vector(  1, -1, 0 ) );
	vertices.PushBack( Vector(  0,  0, 0 ) );
	vertices.PushBack( Vector(  1,  1, 0 ) );
	vertices.PushBack( Vector( -1,  1, 0 ) );

	CAreaBSP bsp;
	bsp.Compile( vertices, 2.0f );

	EXPECT_EQ( (Uint32)2, bsp.GetConvexes().Size() );

	// the first area should have 6 vertices
	const TDynArray< Vector, MC_RenderData >& firstAreaVtcs = bsp.GetConvexes()[0].m_vertices;
	EXPECT_EQ( (Uint32)6, firstAreaVtcs.Size() );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[0], Vector(  1.f,  1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[1], Vector(  0.f,  0.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[2], Vector( -1.f,  1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[3], Vector( -1.f,  1.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[4], Vector(  0.f,  0.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( firstAreaVtcs[5], Vector(  1.f,  1.f, 2.0f, 0 ) ) );

	// the first area should also have 6 vertices
	const TDynArray< Vector, MC_RenderData >& secondAreaVtcs = bsp.GetConvexes()[1].m_vertices;
	EXPECT_EQ( (Uint32)6, secondAreaVtcs.Size() );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[0], Vector( -1.f,  1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[1], Vector(  1.f, -1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[2], Vector( -1.f, -1.f, 0, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[3], Vector( -1.f, -1.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[4], Vector(  1.f, -1.f, 2.0f, 0 ) ) );
	EXPECT_TRUE( Vector::Near3( secondAreaVtcs[5], Vector( -1.f,  1.f, 2.0f, 0 ) ) );
}

