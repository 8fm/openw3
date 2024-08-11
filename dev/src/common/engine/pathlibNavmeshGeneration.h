/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "areaComponent.h"
#include "pathlibNavmeshComponent.h"
#include "pathlibNavmesh.h"
#include "staticMeshComponent.h"

#ifndef NO_NAVMESH_GENERATION

struct Vertex;
class CStaticMeshComponent;
class CFoliageInstance;

namespace Recast
{

enum EGenerationTask
{
	ENavGen_Initialize,
	ENavGen_MarkTriangles,
	ENavGen_ProcessHeightfield,
	ENavGen_BuildRegions,
	ENavGen_BuildContourSet,
	ENavGen_BuildPolyMesh,
	ENavGen_BuildPolyMeshDetail,
	ENavGen_ConsistencyProcessing,
	ENavGen_PrepareExport,
	ENavGen_ExportGeometry
};

struct CGenerationInputData : public Red::System::NonCopyable
{
	CGenerationInputData( CNavmeshComponent::InputIterator* inputIterator, CNavmeshComponent* component );
	~CGenerationInputData();


	CNavmeshComponent::InputIterator* StealInputIterator()				{ CNavmeshComponent::InputIterator*	inputIterator = m_inputIterator; m_inputIterator = NULL; return inputIterator; }

	typedef CAreaComponent::TAreaPoints TAreaPoints;

	const SNavmeshParams&					m_params;
	const Box&								m_bbox;
	const TAreaPoints&						m_boundingPoly;
	const TDynArray< Vector >&				m_defaultGenerationRoots;
	const Matrix&							m_localToWorld;
	CNavmeshComponent::InputIterator*		m_inputIterator;
	CNavmeshComponent*						m_navmeshComponent;
};

struct CStreamingOverride : public Red::System::NonCopyable
{
protected:
	TDynArray< CEntity* >					m_forceStreamedEntities;
public:
	CStreamingOverride( const Box& bbox );
	~CStreamingOverride();
};


class CGenerationProcessingData
{
public:
	struct ConnectorEdge
	{
		ConnectorEdge()													{}
		ConnectorEdge( Uint16 indBegin, Uint16 indEnd )
			: m_indBegin( indBegin )
			, m_indEnd( indEnd )										{}
		Uint16								m_indBegin;
		Uint16								m_indEnd;
	};
	//struct ConnectorEdge 
	//{
	//	ConnectorEdge()													{}
	//	ConnectorEdge( Uint16 triIndex, Uint16 edge )
	//		: m_triIndex( triIndex )
	//		, m_edge( edge )											{}
	//	Uint16								m_triIndex;
	//	Uint16								m_edge;
	//	Bool operator<( const ConnectorEdge& e ) const					{ return m_triIndex < e.m_triIndex ? true : m_triIndex > e.m_triIndex ? false : m_edge < e.m_edge; }
	//};

	SNavmeshParams							m_params;
	TDynArray< Float >						m_vertsInput;
	TDynArray< Int32 >						m_trisInput;
	TDynArray< Float >						m_unwalkableVertsInput;
	TDynArray< Int32 >						m_unwalkableTrisInput;
	TDynArray< Vector3 >					m_generationRoots;

	TDynArray< Float >						m_vertsExtensionInput;
	TDynArray< Int32 >						m_trisExtensionInput;

	TDynArray< Vector3 >					m_vertsOutput;
	TDynArray< Uint16 >						m_trisOutput;

	TDynArray< ConnectorEdge >				m_connectorEdges;

	static Vector3 ToRecastPosition( const Vector3& redPos )			{ return Vector3( redPos.X, redPos.Z, redPos.Y ); }
public:
	CGenerationProcessingData( CGenerationInputData& input );

#ifdef DEBUG_NAVMESH_COLORS
	TDynArray< Uint32 >						m_triangleColours;
#endif
};

class CGenerator : public CGenerationProcessingData
{
protected:
	volatile EGenerationTask				m_currentGenerationTask;
	TDynArray< PathLib::SNavmeshProblem >	m_problems;

	static const Float GENROOT_AND_NAVMESH_MAX_DISTANCE;

	Bool VerifyInput( CGenerationInputData& input );

