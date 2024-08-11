#pragma once

#include "pathlib.h"
#include "pathlibNavmesh.h"
#include "areaComponent.h"
#include "attachment.h"

class CNavmesh;
#ifndef NO_NAVMESH_GENERATION
namespace Recast
{
	class CNavmeshGenerationJob;
	struct CGenerationInputData;
	class CGenerator;
};

#endif



namespace PathLib
{
	class CNavmeshRenderer;
	class CNavmesh;
};

struct SNavmeshParams
{
	DECLARE_RTTI_STRUCT( SNavmeshParams );

public:
	Bool m_useGenerationRootPoints;
	Bool m_useTerrainInGeneration;
	Bool m_useStaticMeshesInGeneration;
	Bool m_collectFoliage;
	Bool m_previewOriginalGeometry;
	Bool m_useCollisionMeshes;
	Bool m_monotonePartitioning;
	Bool m_detectTerrainConnection;
	Bool m_stepOnNonWalkableMeshes;
	Bool m_cutMeshesWithBoundings;
	Bool m_smoothWalkableAreas;
	Float m_extensionLength;
	Float m_cellWidth;
	Float m_cellHeight;
	Float m_walkableSlopeAngle;
	Float m_agentHeight;
	Float m_agentClimb;
	Float m_margin;
	Float m_maxEdgeLen;
	Float m_maxEdgeError;
	Float m_regionMinSize;
	Float m_regionMergeSize;
	Uint32 m_vertsPerPoly;
	Float m_detailSampleDist;
	Float m_detailSampleMaxError;
	Float m_extraStreamingRange;

	SNavmeshParams()
		: m_useGenerationRootPoints( true )
		, m_useTerrainInGeneration( false )
		, m_useStaticMeshesInGeneration( true )
		, m_collectFoliage( true )
		, m_previewOriginalGeometry( false )
		, m_useCollisionMeshes( true )
		, m_monotonePartitioning( false )
		, m_detectTerrainConnection( true )
		, m_stepOnNonWalkableMeshes( false )
		, m_cutMeshesWithBoundings( false )
		, m_smoothWalkableAreas( false )
		, m_extensionLength( 2.f )
		, m_cellWidth( 0.1f )
		, m_cellHeight( 0.1f )
		, m_walkableSlopeAngle( 52.5f )
		, m_agentHeight( 1.8f )
		, m_margin( 0.1f )
		, m_agentClimb( 0.5f )
		, m_maxEdgeLen( 20.0f )
		, m_maxEdgeError( 1.3f )
		, m_regionMinSize( 13.0f )
		, m_regionMergeSize( 20.0f )
		, m_vertsPerPoly( 8 )
		, m_detailSampleDist( 6.0f )
		, m_detailSampleMaxError( 4.0f )
		, m_extraStreamingRange( 5.f )
	{

	}

	void ResetToDefaults();
};

BEGIN_CLASS_RTTI( SNavmeshParams );
	PROPERTY_EDIT( m_useGenerationRootPoints, TXT("Collect and use navmesh root points in generation process") )
	PROPERTY_EDIT( m_useTerrainInGeneration, TXT("Collect terrain geometry for generation") )
	PROPERTY_EDIT( m_useStaticMeshesInGeneration, TXT("Collect static meshes for generation. U rly want to change this setting?") )
	PROPERTY_EDIT( m_collectFoliage, TXT("Collect vegetation as static obstacles") )
	PROPERTY_EDIT( m_previewOriginalGeometry, TXT("Preview original geometry triangles or generate new ones for the nav mesh") )
	PROPERTY_EDIT( m_useCollisionMeshes, TXT("Use collision meshes or standard geometry for generating the navmesh") )
	PROPERTY_EDIT( m_monotonePartitioning, TXT("Monotone partitioning without holes OR employ distance field and produce more complex navmesh") )
	PROPERTY_EDIT( m_detectTerrainConnection, TXT("Decect area connection based on extended terrain geometry") )
	PROPERTY_EDIT( m_stepOnNonWalkableMeshes, TXT("Enable stepping on non-walkable meshes if they are agentClimb low.") )
	PROPERTY_EDIT( m_cutMeshesWithBoundings, TXT("Cut collected meshes using boundings provided and detect their external connectivity.") )
	PROPERTY_EDIT( m_smoothWalkableAreas, TXT("Apply median filter algorithm that smooth out walkable areas.") )
	PROPERTY_EDIT( m_extensionLength, TXT("Radius of extension to generated region. Extended region is used for auto connection generation.") )
	PROPERTY_EDIT( m_cellWidth, TXT("Navmesh tile width") );
	PROPERTY_EDIT( m_cellHeight, TXT("Navmesh tile height") );
	PROPERTY_EDIT( m_walkableSlopeAngle, TXT("Navmesh walkable slope angle") );
	PROPERTY_EDIT( m_agentHeight, TXT("Agent walkable height") );
	PROPERTY_EDIT( m_agentClimb, TXT("Agent walkable climb") );
	PROPERTY_EDIT( m_margin, TXT("Agent walkable radius") );
	PROPERTY_EDIT( m_maxEdgeLen, TXT("Navmesh edge max length") );
	PROPERTY_EDIT( m_maxEdgeError, TXT("Navmesh maximum edge error") );
	PROPERTY_EDIT( m_regionMinSize, TXT("Navmesh minimum region size") );
	PROPERTY_EDIT( m_regionMergeSize, TXT("Navmesh region merge size") );
	PROPERTY_EDIT_RANGE( m_vertsPerPoly, TXT("Navmesh verticles per polygon"), 3, 32 );
	PROPERTY_EDIT( m_detailSampleDist, TXT("Sample detail distance") );
	PROPERTY_EDIT( m_detailSampleMaxError, TXT("Sample detail max error tolerance") );
	PROPERTY_EDIT( m_extraStreamingRange, TXT("Extra range for streaming") );
