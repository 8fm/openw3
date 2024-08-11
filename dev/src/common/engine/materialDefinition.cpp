/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialDefinition.h"
#include "../core/sortedmap.h"

IMPLEMENT_ENGINE_CLASS( IMaterialDefinition );

IMPLEMENT_RTTI_ENUM( EMaterialDataType );
IMPLEMENT_RTTI_ENUM( EMaterialSamplerType );
IMPLEMENT_RTTI_ENUM( EMaterialParamType );
IMPLEMENT_RTTI_ENUM( EMaterialShaderTarget );
IMPLEMENT_RTTI_ENUM( ERenderFeedbackDataType );
IMPLEMENT_RTTI_ENUM( EMaterialVertexFactory );

IMaterialDefinition::IMaterialDefinition()
	: IMaterial( nullptr )
	, m_canUseOnMeshes( true )
	, m_canUseOnApexMeshes( false )
	, m_canUseOnParticles( false )
	, m_canUseOnCollapsableObjects( false )
	, m_canUseSkinning( true )
	, m_canUseSkinnedInstancing( false )
	, m_canUseOnMorphMeshes( false )
{
}

// REMOVE AFTER RESAVE
class CompiledTechniqueTEMP_REMOVE_AFTER_RESAVE
{
public:
	TMaterialUsedParameterArray			m_pixelParameters;		// Dynamic data binders
	TMaterialUsedParameterArray			m_vertexParameters;		// Dynamic data binders
	Uint64								m_pixelShaderHash;		// Hash of pixel shader in shader cache
	Uint64								m_vertexShaderHash;		// Hash of vertex shader in shade  cache
	Uint64								m_hullShaderHash;		// Hash of hull shader in shader cache
	Uint64								m_domainShaderHash;		// Hash of domain shader in shade  cache

public:
	// Serialization
	friend IFile& operator<<( IFile& file, CompiledTechniqueTEMP_REMOVE_AFTER_RESAVE& technique )
	{
		file << technique.m_pixelShaderHash;
		file << technique.m_vertexShaderHash;
		file << technique.m_hullShaderHash;
		file << technique.m_domainShaderHash;
		file << technique.m_pixelParameters;
		file << technique.m_vertexParameters;
		return file;
	}
};

void IMaterialDefinition::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if( file.IsGarbageCollector() )
	{
		return;		// We don't need to do anything else for GC as no pointers are stored
	}

	// TODO: Remove after resave
	if ( file.GetVersion() < VER_REMOVED_COOKEDTECHNIQUES )
	{
		THashMap< Uint32, CompiledTechniqueTEMP_REMOVE_AFTER_RESAVE > tempHashmap;

		// Material techniques
		if ( file.GetVersion() >= VER_REMOVED_TMAP )
		{
			file << tempHashmap;
		}
		else
		{
			TSortedMap< Uint32, CompiledTechniqueTEMP_REMOVE_AFTER_RESAVE > oldMap;
			file << oldMap;
		}
	}

	// Serialize
	file << m_pixelParameters;
	file << m_vertexParameters;
}

String IMaterialDefinition::GetShaderName() const 
{ 
	return GetFile() ? GetFile()->GetFileName().StringBefore( TXT(".w2mg") ) : String::EMPTY; 
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
Bool IMaterialDefinition::SupportsContext( const MaterialRenderingContext& context ) const
{
	if ( CompileAllTechniques() )
	{
		return true;
	}

	return true;
}
#endif // NO_RUNTIME_MATERIAL_COMPILATION