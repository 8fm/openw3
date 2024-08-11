/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "meshColoringScheme.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/rigidMeshComponent.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/layerInfo.h"

CMeshColoringSchemeCollisionType::CMeshColoringSchemeCollisionType()
{
	m_colorDisabled				= Vector( 0.6f, 0.6f, 0.6f, 0.0f ); // gray as we are not interested in disabled meshes
	m_colorStatic				= Vector( 1.0f, 0.0f, 0.0f, 0.5f ); // red
	m_colorStaticWalkable		= Vector( 0.0f, 1.0f, 0.0f, 0.5f ); // green
	m_colorStaticMetaobstacle	= Vector( 0.0f, 0.5f, 1.0f, 0.5f ); // who cares
	m_colorDynamic				= Vector( 0.5f, 0.0f, 1.0f, 0.5f ); // violet
	m_colorWalkable				= Vector( 0.0f, 0.3f, 0.7f, 0.5f ); // sea-like
	m_colorImmediate			= Vector( 1.0f, 0.0f, 1.0f, 0.5f ); // some else
}

Vector CMeshColoringSchemeCollisionType::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsA< CStaticMeshComponent >() )
	{
		const CStaticMeshComponent *staticMeshComponent = static_cast< const CStaticMeshComponent* >( drawComp );
		switch( staticMeshComponent->GetPathLibCollisionType() )
		{
		case PLC_Disabled:
			return m_colorDisabled;
		case PLC_Static:
			return m_colorStatic;
		case PLC_StaticWalkable:
			return m_colorStaticWalkable;
		case PLC_StaticMetaobstacle:
			return m_colorStaticMetaobstacle;
		case PLC_Dynamic:
			return m_colorDynamic;
		case PLC_Walkable:
			return m_colorWalkable;
		case PLC_Immediate:
			return m_colorImmediate;
		default:
			ASSERT( !TXT("Unknown EPathEngineCollision type") );
		}
	}

	return Vector( 0.0f, 0.0f, 0.0f, 0.0f );
}

