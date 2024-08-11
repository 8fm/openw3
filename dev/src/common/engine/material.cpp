/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "material.h"
#include "materialDefinition.h"
#include "materialGraph.h"
#include "renderer.h"
#include "cubeTexture.h"
#include "textureArray.h"
#include "renderResource.h"

IMPLEMENT_ENGINE_CLASS( IMaterial );
IMPLEMENT_RTTI_ENUM( EColorChannel );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
CodeChunk CodeChunk::EMPTY;
Red::Threads::CMutex IMaterial::st_accessMutex;
THashSet< IMaterial* > IMaterial::st_allMaterials;
#endif //NO_RUNTIME_MATERIAL_COMPILATION

IMaterial::IMaterial( IMaterial* baseMaterial )
	: m_baseMaterial( baseMaterial )
	, m_renderResource( NULL )
{
}

IMaterial::~IMaterial()
{
	RemoveRenderResource();
}

void IMaterial::OnAllHandlesReleased()
{
	// Nothing else is referencing us, so release the render resource. It'll be recreated if it's requested again.
	RemoveRenderResource();

	TBaseClass::OnAllHandlesReleased();
}

void IMaterial::ForceRecompilation( Bool createRenderResource /*= true*/ )
{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION

	// Update rendering parameters
	UpdateRenderingParams();

	// Recompile all dependent instances
	for ( IMaterial* material : st_allMaterials )
	{
		if ( material == this )
		{
			continue;
		}

		if ( material->GetBaseMaterial() == this )
		{
			material->ForceRecompilation( createRenderResource );
		}

		if ( IMaterialDefinition* thisDefinition = Cast< IMaterialDefinition >( this ) )
		{
			IMaterialDefinition* definition = material->GetMaterialDefinition();
			if ( definition && definition == thisDefinition )
			{
				material->ForceRecompilation( createRenderResource );
			}
		}
	}

	GRender->ForceMaterialRecompilation( this );
#endif

	if ( createRenderResource )
	{
		// Create new rendering resource
		CreateRenderResource();
	}
}

void IMaterial::RemoveRenderResource()
{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	if ( m_renderResource )
	{
		// Unregister from the list of materials
		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_accessMutex );
		Bool eraseResult = st_allMaterials.Erase( this );
		RED_FATAL_ASSERT( eraseResult, "Material %s was not erased from global list, expect problems", GetFriendlyName().AsChar() );
	}
#endif //NO_RUNTIME_MATERIAL_COMPILATION

	// Release rendering proxy
	SAFE_RELEASE( m_renderResource );
}

void IMaterial::CreateRenderResource()
{
	// Release current one
	const bool didExist = ( nullptr != m_renderResource );
	SAFE_RELEASE( m_renderResource );

	if ( GRender->IsDeviceLost() )
	{
		return;
	}

	// Create rendering resource for material
	m_renderResource = GRender->UploadMaterial( this );

	if ( m_renderResource )
	{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_accessMutex );
		// Register in the list
		if ( !didExist )
		{
			Bool insertResult = st_allMaterials.Insert( this );
			RED_ASSERT( insertResult, TXT("Material %s was not added to global list, expect problems"), GetFriendlyName().AsChar() );
		}
#endif

		// Proxy is valid
		m_proxyFailed = false;
	}
	else
	{
		// Failed
		WARN_ENGINE( TXT("Unable to create rendering proxy for material '%ls'"), GetFriendlyName().AsChar() );
		m_proxyFailed = true;
	}
}

IRenderResource* IMaterial::GetRenderResource() const
{
	// Not created, create
	if ( !m_proxyFailed && !m_renderResource )
	{
		IMaterial* nonConstMaterial = const_cast< IMaterial* >( this );
		nonConstMaterial->CreateRenderResource();
	}

	// Return proxy
	return m_renderResource;
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

void IMaterial::RecompileMaterialsUsingTexture( ITexture* texture )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_accessMutex );

	// Scan all materials
	for ( IMaterial* material : st_allMaterials )
	{
		if ( material->IsCompiled() )
		{
			IMaterialDefinition* def = material->GetMaterialDefinition();
			if ( def )
			{
				const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
				for ( Uint32 j=0; j<params.Size(); j++ )
				{
					const CMaterialGraph::Parameter& param = params[j];
					if ( param.m_type == CMaterialGraph::PT_Texture )
					{
						THandle< ITexture > theTexture;
						material->ReadParameter( param.m_name, theTexture );

						// Is it that texture ?
						if ( theTexture == texture )
						{
							material->ForceRecompilation();
							break;
						}
					}
				}
			}
		}
	}
}