	Bool CollectMeshes( CGenerationInputData& input, TDynArray< CStaticMeshComponent* >& outMeshes, TDynArray< CDeniedAreaComponent* >& outDeniedAreas, TDynArray< CNavmeshBorderAreaComponent* >& outBorderAreas );
	Bool CollectShapeGeometry( CGenerationInputData& input, const TDynArray< Vector >& vertices, const TDynArray< Uint32 >& indices, Bool walkable, Bool detectExtension );
	Bool CollectMeshGeometry( CGenerationInputData& input, const TDynArray< CStaticMeshComponent* >& meshes, const TDynArray< CDeniedAreaComponent* >& deniedAreas );
	Bool CollectTerrainGeometry( CGenerationInputData& input, const Box& bbox, const Vector* boundingPoly, Uint32 boundingPolySize, TDynArray< Float >& vertsInput, TDynArray< Int32 >& trisInput );
	Bool CollectTerrainGeometry( CGenerationInputData& input, Bool collectForExtension );
	Bool CollectFoliageGeometry( CGenerationInputData& input, const SFoliageInstance & foliageInstance, const TDynArray< Sphere >& foliageShape );
	Bool CollectFoliageGeometry( CGenerationInputData& input );
	Bool ApplyBorderAreas( CGenerationInputData& input, const TDynArray< CNavmeshBorderAreaComponent* >& borderAreas );
	Bool CollectGeometry( CGenerationInputData& input );

	void NoticeErrorInternal( PathLib::SNavmeshProblem&& p );
	void NoticeError( String&& error )										{ NoticeErrorInternal( Move( PathLib::SNavmeshProblem( Move( error ) ) ) ); } 
	void NoticeError( String&& error, const Vector3& v )					{ NoticeErrorInternal( Move( PathLib::SNavmeshProblem( Move( error ), v ) ) ); }

public:
	CGenerator( CGenerationInputData& input );

	// synchronous processing
	Bool Initialize( CGenerationInputData& input );

	// possibly asynchronous processing
	Bool Generate();

	void ReportErrors();
	const TDynArray< PathLib::SNavmeshProblem >& GetProblems() const		{ return m_problems; }
	Bool HadProblems() const												{ return !m_problems.Empty(); }
	EGenerationTask GetCurrentTaskDescription() const volatile				{ return m_currentGenerationTask; }

	static Bool CollectComponent( EPathLibCollision collisionFlag )			{ return collisionFlag == PLC_Static || collisionFlag == PLC_StaticWalkable || collisionFlag == PLC_Walkable; }
	static Bool ComponentIsWalkable( EPathLibCollision collisionFlag )		{ return collisionFlag == PLC_StaticWalkable || collisionFlag == PLC_Walkable; }
	static Bool CollectDeniedArea( CDeniedAreaComponent* da );
	static Bool DeniedAreaMeshIsWalkable( CDeniedAreaComponent* da );
	static Bool CollectStaticMesh( CStaticMeshComponent* staticMesh );
	static Bool StaticMeshIsWalkable( CStaticMeshComponent* staticMesh )	{ return ComponentIsWalkable( staticMesh->GetPathLibCollisionType() ); }
};

class CNavmeshGenerationJob : public CTask
{
protected:
	CGenerator*								m_generator;

	volatile Bool							m_isSuccess;
	PathLib::CNavmesh*						m_navmesh;

	CNavmeshGenerationJob( CGenerationInputData& inputData );
	Bool Init();

public:
	~CNavmeshGenerationJob();

	static CNavmeshGenerationJob* CreateGenerationJob( CGenerationInputData& inputData );

	void Run() override;

	virtual void StopRecursiveGeneration() volatile;
	virtual Bool IsRunningRecursiveGeneration() const volatile;

	Bool IsSuccess() const volatile											{ return m_isSuccess; }
	EGenerationTask GetCurrentTaskDescription() const volatile				{ return m_generator->GetCurrentTaskDescription(); }
	PathLib::CNavmesh* GetOutputNavmesh() const								{ return m_navmesh; }
	PathLib::CNavmesh* StealOutputNavmesh()									{ PathLib::CNavmesh* navi = m_navmesh; m_navmesh = NULL; return navi; }
	const CGenerator& GetGenerator() const									{ return *m_generator; }

#ifndef NO_DEBUG_PAGES
	const Char* GetDebugName() const override;
	Uint32 GetDebugColor() const override;
#endif
};

class CNavmeshRecursiveGenerationJob : public CNavmeshGenerationJob
{
protected:
	volatile Bool							m_isRunningRecursiveGeneration;
	CNavmeshComponent::InputIterator*		m_inputIterator;

	CNavmeshRecursiveGenerationJob( CGenerationInputData& inputData );
	~CNavmeshRecursiveGenerationJob();
public:
	static CNavmeshRecursiveGenerationJob* CreateGenerationJob( CGenerationInputData& inputData );

	void StopRecursiveGeneration() volatile override;
	Bool IsRunningRecursiveGeneration() const volatile override;

	void StoreInputIterator( CGenerationInputData& inputData )		{ m_inputIterator = inputData.StealInputIterator(); }
	CNavmeshComponent::InputIterator* StealInputIterator()			{ CNavmeshComponent::InputIterator* it = m_inputIterator; m_inputIterator = NULL; return it; }
};

};				// namespace Recast

#endif