void CMeshColoringSchemeCollisionType::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth  = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 10;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 80, upMargin + (8 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + 1*spaceWidth, TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + 3*spaceWidth, TXT("Disabled"),        Color( m_colorDisabled ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 5*spaceWidth, TXT("Static"),          Color( m_colorStatic ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 6*spaceWidth, TXT("Static walkable"), Color( m_colorStaticWalkable ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 6*spaceWidth, TXT("Static metaobstacle"), Color( m_colorStaticMetaobstacle ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 7*spaceWidth, TXT("Walkable"),        Color( m_colorWalkable ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 8*spaceWidth, TXT("Dynamic"),         Color( m_colorDynamic ) );
}

//////////////////////////////////////////////////////////////////////////


CMeshColoringSchemeEntityType::CMeshColoringSchemeEntityType()
{
	m_colorEntityTemplate	= Vector(1.0f,1.0f,0.0f,1.0f );
	m_colorMesh				= Vector(0.0f,0.0f,1.0f,1.0f );
	m_colorStaticMesh		= Vector(1.0f,0.0f,1.0f,1.0f );
	m_colorRigidMesh		= Vector(0.0f,1.0f,1.0f,1.0f );
	m_colorDestructionMesh	= Vector(0.0f,0.5f,0.5f,1.0f );
	m_colorClothMesh		= Vector(0.0f,0.25f,0.25f,1.0f );
}

Vector CMeshColoringSchemeEntityType::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsExactlyA< CStaticMeshComponent >() )
	{
		return m_colorStaticMesh;
	}

	if ( drawComp->IsExactlyA< CRigidMeshComponent >() )
	{
		return m_colorRigidMesh;
	}

	if ( drawComp->IsExactlyA< CDestructionSystemComponent >() )
	{
		return m_colorDestructionMesh;
	}

	if ( drawComp->IsExactlyA< CClothComponent >() )
	{
		return m_colorClothMesh;
	}

	if ( drawComp->IsExactlyA< CMeshComponent >() )
	{
		return m_colorMesh;
	}

	if ( drawComp->GetEntity() && drawComp->GetEntity()->GetEntityTemplate() )
	{
		return m_colorEntityTemplate;
	}

	return Vector( 0.0f, 0.0f, 0.0f, 0.0f );
}

void CMeshColoringSchemeEntityType::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 10;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 80, upMargin + (8 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + 1*spaceWidth, TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + 3*spaceWidth, TXT("Entity template"),     Color( m_colorEntityTemplate ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 4*spaceWidth, TXT("Mesh"),				Color( m_colorMesh ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 5*spaceWidth, TXT("Static mesh"),         Color( m_colorStaticMesh ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 6*spaceWidth, TXT("Rigid mesh"),			Color( m_colorRigidMesh ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 7*spaceWidth, TXT("Destruction mesh"),	Color( m_colorDestructionMesh ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 8*spaceWidth, TXT("Cloth mesh"),			Color( m_colorClothMesh ) );
}



//////////////////////////////////////////////////////////////////////////

CMeshColoringSchemeShadows::CMeshColoringSchemeShadows()
{
	m_colorCastShadows						= Vector( 1.0f, 0.0f, 0.0f, 1.0f ); // red
	m_colorNoShadows						= Vector( 0.0f, 1.0f, 0.0f, 1.0f ); // green
	m_colorShadowsFromLocalLightsOnly		= Vector( 0.0f, 0.0f, 1.0f, 1.0f ); // blue
}

Vector CMeshColoringSchemeShadows::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if( drawComp->IsCastingShadows() )
	{
		return m_colorCastShadows;
	}
	else if( drawComp->IsCastingShadowsFromLocalLightsOnly() )
	{
		return m_colorShadowsFromLocalLightsOnly;
	}
	else
	{
		return m_colorNoShadows;
	}
}

void CMeshColoringSchemeShadows::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth =  frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 10;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 80, upMargin + (4 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + 1*spaceWidth, TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + 3*spaceWidth, TXT("No shadows"),		Color( m_colorNoShadows ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 4*spaceWidth, TXT("Local lights shadows"),         Color( m_colorShadowsFromLocalLightsOnly) );
	frame->AddDebugScreenText( leftMargin, upMargin + 5*spaceWidth, TXT("Global + local shadows"),         Color( m_colorCastShadows ) );
}


//////////////////////////////////////////////////////////////////////////

CMeshColoringSchemeSoundMaterial::CMeshColoringSchemeSoundMaterial()
{
	m_colorNone         = Vector( 1.0f, 0.0f, 0.0f, 0.5f );    // red
	m_colorDirt         = Vector( 1.0f, 0.64f, 0.0f, 0.5f );   // orange
	m_colorStone        = Vector( 0.6f, 0.6f, 0.6f, 0.5f );    // gray
	m_colorWood         = Vector( 1.0f, 1.0f, 0.0f, 0.5f );    // yellow
	m_colorWatershallow = Vector( 0.72f, 0.012f, 1.0f, 0.5f ); // violet
	m_colorWaterdeep    = Vector( 0.5f, 0.0f, 0.5f, 0.5f );    // dark violet
	m_colorMud          = Vector( 0.76f, 0.69f, 0.5f, 0.5f );  // beige
	m_colorGrass        = Vector( 0.0f, 1.0f, 0.0f, 0.5f );    // green
	m_colorMetal        = Vector( 0.6f, 1.0f, 0.5f, 0.5f );    // custom
}

Vector CMeshColoringSchemeSoundMaterial::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
/*	if ( drawComp->IsA< CMeshComponent >() )
	{
		const CMeshComponent *meshComp = static_cast< const CMeshComponent* >( drawComp );
		CMesh *mesh = meshComp->GetMesh();
		if ( mesh )
		{
			switch ( mesh->GetSoundIdentification() )
			{
			case 0:
				return m_colorNone;
			case 1:
				return m_colorDirt;
			case 2:
				return m_colorStone;
			case 3:
				return m_colorWood;
			case 4:
				return m_colorWatershallow;
			case 5:
				return m_colorWaterdeep;
			case 6:
				return m_colorMetal;
			case 7:
				return m_colorMud;
			case 8:
				return m_colorGrass;
			}
		}
	}*/

	return Vector( 0.0f, 0.0f, 0.0f, 0.0f );
}

void CMeshColoringSchemeSoundMaterial::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth  = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 10;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 80, upMargin + (11 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + 1*spaceWidth, TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + 3*spaceWidth, TXT("None"),          Color( m_colorNone ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 4*spaceWidth, TXT("Dirt"),          Color( m_colorDirt ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 5*spaceWidth, TXT("Stone"),         Color( m_colorStone ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 6*spaceWidth, TXT("Wood"),          Color( m_colorWood ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 7*spaceWidth, TXT("Water shallow"), Color( m_colorWatershallow ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 8*spaceWidth, TXT("Water deep"),    Color( m_colorWaterdeep ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 9*spaceWidth, TXT("Metal"),         Color( m_colorMetal ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 10*spaceWidth, TXT("Mud"),           Color( m_colorMud ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 11*spaceWidth, TXT("Grass"),         Color( m_colorGrass ) );
}

//////////////////////////////////////////////////////////////////////////

#ifdef SOUND_OCCLUSSION_MESH_COLORING

CMeshColoringSchemeSoundOccl::CMeshColoringSchemeSoundOccl()
{
}

Vector CMeshColoringSchemeSoundOccl::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsA< CStaticMeshComponent >() )
	{
		const CStaticMeshComponent *meshComp = static_cast< const CStaticMeshComponent* >( drawComp );
		return Vector( 1.0f, 0.0f, 0.0f, meshComp->GetSoundOcclusionValue() );
	}

	return Vector( 0.0f, 0.0f, 0.0f, 0.0f );
}

void CMeshColoringSchemeSoundOccl::GenerateEditorFragments( CRenderFrame* frame )
{
}

#endif

///////////////////////////////////////////////////////////////////////////

CMeshColoringSchemeRendering::CMeshColoringSchemeRendering()
	: m_colorLOD0( Vector( 1.0f, 0.0f, 0.0f, 0.5f ) )
	, m_colorLOD1( Vector( 1.0f, 1.0f, 0.0f, 0.5f ) )
	, m_colorLOD2( Vector( 0.0f, 1.0f, 0.0f, 0.5f ) )
	, m_colorLOD3( Vector( 1.0f, 0.64f, 0.0f, 0.5f ) )
	, m_colorLOD4( Vector( 1.0f, 0.0f, 1.0f, 0.5f ) )
{
	m_refreshOnMove = true;
}

Vector CMeshColoringSchemeRendering::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsA< CMeshComponent >() )
	{
		const CMeshComponent *meshComp = static_cast< const CMeshComponent* >( drawComp );
		const CMesh* mesh = meshComp->GetMeshNow();
		if ( mesh )
		{
			Int32 forcedLod = meshComp->GetForcedLODLevel();
			Int32 visibleLod = 0;
			Vector cameraPosition = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
			Float distanceFromCameraSquared = meshComp->GetBoundingBox().SquaredDistance( cameraPosition );

			if ( forcedLod == -1 )
			{
				Int32 lods = static_cast<Int32>( mesh->GetNumLODLevels() );
				if ( lods < 2 )
				{
					return m_colorLOD0;
				}
				const CMesh::TLODLevelArray& lodLevels = mesh->GetMeshLODLevels();
				for ( Int32 i=lods - 1; i>=0; --i )
				{
					const CMesh::LODLevel& level = lodLevels[i];
					const Float distance = level.GetDistance();

					if ( !level.IsUsedOnPlatform() )
					{
						continue;
					}

					if ( distanceFromCameraSquared > distance*distance )
					{
						visibleLod = i;
						break;
					}
				}
			}
			else
			{
				visibleLod = forcedLod;
			}

			switch ( visibleLod )
			{
			case 0:
				return m_colorLOD0;
			case 1:
				return m_colorLOD1;
			case 2:
				return m_colorLOD2;
			case 3:
				return m_colorLOD3;
			default:
				return m_colorLOD4;
			}
		}
	}
	return m_colorLOD0;
}

void CMeshColoringSchemeRendering::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 10;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 80, upMargin + (11 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + 1*spaceWidth, TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + 3*spaceWidth, TXT("LOD 0 (First)"),	Color( m_colorLOD0 ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 4*spaceWidth, TXT("LOD 1"), Color( m_colorLOD1 ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 5*spaceWidth, TXT("LOD 2"), Color( m_colorLOD2 ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 6*spaceWidth, TXT("LOD 3"), Color( m_colorLOD3 ) );
	frame->AddDebugScreenText( leftMargin, upMargin + 7*spaceWidth, TXT("LOD 4"), Color( m_colorLOD4 ) );
}

///////////////////////////////////////////////////////////////////////////

CMeshColoringSchemeStreaming::CMeshColoringSchemeStreaming()
	: m_colorNone( Vector( 1.0f, 0.0f, 0.0f, 0.9f ) )
	, m_colorLOD0( Vector( 0.7f, 0.0f, 1.0f, 0.75f ) )
{
}

Vector CMeshColoringSchemeStreaming::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsA< CMeshComponent >() )
	{	
		const CMeshComponent *meshComp = static_cast< const CMeshComponent* >( drawComp );
		if ( !meshComp->GetEntity()->ShouldBeStreamed() )
		{
			return m_colorNone;
		}

		return meshComp->IsStreamed() ? m_colorLOD0 : m_colorNone;
	}
	return m_colorNone;
}

void CMeshColoringSchemeStreaming::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 50;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 210, upMargin + (11 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 1 * spaceWidth ), TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 3 * spaceWidth ), TXT("No Streaming"),	Color( m_colorNone ) );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 4 * spaceWidth ), TXT("Streamed"), Color( m_colorLOD0 ) );
}

//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

CMeshColoringSchemeChunks::CMeshColoringSchemeChunks()
	: m_colorMaxChunks( Vector( 1.0f, 0.0f, 0.0f, 1.0f ) )
	, m_colorMinChunks( Vector( 0.3f, 1.0f, 0.3f, 1.0f ) )
{
}

Vector CMeshColoringSchemeChunks::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	if ( drawComp->IsA< CMeshComponent >() )
	{	
		const CMeshComponent *meshComp = static_cast< const CMeshComponent* >( drawComp );
		CMesh* mesh = meshComp->GetMeshNow();

		if( mesh != nullptr ) 
		{
			const Float scaleRange = 20.0f;
			const Float clampedScale = Clamp<Float>( (Float)mesh->GetChunks().Size() / scaleRange, 0.0f, 1.0f );
			return Lerp<Vector>( clampedScale, m_colorMinChunks, m_colorMaxChunks );
		}
	}

	return m_colorMinChunks;
}

void CMeshColoringSchemeChunks::GenerateEditorFragments( CRenderFrame* frame )
{
	Uint32 frameWidth = frame->GetFrameOverlayInfo().m_width;
	Uint32 frameHeight = frame->GetFrameOverlayInfo().m_height;
	const Int32 spaceWidth = 15;
	const Int32 leftMargin = 11;
	const Int32 upMargin = 50;
	frame->AddDebugRect( leftMargin-2, upMargin-1, 210, upMargin + (11 * spaceWidth), Color( 0, 0, 0 )  );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 1 * spaceWidth ), TXT("Legend") );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 3 * spaceWidth ), TXT("1 chunk"),	Color( m_colorMinChunks ) );
	frame->AddDebugScreenText( leftMargin, upMargin + ( 4 * spaceWidth ), TXT("More than 20 chunks"), Color( m_colorMaxChunks ) );
}