void IMaterial::RecompileMaterialsUsingCube( CCubeTexture* texture )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_accessMutex );

	// Scan all materials
	for ( IMaterial* material : st_allMaterials )
	{
		if ( material->IsCompiled() )
		{
			IMaterialDefinition* def = material->GetMaterialDefinition();
			if ( def )
			{
				const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
				for ( Uint32 j=0; j<params.Size(); j++ )
				{
					const CMaterialGraph::Parameter& param = params[j];
					if ( param.m_type == CMaterialGraph::PT_Cube )
					{
						THandle< CCubeTexture > theTexture;
						material->ReadParameter( param.m_name, theTexture );

						// Is it that texture ?
						if ( theTexture == texture )
						{
							material->ForceRecompilation();
							break;
						}
					}
				}
			}
		}
	}
}

void IMaterial::RecompileMaterialsUsingTextureArray( const CTextureArray* textureArray )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > mutex( st_accessMutex );

	// Scan all materials
	for ( IMaterial* material : st_allMaterials )
	{
		if ( material->IsCompiled() )
		{
			IMaterialDefinition* def = material->GetMaterialDefinition();
			if ( def )
			{
				const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
				for ( Uint32 j=0; j<params.Size(); j++ )
				{
					const CMaterialGraph::Parameter& param = params[j];
					if ( param.m_type == CMaterialGraph::PT_TextureArray )
					{
						THandle< CTextureArray > theTexture;
						material->ReadParameter( param.m_name, theTexture );

						// Is it that texture ?
						if ( theTexture == textureArray )
						{
							material->ForceRecompilation();
							break;
						}
					}
				}
			}
		}
	}
}

#endif

void IMaterial::GatherTexturesUsed( IMaterial* material, Uint32 materialIndex, TDynArray< CBitmapTexture* >& textures )
{
	if ( material )
	{
		// Get definition for this material 
		IMaterialDefinition* def = material->GetMaterialDefinition();
		if ( def )
		{
			// Scan for texture parameters
			const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
			for ( Uint32 i=0; i<params.Size(); i++ )
			{
				const IMaterialDefinition::Parameter& param = params[i];
				if ( param.m_type == IMaterialDefinition::PT_Texture )
				{
					// Get value ( the bounded texture )
					THandle< CBitmapTexture > texture;
					material->ReadParameter( param.m_name, texture );

					// Add to list
					if ( texture.IsValid() )
					{
						// Find existing slot
						Bool added = false;
						for ( Uint32 j=0; j<textures.Size(); j++ )
						{
							CBitmapTexture* texInfo = textures[j];
							if ( texInfo == texture.Get() )
							{
								added = true;
								break;
							}
						}

						// Add new info struct
						if ( !added )
						{
							textures.PushBack( texture.Get() );
						}
					}					
				}
			}
		}
	}
}

void IMaterial::GatherTextureRenderResources( TDynArray< IRenderResource* >& textures )
{
	// Get definition for this material 
	IMaterialDefinition* def = GetMaterialDefinition();
	if ( !def )
	{
		return;
	}

	// Scan for texture parameters
	const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
	for ( Uint32 i=0; i<params.Size(); i++ )
	{
		const IMaterialDefinition::Parameter& param = params[i];
		if ( param.m_type == IMaterialDefinition::PT_Texture )
		{
			// Get value ( the bounded texture )
			THandle< CBitmapTexture > texture;
			ReadParameter( param.m_name, texture );

			// Add to list
			if ( texture.IsValid() )
			{
				// Find existing slot
				textures.PushBackUnique( texture.Get()->GetRenderResource() );
			}
		}
		else if ( param.m_type == IMaterialDefinition::PT_TextureArray )
		{
			// Get value ( the bounded texture )
			THandle< CTextureArray > texture;
			ReadParameter( param.m_name, texture );

			// Add to list
			if ( texture.IsValid() )
			{
				// Find existing slot
				textures.PushBackUnique( texture.Get()->GetRenderResource() );
			}
		}
		else if ( param.m_type == IMaterialDefinition::PT_Cube )
		{
			// Get value ( the bounded texture )
			THandle< CCubeTexture > texture;
			ReadParameter( param.m_name, texture );

			// Add to list
			if ( texture.IsValid() )
			{
				// Find existing slot
				textures.PushBackUnique( texture.Get()->GetRenderResource() );
			}
		}
	}
}
