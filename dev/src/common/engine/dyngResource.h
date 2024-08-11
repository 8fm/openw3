
#pragma once

#include "../core/resource.h"

class CDyngResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CDyngResource, CResource, "w3dyng", "Dyng" );	

	String					m_name;
	//TDynArray< Int32 >		m_temporary;
public:
	THandle< CSkeleton >	m_dyngSkeleton;

	TDynArray< String >		m_nodeNames;
	TDynArray< String >		m_nodeParents;
	TDynArray< Float >		m_nodeMasses;
	TDynArray< Float >		m_nodeStifnesses;
	TDynArray< Float >		m_nodeDistances;
	TDynArray< Matrix >		m_nodeTransforms;

	TDynArray< Int32 >		m_linkTypes;
	TDynArray< Float >		m_linkLengths;
	TDynArray< Int32 >		m_linkAs;
	TDynArray< Int32 >		m_linkBs;

	TDynArray< Int32 >		m_triangleAs;
	TDynArray< Int32 >		m_triangleBs;
	TDynArray< Int32 >		m_triangleCs;

	TDynArray< String >		m_collisionParents;
	TDynArray< Float >		m_collisionRadiuses;
	TDynArray< Float >		m_collisionHeights;
	TDynArray< Matrix >		m_collisionTransforms;

#ifndef NO_RESOURCE_IMPORT

public:
	struct FactoryInfo : public CResource::FactoryInfo< CDyngResource >
	{				
		String					m_name;
		//TDynArray< Int32 >		m_temporaryData;

		TDynArray< String >		m_nodeNamesData;
		TDynArray< String >		m_nodeParentsData;
		TDynArray< Float >		m_nodeMassesData;
		TDynArray< Float >		m_nodeStifnessesData;
		TDynArray< Float >		m_nodeDistancesData;
		TDynArray< Matrix >		m_nodeTransformsData;

		TDynArray< Int32 >		m_linkTypesData;
		TDynArray< Float >		m_linkLengthsData;
		TDynArray< Int32 >		m_linkAsData;
		TDynArray< Int32 >		m_linkBsData;

		TDynArray< Int32 >		m_triangleAsData;
		TDynArray< Int32 >		m_triangleBsData;
		TDynArray< Int32 >		m_triangleCsData;

		TDynArray< String >		m_collisionParentsData;
		TDynArray< Float >		m_collisionRadiusesData;
		TDynArray< Float >		m_collisionHeightsData;
		TDynArray< Matrix >		m_collisionTransformsData;
	};

	static CDyngResource* Create( const FactoryInfo& data );

#endif

public:
	void CreateSkeleton();
	void RecreateSkeleton();

	virtual void OnSerialize( IFile& file );
	virtual void OnPropertyPostChange( IProperty* property );
};

BEGIN_CLASS_RTTI( CDyngResource );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_name,					TXT("Name") );

	PROPERTY_RO( m_dyngSkeleton,			TXT("Dyng skeleton") );

	PROPERTY_RO( m_nodeNames,				TXT("NodeNames") );
	PROPERTY_RO( m_nodeParents,				TXT("NodeParents") );
	PROPERTY_RO( m_nodeMasses,				TXT("NodeMasses") );
	PROPERTY_RO( m_nodeStifnesses,			TXT("NodeStifnesses") );
	PROPERTY_RO( m_nodeDistances,			TXT("NodeDistances") );
	PROPERTY_RO( m_nodeTransforms,			TXT("NodeTransforms") );

	PROPERTY_RO( m_linkTypes,				TXT("LinkTypes") );
	PROPERTY_RO( m_linkLengths,				TXT("LinkLengths") );
	PROPERTY_RO( m_linkAs,					TXT("LinksA") );
	PROPERTY_RO( m_linkBs,					TXT("LinksB") );

	PROPERTY_RO( m_triangleAs,				TXT("TrianglesA") );
	PROPERTY_RO( m_triangleBs,				TXT("TrianglesB") );
	PROPERTY_RO( m_triangleCs,				TXT("TrianglesC") );

	PROPERTY_RO( m_collisionParents,		TXT("CollisionsParents") );
	PROPERTY_RO( m_collisionRadiuses,		TXT("CollisionsRadiuses") );
	PROPERTY_RO( m_collisionHeights,		TXT("CollisionsHeights") );
	PROPERTY_RO( m_collisionTransforms,		TXT("CollisionsTransforms") );
END_CLASS_RTTI();
