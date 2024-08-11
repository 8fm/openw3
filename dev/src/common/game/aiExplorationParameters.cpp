/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiExplorationParameters.h"


IMPLEMENT_ENGINE_CLASS( IAIExplorationTree )

void IAIExplorationTree::Setup( const Vector3& interactionPoint, const Vector3& destinationPoint, CComponent* metalinkComponent )
{
	m_interactionPoint = interactionPoint;
	m_destinationPoint = destinationPoint;
	m_metalinkComponent = metalinkComponent;
}

CName IAIExplorationTree::GetInteractionPointParamName()
{
	return CNAME( interactionPoint );
}
CName IAIExplorationTree::GetDestinationPointParamName()
{
	return CNAME( destinationPoint );
}
CName IAIExplorationTree::GetMetalinkParamName()
{
	return CNAME( metalinkComponent );
}