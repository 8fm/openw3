#include "build.h"
#include "meshTypeResource.h"
#include "drawableComponent.h"
#include "renderResource.h"
#include "material.h"
#include "../core/variant.h"
#include "../core/dataError.h"

IMPLEMENT_SIMPLE_RTTI_TYPE( SMeshTypeResourceLODLevel );
IMPLEMENT_ENGINE_CLASS( CMeshTypeResource );

CMeshTypeResource::CMeshTypeResource()
	: m_renderResource( nullptr )
	, m_autoHideDistance( 20.0f )
	, m_skeletonMappingCache( this )
{
}

CMeshTypeResource::~CMeshTypeResource()
{
	ReleaseRenderResource();
}

Bool CMeshTypeResource::IsRenderingReady() const
{
	if ( IsLoading() )
		return false;

	if ( m_renderResource && !m_renderResource->IsFullyLoaded() )
		return false;

	return true;
}

void CMeshTypeResource::OnPostLoad()
{
#ifndef NO_EDITOR
	for ( Uint32 i = 0; i < m_materials.Size(); ++i )
	{
		if ( ! m_materials[i].IsValid() )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Null material in material list for chunk %ls"), m_materialNames[i].AsChar() );
		}
		else
		{
			if ( m_materials[i]->GetBaseMaterial() == nullptr && m_materialNames.Size() > 0 && m_materialNames[i] != TXT( "volume" ) )
			{
				DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Chunk '%ls' uses a material instance that has a missing base material"), m_materialNames[i].AsChar() );
			}
		}
	}

	const Uint32 numLods = GetNumLODLevels();
	for ( Uint32 i = 0; i < numLods; ++i )
	{
		const SMeshTypeResourceLODLevel& lod = GetLODLevel( i );
		if ( lod.GetDistance() >= m_autoHideDistance )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("LOD '%u' has distance set to %f, which is farther than auto-hide (%f). This LOD may not show up."), i, lod.GetDistance(), m_autoHideDistance );
		}
	}
#endif

	TBaseClass::OnPostLoad();
}

void CMeshTypeResource::OnPreSave()
{
	TBaseClass::OnPreSave();

#ifndef NO_EDITOR
	const Uint32 numLods = GetNumLODLevels();
	for ( Uint32 i = 0; i < numLods; ++i )
	{
		const SMeshTypeResourceLODLevel& lod = GetLODLevel( i );
		if ( lod.GetDistance() >= m_autoHideDistance )
		{
			DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("LOD '%u' has distance set to %f, which is farther than auto-hide (%f). This LOD may not show up."), i, lod.GetDistance(), m_autoHideDistance );
		}
	}
#endif
}


void CMeshTypeResource::OnAllHandlesReleased()
{
	ReleaseRenderResource();

	TBaseClass::OnAllHandlesReleased();
}

Bool CMeshTypeResource::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
	if ( propertyName == TXT("materials") )
	{
		TDynArray< IMaterial* > materials;
		TMaterials materialsH;
		if ( readValue.AsType< TMaterials >( materialsH ) )
		{
			m_materials = materialsH;
		}
		else if ( readValue.AsType< TDynArray< IMaterial* > >( materials ) )
		{
			m_materials.Resize( materials.Size() );
			for ( Uint32 i=0; i<materials.Size(); ++i )
			{
				m_materials[i] = materials[i];
			}
		}

		return true;
	}
#ifndef NO_RESOURCE_IMPORT
	else if ( propertyName == TXT("materialNames") )
	{
		readValue.AsType< TDynArray< String > >( m_materialNames );
		return true;
	}
#endif
	else if ( propertyName == TXT("boundingBox") )
	{
		readValue.AsType< Box >( m_boundingBox );
		return true;
	}
	else if ( propertyName == TXT("autoHideDistance") )
	{
		readValue.AsType< Float >( m_autoHideDistance );
		return true;
	}
	else if ( propertyName == TXT("shadowRenderingDistance") )
	{
		return true;
	}

	// Pass to base class
	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void CMeshTypeResource::OnPropertyPostChange( IProperty* property )
{
	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );

	// Check the property
	Bool recreateMesh = false;
	Bool regenerateProxies = false;
	if ( property->GetName() == TXT("isTwoSided") ) { recreateMesh = true; }

	// Recreate mesh
	if ( recreateMesh )
	{
		CreateRenderResource();
		regenerateProxies = true;
	}

	// Regenerate drawable proxies
	if ( regenerateProxies )
	{
		CDrawableComponent::RecreateProxiesOfRenderableComponents();
	}
}

void CMeshTypeResource::SetMaterial( Uint32 index, IMaterial* material )
{
	if ( index < m_materials.Size() )
	{
		m_materials[ index ] = material;
	}
}

void CMeshTypeResource::SetAutoHideDistance( Float distance )
{
	if ( distance >= 0.0f )
	{
		if ( MarkModified() )
		{
			m_autoHideDistance = distance;
		}
	}
}

IRenderResource* CMeshTypeResource::GetRenderResource() const
{
	if ( !m_renderResource )
	{
		const_cast< CMeshTypeResource* >( this )->CreateRenderResource();
	}

	return m_renderResource;
}

void CMeshTypeResource::CreateRenderResource()
{
}

void CMeshTypeResource::ReleaseRenderResource()
{
	SAFE_RELEASE( m_renderResource );
}

Float CMeshTypeResource::GetDefaultLODDistance( Int32 level )
{
	return Max( level * 10.0f, 0.0f );
}


void CMeshTypeResource::OnSerialize( IFile & file )
{
	TBaseClass::OnSerialize(file);

	if( file.IsGarbageCollector() )
	{ 
		if( m_renderResource && m_renderResource->GetRefCount() == 1 )
		{
			// ctremblay Hack to release unused render resources. 
			// If we get here It means the CMeshTypeResource is the sole owner of the render resource. There are no render proxy or anything using it. We can safely release it. 
			// It will be recreated if or when it is needed again.
			// This case happen for CMeshTypeResource in startup and world bundle. Those cpu resource are never released but we do need to clean up render resource! 
			ReleaseRenderResource();
		}
	}
}

const SMeshTypeResourceLODLevel& CMeshTypeResource::GetLODLevel( Uint32 level ) const
{
	HALT( "Invalid LOD level %d", level );
	static SMeshTypeResourceLODLevel defaultLevel;
	return defaultLevel;
}