//////////////////////////////////////////////////////////////////////////
// Static struct to calculate these once
static struct SLBTEnumInfo
{
	TDynArray< String > names;
	TDynArray< Color >	colors;
	Uint32				maxWidth;
	Bool				initialized;

	SLBTEnumInfo() : initialized( false ){}

	void Init()
	{
		// Initialization check
		if ( initialized )
		{
			return;
		}
		initialized = true;

		// Get all options for the enum
		CEnum* lbtEnum = SRTTI::GetInstance().FindEnum( CNAME( ELayerBuildTag ) );
		const TDynArray< CName >& options = lbtEnum->GetOptions();
		
		// Create names and calculate max width
		maxWidth = 0;
		for ( auto it=options.Begin(); it != options.End(); ++it )
		{
			names.PushBack( (*it).AsString().MidString( 4, 1000 ) ); // skip LBT_ part
			colors.PushBack( LayerBuildTagColors::GetColorFor( (ELayerBuildTag)( it - options.Begin() ) ) );
			maxWidth = Max( maxWidth, (*it).AsString().GetLength() );
		}
	}
} LBTEnumInfo;

CMeshColoringSchemeLayerBuildTag::CMeshColoringSchemeLayerBuildTag()
{
	LBTEnumInfo.Init();
}

Vector CMeshColoringSchemeLayerBuildTag::GetMeshSelectionColor( const CDrawableComponent* drawComp ) const
{
	CLayer* layer = drawComp->GetLayer();
	if ( layer != nullptr && layer->GetLayerInfo() != nullptr )
	{
		Vector color = LayerBuildTagColors::GetColorFor( layer->GetLayerInfo()->GetLayerBuildTag() ).ToVector();
		color.W = 0.5f;
		return color;
	}
	return Vector( 0.0f, 0.0f, 0.0f, 0.0f );
}

void CMeshColoringSchemeLayerBuildTag::GenerateEditorFragments( CRenderFrame* frame )
{
	frame->AddDebugRect( 10, 10, 14 + LBTEnumInfo.maxWidth*7, 14 + LBTEnumInfo.names.Size()*15, Color::BLACK );
	Int32 y = 30;
	for ( Uint32 i=0; i < LBTEnumInfo.names.Size(); ++i )
	{
		frame->AddDebugScreenText( 24, y, LBTEnumInfo.names[i], LBTEnumInfo.colors[i] );
		y += 15;
	}
}
