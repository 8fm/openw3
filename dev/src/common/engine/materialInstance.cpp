/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "materialInstance.h"
#include "../../common/core/gatheredResource.h"
#include "../core/depot.h"
#include "mesh.h"
#include "meshEnum.h"
#include "../core/fileSkipableBlock.h"
#include "materialGraph.h"
#include "materialParameter.h"
#include "cubeTexture.h"
#include "textureArray.h"
#include "../core/2darray.h"
#include "../core/debugPageHTMLDoc.h"

IMPLEMENT_ENGINE_CLASS( CMaterialInstance );

CGatheredResource resTextureAutoBindDef( TXT("engine\\materials\\textureautobind.csv"), RGF_NotCooked );

CMaterialInstance::CMaterialInstance()
	: IMaterial( NULL )
	, m_enableMask( false )
{
}

CMaterialInstance::CMaterialInstance( CObject* parent, IMaterial* material, Bool createRenderResource /*= true*/ )
	: IMaterial( material )
	, m_enableMask( false )
{
	SetParent( parent );

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	ForceRecompilation( createRenderResource );
#endif
}

#ifndef NO_EDITOR
Bool CMaterialInstance::HasAnyBlockParameters() const
{
	return GetMaterialDefinition()->HasAnyBlockParameters();
}
#endif

void CMaterialInstance::OnSerialize( IFile& file )
{
	// Serialize base class
	TBaseClass::OnSerialize( file );

	// GC
	if ( file.IsGarbageCollector() )
	{
		{
			for ( Uint32 i=0; i<m_parameters.Size(); i++ )
			{
				MaterialParameterInstance& info = m_parameters[i];
				info.Serialize( file );
			}
		}
		return;
	}

	// Save parameters
	if ( file.IsWriter() )
	{
		// Write number of parameters
		Uint32 count = m_parameters.Size();
		file << count;

		// Save each parameter
		for ( TMaterialParameters::iterator i=m_parameters.Begin(); i!=m_parameters.End(); ++i )
		{
			MaterialParameterInstance& param = *i;

			// Save param
			{
				CFileSkipableBlock block( file );

				// Save parameter name
				CName paramName = param.GetName();
				file << paramName;

				// Save parameter type
				CName typeName = param.GetType()->GetName();
				file << typeName;

				// Save parameter data
				param.Serialize( file );
			}
		}
	}

	// Load parameters
	if ( file.IsReader() )
	{
		// Load number of parameters
		Uint32 numParams = 0;
		file << numParams;

		// Clear current parameter list and reserve space for params
		m_parameters.ClearFast();
		m_parameters.Reserve( numParams );

		// Load parameters
		for ( Uint32 i=0; i<numParams; i++ )
		{
			CFileSkipableBlock block( file );

			// No base material, skip parameter
			if ( !m_baseMaterial )
			{
				block.Skip();
				continue;
			}

			// Load param name
			CName paramName;
			file << paramName;

			// Load parameter type
			CName paramType;
			file << paramType;

			// Create parameter data
			MaterialParameterInstance* param = new ( m_parameters ) MaterialParameterInstance( paramName, paramType );
			if ( !param->Serialize( file ) )
			{
				m_parameters.PopBackFast();
				block.Skip();
			}

			// Convert
			{
				const MaterialParameterInstance& lastParam = m_parameters.Back();

				// Convert old, non handle values
				if ( lastParam.GetType() == GetTypeObject< ITexture* >() )
				{
					const THandle< ITexture > value( *(ITexture**)lastParam.GetData() );

					m_parameters.PopBack();

					new ( m_parameters ) MaterialParameterInstance( paramName, GetTypeName< THandle< ITexture > >(), &value );					
				}
				else if ( lastParam.GetType() == GetTypeObject< CTextureArray* >() )
				{
					const THandle< CTextureArray > value( *(CTextureArray**)lastParam.GetData() );

					m_parameters.PopBack();

					new ( m_parameters ) MaterialParameterInstance( paramName, GetTypeName< THandle< CTextureArray > >(), &value );					
				}
				else if ( lastParam.GetType() == GetTypeObject< CCubeTexture* >() )
				{
					const THandle< CCubeTexture > value( *(CCubeTexture**)lastParam.GetData() );

					m_parameters.PopBack();

					new ( m_parameters ) MaterialParameterInstance( paramName, GetTypeName< THandle< CCubeTexture > >(), &value );					
				}
				else if ( lastParam.GetType()->GetType() == RT_Pointer )
				{
					RED_LOG_ERROR( TXT("Pointer types are not supported inside CMaterialInstance. Parameter '%ls', type '%ls'"),
						lastParam.GetName().AsChar(), lastParam.GetType()->GetName().AsChar() );
				}
			}
		}
	}
}

