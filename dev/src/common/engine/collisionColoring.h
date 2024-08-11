/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "..\physics\physicsEngine.h"

/// False coloring for collision triangles
class CollisionFalseColoring
{
private:
	Vector		m_lightVector;
	Vector		m_color;

public:
	CollisionFalseColoring()
		: m_color( 1.0f, 1.0f, 1.0f, 1.0f )
	{
		m_lightVector = Vector( 1,1,1,0 ).Normalized3();
	}

	CollisionFalseColoring( const Vector& baseColor )
		: m_color( baseColor )
	{
		m_lightVector = Vector( 1,1,1,0 ).Normalized3();
	}

	Uint32 CalcColor( const Vector& a, const Vector& b, const Vector& c )
	{
		// Calculate lighting
		const Vector triNormal = Vector::Cross( ( b-a ).Normalized3(), ( c-a ).Normalized3(), 0.0f ).Normalized3();
		//const Float diffuse = 0.5f + 0.5f * Vector::Dot3( m_lightVector, triNormal );

		// Calculate color
		//Vector color = m_color * ( 0.05f + 0.2f * diffuse );
		Vector color = ( triNormal * 0.3f ) + 0.5f;

		// Assemble final color
		Color finalColor( color );
		finalColor.A = 255;
		return finalColor.ToUint32();
	}

	Uint32 MaterialColor( const CName& materialName )
	{
		const SPhysicalMaterial* mtl = GPhysicEngine->GetMaterial( materialName );
		if ( !mtl )
		{
			mtl = GPhysicEngine->GetMaterial( CNAME( default ) );
		}

		Color color( 0, 0, 0 );

#ifndef NO_EDITOR
		if ( mtl )
		{
			color = mtl->m_debugColor;
		}
#endif
		color.A = 160;
		return color.ToUint32();
	}
};

