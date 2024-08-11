/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiParameters.h"

class IAIExplorationTree : public IAITree
{
	typedef IAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAIExplorationTree );
protected:
	Vector3							m_interactionPoint;
	Vector3							m_destinationPoint;
	THandle< CComponent >			m_metalinkComponent;
public:
	IAIExplorationTree()
		: m_interactionPoint( 0.f, 0.f, 0.f )
		, m_destinationPoint( 0.f, 0.f, 0.f )												{}

	void Setup( const Vector3& interactionPoint, const Vector3& destinationPoint, CComponent* metalinkComponent );

	static CName					GetInteractionPointParamName();
	static CName					GetDestinationPointParamName();
	static CName					GetMetalinkParamName();
};


BEGIN_CLASS_RTTI( IAIExplorationTree )
	PARENT_CLASS( IAITree )
	PROPERTY_EDIT( m_interactionPoint, TXT("Internal. World position of desired exploration starting position." ) )
	PROPERTY_EDIT( m_destinationPoint, TXT("Internal. World position of desired exploration destination position." ) )
	PROPERTY_EDIT( m_metalinkComponent, TXT("Internal. Pointer to metalink component.") )
END_CLASS_RTTI()