#ifndef NO_DEBUG_PAGES
void CMaterialInstance::OnDebugPageInfo( class CDebugPageHTMLDocument& doc )
{
	TBaseClass::OnDebugPageInfo( doc );

	// Material parameters
	{
		CDebugPageHTMLInfoBlock block( doc, "Material parameters" );

		{
			CDebugPageHTMLTable table( doc, "matprops" );
			table.AddColumn( "Name", 140, true );
			table.AddColumn( "Type", 140, true );
			table.AddColumn( "Value", 320, false );

			for ( const auto& data : m_parameters )
			{
				table.AddRow(
					table.CreateRowData(data.GetName().AsAnsiChar()),
					table.CreateRowData(data.GetType() ? data.GetType()->GetName().AsAnsiChar() : "Unknown"),
					table.CreateRowData(data.GetType(), data.GetData())
					);	
			}

			table.Render(600, "generic", doc.GetURL());
		}
	}
}
#endif

void CMaterialInstance::OnPropertyPostChange( IProperty* property )
{
	// Reset
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	if ( property && property->GetName() == TXT("baseMaterial") )
	{
		m_parameters.Clear();
		ForceRecompilation();
	}
	if ( property && property->GetName() == TXT("enableMask") )
	{
		ForceRecompilation();
	}
#endif

	// Pass to base class
	TBaseClass::OnPropertyPostChange( property );
}

#ifndef NO_EDITOR
void CMaterialInstance::OnPostLoad()
{
	//ASSERT(m_baseMaterial && TXT( "CMaterialInstance has no baseMaterial" ) );
}
#endif

void CMaterialInstance::SetBaseMaterial( IMaterial* material )
{
	// Reset
	m_baseMaterial = material;
	ForceRecompilation();
}

void CMaterialInstance::ClearInstanceParameters()
{
	// Reset
	m_parameters.Clear();
	ForceRecompilation();
}

Bool CMaterialInstance::IsParameterInstanced( const CName& name ) const
{
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		const MaterialParameterInstance& param = m_parameters[i];
		if ( param.GetName() == name )
		{
			return true;
		}
	}

	return false;
}

void CMaterialInstance::AutoAssignTextures()
{
    // Auto-assign default textures
    CMaterialGraph* graph = Cast< CMaterialGraph >( GetMaterialDefinition() );
    if ( !graph )
	{
        return;
	}

	// No mesh
    CMesh *mesh = Cast<CMesh>( GetParent() );
    if ( !mesh )
	{
        return;
	}

    String separator = TXT("_");

    String meshDir  = mesh->GetFile()->GetDirectory()->GetDepotPath();
    String meshName = mesh->GetFile()->GetFileName().StringBefore(TXT("."), true);
    String modelInfo;
    String modelName;

    size_t sepsepPos; 
    if ( meshName.FindSubstring(TXT("__"), sepsepPos, true) && sepsepPos > 0)
    {
        modelName = meshName.LeftString(sepsepPos);
        modelInfo = meshName.MidString(sepsepPos+2);
    }
    if (modelName.Empty())
        modelName = meshName;

    String bodyPart = modelInfo.StringAfter(TXT("_"), true);
    if (!bodyPart.Empty())
        bodyPart = separator + bodyPart;

    // Load info about texture parameters auto binding
    C2dArray *autoBindConf = resTextureAutoBindDef.LoadAndGet< C2dArray >();
    if ( !autoBindConf )
        return;

    Uint32 n2daCols, n2daRows, nParamNameCol, nSuffixCol = 0;
    autoBindConf->GetSize( n2daCols, n2daRows );

    // Check if expected columns exist in the configuration file
    if ( !autoBindConf->FindHeader( TXT("ParamName"), nParamNameCol ) ||
         !autoBindConf->FindHeader( TXT("Suffix"),    nSuffixCol )     )
    {
        autoBindConf->Discard();
        return;
    }

    for ( Uint32 iRow = 0; iRow < n2daRows; ++iRow )
    {
        CName paramName( autoBindConf->GetValue( nParamNameCol, iRow ) );
        if ( !paramName ) continue;

        CMaterialParameter* param = graph->FindParameter( paramName, false );
        if ( param == NULL ) continue;

        String sufix = autoBindConf->GetValue( nSuffixCol, iRow );
        sufix = sufix.Empty() ? sufix : separator + sufix;
    
        // Look for the texture with body part name
        String fileName = sufix.Empty() && bodyPart.Empty()
            ? meshDir + modelName + TXT(".xbm")
            : meshDir + modelName + separator + bodyPart + sufix + TXT(".xbm");
        CDiskFile* file = GDepot->FindFile( fileName );

        // Look for the texture without body part name
        if (!file && !bodyPart.Empty())
        {
            fileName = sufix.Empty()
                ? meshDir + modelName + TXT(".xbm")
                : meshDir + modelName + separator + sufix + TXT(".xbm");
            file = GDepot->FindFile( fileName );
        }
        if ( file == NULL ) continue;
        
        // Bind the texture if it has been found
        CPropertyDataBuffer buffer( param->GetParameterProperty() );
        if ( param->GetParameterProperty()->GetType()->FromString( buffer.Data(), fileName ))
            WriteParameterRaw( param->GetParameterName(), buffer.Data());
    }
}

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
void CMaterialInstance::UpdateRenderingParams()
{
	// material instances don't update rendering params
}
#endif // !NO_RUNTIME_MATERIAL_COMPILATION