END_CLASS_RTTI();



class CNavmeshComponent : public CAreaComponent
{
#ifndef NO_NAVMESH_GENERATION
	friend struct Recast::CGenerationInputData;
#endif
	DECLARE_ENGINE_CLASS( CNavmeshComponent, CAreaComponent, 0 );
protected:
	PathLib::CNavmesh*					m_navmesh;
	SNavmeshParams						m_navmeshParams;
	PathLib::AreaId						m_pathlibAreaId;
	String								m_sharedFileName;
#ifndef NO_NAVMESH_GENERATION
	Recast::CNavmeshGenerationJob*		m_generationJob;
#endif

	TDynArray< Vector >					m_generationRootPoints;

#ifndef NO_NAVMESH_GENERATION
	void				NavmeshAsyncGenerationCompleted();
	void				PostNavmeshInitialization( Bool save );

	Bool				SetGenerationJob( Recast::CNavmeshGenerationJob* job );
	void				ClearGenerationJob();
#endif

public:
	CNavmeshComponent()
		: m_navmesh( NULL )
		, m_pathlibAreaId( PathLib::INVALID_AREA_ID )
	{}

	~CNavmeshComponent();

	////////////////////////////////////////////////////////////////////////
	// component interface
	Bool				IsManualCreationAllowed() const override;
	void				OnAttached( CWorld* world ) override;
	void				OnDetached( CWorld* world ) override;
	void				ComputeNavmeshBasedBounds();
#ifndef NO_EDITOR_FRAGMENTS
	Bool				OnPropertyMissing( CName propertyName, const CVariant& readValue ) override;
	void				OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag ) override;
#endif

	////////////////////////////////////////////////////////////////////////
	// special interface
	Bool				Initialize( const TDynArray< Vector3 >& vertexes, const TDynArray< Uint16 >& triangleVertexes, Bool useTransformation, Bool save );
	PathLib::CNavmesh*	GetNavmesh() const									{ return m_navmesh; }
	void				ClearNavmesh();
	void				SetPathLibAreaId( PathLib::AreaId id )				{ m_pathlibAreaId = id; }
	PathLib::AreaId		GetPathLibAreaId() const							{ return m_pathlibAreaId; }
	Bool				IsNavmeshUsingTransformation() const				{ return false; }
	Bool				IsStoredWithWorldNavdata() const					{ return true; }
	const String&		GetSharedFilePath() const							{ return m_sharedFileName; }
	Bool				IsUsingGenerationRootPoints() const					{ return m_navmeshParams.m_useGenerationRootPoints; }
	const CGUID&		GetGUID4PathLib() const;
	TDynArray< Vector >& GetGenerationRoots()								{ return m_generationRootPoints; }
	Uint32				GetGenerationRootsCount() const						{ return m_generationRootPoints.Size(); }
	Vector				GetGenerationRootWorldPosition( Uint32 i ) const;
	static void			GenericFileName( PathLib::AreaId areaId, String& outFilename );

#ifndef NO_NAVMESH_GENERATION
	////////////////////////////////////////////////////////////////////////
	// navmesh generation
	struct InputIterator
	{
		virtual Bool Reset();												// return true if iterator supports reset operation
		virtual void Next() = 0;											// ++it
		virtual CNode* Get() = 0;											// returns null on end of iterator
		virtual ~InputIterator()											{}
	};

	Bool				GenerateNavmesh( InputIterator* input, Bool transformableNavmesh );
	Bool				GenerateNavmeshAsync();
	Bool				GenerateNavmeshRecursiveAsync();
	void				GenerateNavgraph();
	Bool				IsNavmeshGenerationRunning() const					{ return m_generationJob != NULL; }

	SNavmeshParams&		GetNavmeshParams()									{ return m_navmeshParams; }
	const SNavmeshParams& GetNavmeshParams() const							{ return m_navmeshParams; }

	InputIterator*		AreaBasedInputGenerator();

	void				OnTick( float timeDelta ) override;

	void				ReportErrors();
	Bool				IsRunningRecursiveGeneration() const;
	void				StopRecursiveGeneration();

#ifndef NO_EDITOR
	void				EditorPreDeletion() override;

	Bool				RemoveOnCookedBuild() override;
#endif


#else
	RED_INLINE Bool	IsNavmeshGenerationRunning() const					{ return false; }
#endif
};


BEGIN_CLASS_RTTI( CNavmeshComponent );
	PARENT_CLASS( CAreaComponent );
	PROPERTY_EDIT( m_navmeshParams, TXT("Navmesh generation parameters") );
	PROPERTY( m_pathlibAreaId );
	PROPERTY( m_sharedFileName );
	PROPERTY_EDIT( m_generationRootPoints, TXT("Component 'internal' root points (positions in local space)") );
END_CLASS_RTTI();

class CNavmeshGenerationRootComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CNavmeshGenerationRootComponent, CComponent, 0 );
public:
	CNavmeshGenerationRootComponent()										{}
	~CNavmeshGenerationRootComponent()										{}
};

BEGIN_CLASS_RTTI( CNavmeshGenerationRootComponent );
	PARENT_CLASS( CComponent );
END_CLASS_RTTI();

class CNavmeshInputAttachment : public IAttachment
{
	DECLARE_ENGINE_CLASS( CNavmeshInputAttachment, IAttachment, 0 );
public:
	Bool Init( CNode* parent, CNode* child, const AttachmentSpawnInfo* info ) override;
};

BEGIN_CLASS_RTTI( CNavmeshInputAttachment )
	PARENT_CLASS( IAttachment );
END_CLASS_RTTI();
