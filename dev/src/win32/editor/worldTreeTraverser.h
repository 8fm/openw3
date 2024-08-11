/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace CollisionMem
{
	//** *******************************
	//* World structure
	//*
	//* LayerGroup
	//*   LayerGroup
	//*   LayerInfo
	//*		Entity
	//*		  Component
	class CWorldTreeTraverser
	{
	private:

		TMeshMap	m_MeshMap;

	public:

		void	TraverseWorldTree	( CWorld * pWorld, class CCollisionMemToolModel * pModel );

	private:

		void	ProcessNode			( CLayerGroup * pNode, class CCollisionMemToolModel * pModel );
		void	ProcessNode			( CLayerInfo * pNode, class CCollisionMemToolModel * pModel );
	};
}