const IMaterialDefinition* CMaterialInstance::GetMaterialDefinition() const
{
	const IMaterial* material = m_baseMaterial.Get();
	while ( material && !material->IsA< IMaterialDefinition >() )
	{
		material = material->GetBaseMaterial();
	}
	ASSERT( !material || material->IsA< IMaterialDefinition >() );
	return (const IMaterialDefinition* )material;
}

IMaterialDefinition* CMaterialInstance::GetMaterialDefinition()
{
	const IMaterial* material = m_baseMaterial.Get();
	while ( material && !material->IsA< IMaterialDefinition >() )
	{
		material = material->GetBaseMaterial();
	}
	ASSERT( !material || material->IsA< IMaterialDefinition >() );
	return (IMaterialDefinition* )material;
}


void CMaterialInstance::CopyParametersFrom( IMaterial* sourceMaterial, Bool recreateResource /*= true*/ )
{
	TDynArray< CName > paramsInSource;
	sourceMaterial->GetAllParameterNames( paramsInSource );

	// Enough storage for largest parameter type.
	Uint8 paramData[ sizeof( Matrix ) ];

	for ( const auto& paramName : paramsInSource )
	{
		Red::MemoryZero( paramData, sizeof(paramData) );
		if ( sourceMaterial->ReadParameterRaw( paramName, paramData ) )
		{
			// Don't recreate the render resource. That's just unnecessary work, since we'll be setting several parameters.
			WriteParameterRaw( paramName, paramData, false );
		}
	}

	if ( recreateResource )
	{
		CreateRenderResource();
	}
}

void CMaterialInstance::GetAllParameterNames( TDynArray< CName >& outNames ) const
{
	// Since we're getting _all_ parameters, and WriteParameter generally will only add parameters that are already in
	// the material definition, it's enough to collect all parameter names from the definition.
	const IMaterialDefinition* definition = GetMaterialDefinition();
	if ( definition )
	{
		definition->GetAllParameterNames( outNames );
	}
	else
	{
		// If there's no definition then collect our own parameters. and those of any base we may have, as a fallback.
		// PushBackUnique is maybe not ideal, but we're probably not dealing with large numbers of parameters.
		for ( const auto& param : m_parameters )
		{
			outNames.PushBackUnique( param.GetName() );
		}
		if ( m_baseMaterial != nullptr )
		{
			m_baseMaterial->GetAllParameterNames( outNames );
		}
	}
}

Bool CMaterialInstance::WriteParameterRaw( const CName& name, const void* data, Bool recreateResource /*= true*/ )
{
	// Write to existing parameter
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		MaterialParameterInstance& info = m_parameters[i];
		if ( info.GetName() == name )
		{
			// Write value
			info.GetType()->Copy( info.GetData(), data );
			if ( recreateResource )
			{
				CreateRenderResource();
			}
			return true;
		}
	}

	// Get base material graph
	IMaterialDefinition* definition = GetMaterialDefinition();
	if ( definition )
	{
		// Find property
		CMaterialGraph* graph = SafeCast< CMaterialGraph >( definition );
		if ( graph )
		{
			CMaterialParameter* param = graph->FindParameter( name );
			if ( param )
			{
				// Create instanced parameter
				new ( m_parameters ) MaterialParameterInstance( name, param->GetClass(), data );
				if ( recreateResource )
				{
					CreateRenderResource();
				}
				return true;
			}
		}
	}

	// Unknown parameter
	return false;
}

Bool CMaterialInstance::ReadParameterRaw( const CName& name, void* data ) const
{
	// Write to existing parameter
	for ( Uint32 i=0; i<m_parameters.Size(); i++ )
	{
		const MaterialParameterInstance& info = m_parameters[i];

		// TODO what if name is the same, but type is different
		// like: float != vector
		if ( info.GetName() == name )
		{
			info.GetType()->Copy( data, info.GetData() );
			return true;
		}
	}

	// Read from base material
	if ( m_baseMaterial )
	{
		return m_baseMaterial->ReadParameterRaw( name, data );
	}

	// Invalid
	return false;
}


Bool CMaterialInstance::IsMasked() const
{
	const IMaterialDefinition* definition = GetMaterialDefinition();
	return definition && definition->IsMasked() && ( !definition->CanInstanceOverrideMasked() || m_enableMask );
}
