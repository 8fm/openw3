#include "build.h"
#include "apexResource.h"
#ifdef USE_APEX
#include "apexResource.h"
#include "NxApexAsset.h"
#include "NxParamUtils.h"
#include "NxClothingAsset.h"
#include "NxApexSDKCachedData.h"
#include "NxModule.h"
#endif
#include "game.h"
#include "gameTimeManager.h"
#include "collisionCache.h"
#include "../physics/compiledCollision.h"
#include "../physics/physicsEngine.h"
#include "../physics/physXEngine.h"
#include "baseEngine.h"
#include "../core/dataError.h"
#include "../core/2darray.h"
#include "material.h"
#include "materialDefinition.h"


namespace Config
{
	TConfigVar< Float >				cvApexLargeObjectThreshold( "Rendering/Shadows", "ApexLargeObjectThreshold", 2.5f ); // objects smaller than this are only in cascade 1-2. larger will go in 3.
}


#ifndef NO_EDITOR
/// Small record we can use to track whether an Apex asset is still in use. This is given as the asset's userData field.
struct SApexAssetUserData
{
	CApexResource*					resource;
	Red::Threads::CAtomic< Int32 >	refCount;

	SApexAssetUserData( CApexResource* resource ) : resource( resource ) {}
};
#endif

#ifdef USE_APEX
using namespace physx;
using namespace physx::apex;
#endif 

IMPLEMENT_ENGINE_CLASS( CApexResource );


#ifdef USE_APEX

class CApexResourceAsyncTask : public CTask
{
public:
	CompiledCollisionPtr m_compiledCollision;
	String m_assetName;
	class physx::apex::NxApexAsset*	m_savedAsset;

	TDynArray< String > m_apexMaterialNames;


	static Red::Threads::CAtomic< Uint32 > m_currentlyProcessingCount;

	CApexResourceAsyncTask( CompiledCollisionPtr compiledCollision, const String& assetName ) : m_compiledCollision( compiledCollision )
	{
		m_assetName = assetName;
		if( m_assetName.Size() > 120 )
		{
			m_assetName = m_assetName.RightString( 120 );
		}
	}

public:
	void Run() override
	{
		PC_SCOPE_PIX( Apex );

		NxApexSDK* apexSdk = NxGetApexSDK();
		void* buffer = m_compiledCollision->GetGeometries()[ 0 ].GetCompiledData();
		Uint32 bufferSize = m_compiledCollision->GetGeometries()[ 0 ].GetCompiledDataSize();
		physx::PxFileBuf* stream = apexSdk->createMemoryReadStream( buffer, bufferSize );
		NxParameterized::Serializer::SerializeType serType = NxGetApexSDK()->getSerializeType( *stream );
		NxParameterized::Serializer*  serializer = apexSdk->createSerializer(serType);
		NxParameterized::Serializer::DeserializedData data;
		NxParameterized::Serializer::ErrorType error = serializer->deserialize(*stream, data);
		serializer->release();
		stream->release();

		CApexResource::AdjustApexMaterialNames( m_assetName, data[0], m_apexMaterialNames );

		m_savedAsset = apexSdk->createAsset( data[0], UNICODE_TO_ANSI( m_assetName.AsChar() ) );
		if( !m_savedAsset )
		{
			CApexResourceAsyncTask::m_currentlyProcessingCount.Decrement();
			return;
		}
		NxApexSDKCachedData& cachedData = apexSdk->getCachedData();
		NxApexModuleCachedData* moduleCache = cachedData.getCacheForModule( m_savedAsset->getObjTypeID() );
		if( !moduleCache )
		{
			CApexResourceAsyncTask::m_currentlyProcessingCount.Decrement();
			return;
		}
		{
			moduleCache->getCachedDataForAssetAtScale( *m_savedAsset, PxVec3( 0.01f, 0.01f, 0.01f ) );

		}
		CApexResourceAsyncTask::m_currentlyProcessingCount.Decrement();
	}

#ifndef NO_DEBUG_PAGES
public:
	virtual const Char* GetDebugName() const override { return TXT("CApexResourceAsyncTask"); }
	virtual Uint32		GetDebugColor() const override { return Color::CYAN.ToUint32(); }
#endif
};

Red::Threads::CAtomic< Uint32 > CApexResourceAsyncTask::m_currentlyProcessingCount( 0 );

#endif 

CApexResource::CApexResource()
	: m_savedAsset( NULL )
#ifndef NO_EDITOR
	, m_previewAsset( NULL )
#endif
	, m_defaultParameters( NULL )
	, m_shadowDistance( -1.0f )
{
}

CApexResource::~CApexResource()
{
#ifdef USE_APEX

#ifndef NO_EDITOR
	ReleaseAssetRef( m_previewAsset );
	ReleaseAssetRef( m_savedAsset );
	m_previewAsset = NULL;
#endif

	m_savedAsset = NULL;
#ifdef NO_EDITOR
	m_compiledBuffer.Reset();
#endif

	DestroyDefaults();

#endif
}


void CApexResource::DestroyDefaults()
{
#ifdef USE_APEX
	if ( m_defaultParameters )
	{
		const_cast< NxParameterized::Interface* >( m_defaultParameters )->destroy();
		m_defaultParameters = NULL;
	}
#endif
}

void CApexResource::CreateDefaults()
{
#ifdef USE_APEX
	DestroyDefaults();

	NxApexSDK* apexSdk = NxGetApexSDK();
	if( apexSdk )
	{
		physx::PxFileBuf* stream = apexSdk->createMemoryReadStream( m_apexBinaryAsset.TypedData(), m_apexBinaryAsset.Size() );
		if (!stream || stream->getOpenMode() != physx::PxFileBuf::OPEN_READ_ONLY) return;

		NxParameterized::Serializer::SerializeType serType = NxGetApexSDK()->getSerializeType(*stream);
		NxParameterized::Serializer * ser = apexSdk->createSerializer(serType);

		NxParameterized::Serializer::DeserializedData data;
		NxParameterized::Serializer::ErrorType error = ser->deserialize(*stream, data); // assume there is one asset in the stream for this case

		// Clean up serializer and stream, now that we're done with them.
		ser->release();

		stream->release();

		if( error != NxParameterized::Serializer::ERROR_NONE) return;

		NxParameterized::Interface *params = data[0];

		// Make a copy of the asset's parameters, which we can use as our defaults.
		NxParameterized::Interface *defParams = 0;
		params->clone( defParams );
		m_defaultParameters = defParams;

		data.releaseAll();
	}
#endif
}


void CApexResource::AdjustApexMaterialNames( const String& namePrefix, NxParameterized::Interface* params, TDynArray< String >& outApexMaterialNames, const TDynArray< String >* namesToCheckAgainst /*= nullptr*/ )
{
	if ( !params ) return;

	// After we're loaded, we'll go through the material list and prefix the apex material name with this resource's depot path. This
	// will make the material unique to this resource, and allow us to play nicely with apex :) The name change is only applied to the
	// Apex material, so there is no visible change (e.g. the material list in the mesh editor shows unaltered names).
	//
	// By modifying m_defaultParameters, the change will be applied to any asset (and so any actor) created from this resource, but it
	// will not be saved, so we're safe to repeat it next time it's loaded. By doing it live (not at import), we avoid problems with if
	// the depot name should change (e.g. rename, or making a copy). Also, it prevents having to reimport existing assets.
	String materialNamePrefix = namePrefix + TXT("::");

	// We should have exactly as many apex names as material names, so reserve space for that.
	outApexMaterialNames.ClearFast();

#ifdef USE_APEX
	NxApexSDK* apexSdk = NxGetApexSDK();

	// Find all 'materialNames' array. Cloths and Destructibles have differences in their parameters, and this is nice and general.
	PxU32 numMaterialNamesArrays;
	const NxParameterized::ParamResult* arraysResult = NxParameterized::getParamList( *params, NULL, "materialNames",
		numMaterialNamesArrays, true, false, apexSdk->getParameterizedTraits() );
	if ( arraysResult )
	{
		// Figure out how many materials we have, so we can reserve just enough space.
		{
			Uint32 numMaterials = 0;
			for ( Uint32 i = 0; i < numMaterialNamesArrays; ++i )
			{
				const NxParameterized::ParamResult& namesResult = arraysResult[i];
				if ( namesResult.mDataType == NxParameterized::TYPE_ARRAY )
				{
					numMaterials += namesResult.mArraySize;
				}
			}
			outApexMaterialNames.Reserve( numMaterials );
		}


		// Loop over all materialNames arrays. Depending on the asset type, there may be multiple lists for different LODs or something.
		for ( Uint32 i = 0; i < numMaterialNamesArrays; ++i )
		{
			const NxParameterized::ParamResult& namesResult = arraysResult[i];
			if ( namesResult.mDataType == NxParameterized::TYPE_ARRAY )
			{
				// Grab a handle to the actual materialNames array. We'll iterate over it and make the required changes to the names.
				NxParameterized::Handle namesHandle( namesResult.mHandle );
				for ( Int32 j = 0; j < namesResult.mArraySize; ++j )
				{
					namesHandle.set( j );

					// Modify the material name.
					const char* materialName;
					namesHandle.getParamString( materialName );
					String materialString = String( ANSI_TO_UNICODE( materialName ) );
					String newMaterialName = materialNamePrefix + materialString;

					// Store the new name back into the Apex parameters, and also into our own array.
					namesHandle.setParamString( UNICODE_TO_ANSI( newMaterialName.AsChar() ) );

					// Only add new ones to our array. A single name can appear multiple times, if multiple chunks in the resource use the same material.
					// We still need to modify the name above, so that every chunk uses the correct name.
					if ( !outApexMaterialNames.Exist( newMaterialName ) )
					{
#ifndef NO_RESOURCE_IMPORT
						if ( namesToCheckAgainst != nullptr )
						{
							ASSERT( materialString == (*namesToCheckAgainst)[outApexMaterialNames.Size()], TXT("Apex material name does not match?") );
						}
#endif

						outApexMaterialNames.PushBack( newMaterialName );
					}

					namesHandle.popIndex();
				}
			}
		}

		NxParameterized::releaseParamList( numMaterialNamesArrays, arraysResult, apexSdk->getParameterizedTraits() );
	}

#ifndef NO_RESOURCE_IMPORT
	if ( namesToCheckAgainst != nullptr )
	{
		ASSERT( outApexMaterialNames.Size() == namesToCheckAgainst->Size(), TXT("Apex Resource has %d apex material names, but %d engine material names. Should be the same!"), outApexMaterialNames.Size(), namesToCheckAgainst->Size() );
	}
#endif

#endif
}

void CApexResource::ExtractApexMaterialNames( const NxParameterized::Interface* params, TDynArray< String >& outApexMaterialNames )
{
	if ( !params ) return;

	// We should have exactly as many apex names as material names, so reserve space for that.
	outApexMaterialNames.ClearFast();

#ifdef USE_APEX
	NxApexSDK* apexSdk = NxGetApexSDK();

	// Find all 'materialNames' array. Cloths and Destructibles have differences in their parameters, and this is nice and general.
	PxU32 numMaterialNamesArrays;
	const NxParameterized::ParamResult* arraysResult = NxParameterized::getParamList( *params, NULL, "materialNames",
		numMaterialNamesArrays, true, false, apexSdk->getParameterizedTraits() );
	if ( arraysResult )
	{
		// Figure out how many materials we have, so we can reserve just enough space.
		{
			Uint32 numMaterials = 0;
			for ( Uint32 i = 0; i < numMaterialNamesArrays; ++i )
			{
				const NxParameterized::ParamResult& namesResult = arraysResult[i];
				if ( namesResult.mDataType == NxParameterized::TYPE_ARRAY )
				{
					numMaterials += namesResult.mArraySize;
				}
			}
			outApexMaterialNames.Reserve( numMaterials );
		}


		// Loop over all materialNames arrays. Depending on the asset type, there may be multiple lists for different LODs or something.
		for ( Uint32 i = 0; i < numMaterialNamesArrays; ++i )
		{
			const NxParameterized::ParamResult& namesResult = arraysResult[i];
			if ( namesResult.mDataType == NxParameterized::TYPE_ARRAY )
			{
				// Grab a handle to the actual materialNames array. We'll iterate over it and make the required changes to the names.
				NxParameterized::Handle namesHandle( namesResult.mHandle );
				for ( Int32 j = 0; j < namesResult.mArraySize; ++j )
				{
					namesHandle.set( j );

					// Modify the material name.
					const char* materialName;
					namesHandle.getParamString( materialName );
					String materialString = String( ANSI_TO_UNICODE( materialName ) );

					// Only add new ones to our array. A single name can appear multiple times, if multiple chunks in the resource use the same material.
					// We still need to modify the name above, so that every chunk uses the correct name.
					if ( !outApexMaterialNames.Exist( materialString ) )
					{
						outApexMaterialNames.PushBack( materialString );
					}

					namesHandle.popIndex();
				}
			}
		}

		NxParameterized::releaseParamList( numMaterialNamesArrays, arraysResult, apexSdk->getParameterizedTraits() );
	}
#endif
}


Float CApexResource::CalcShadowDistance( CWorld* world, const Box& bounds ) const
{
	Float shadowDistance = m_shadowDistance;

	if ( m_shadowDistance < 0.0f )
	{
		RED_ASSERT( world != nullptr, TXT("Non-null world is required!") );
		Float extent = GetBoundingBox().CalcExtents().Mag3();

		Float cascadeEnd = world->GetShadowConfig().m_cascadeRange2;
		// Large objects will reach into cascade 3
		if ( extent > Config::cvApexLargeObjectThreshold.Get() )
		{
			cascadeEnd = world->GetShadowConfig().m_cascadeRange3;
		}

		// Shadow distance is just at the end of the selected cascade. Bump it back by the bounds extent so we hopefully won't
		// get cut by the cascade boundary.
		shadowDistance = cascadeEnd - extent;
	}

	return Min( shadowDistance, m_autoHideDistance );
}


void CApexResource::OnPropertyPostChange( IProperty* property )
{
	CMeshTypeResource::OnPropertyPostChange( property );

	GCollisionCache->InvalidateCollision( GetDepotPath() );
}

void CApexResource::AddRef()
{
#ifdef NO_EDITOR
	if( m_ref.GetValue() == 0 )
	{
		AddToRootSet();
#ifdef USE_APEX
		m_savedAsset->userData = this;
#endif
	}
#else
	if( m_ref.GetValue() == 0 )
	{
		AddToRootSet();
		CreateDefaults();

		// Fix up material names so they have our depot path as a prefix. The names are fixed offline on cook, so only do
		// this for uncooked.
		if ( !IsCooked() )
		{
			AdjustApexMaterialNames( GetDepotPath(), const_cast< NxParameterized::Interface* >( m_defaultParameters ), m_apexMaterialNames, &m_materialNames );
		}

		RebuildPreviewAsset( false );

		// Also use it for the saved asset. Since we keep track of references, we can use the same object (rather than duplicating it).
		ReleaseAssetRef( m_savedAsset );
		m_savedAsset = m_previewAsset;

		AddAssetRef( m_savedAsset );
	}
#endif
	m_ref.Increment();
}

void CApexResource::ReleaseRef()
{
#ifdef NO_EDITOR
	if( m_ref.GetValue() == 1 && m_savedAsset )
	{
		m_savedAsset = nullptr;
		m_compiledBuffer.Reset();
		RemoveFromRootSet();

	}
#else
	if( m_ref.GetValue() == 1 )
	{
		ReleaseAssetRef( m_previewAsset );
		ReleaseAssetRef( m_savedAsset );
		m_previewAsset = NULL;
		m_savedAsset = nullptr;
		DestroyDefaults();
		RemoveFromRootSet();
	}
#endif
	m_ref.Decrement();
}


Bool CApexResource::TryPreload( CompiledCollisionPtr& compiledBuffer )
{
#ifndef NO_EDITOR
	return m_apexBinaryAsset.Size() != 0;
#else
	if( m_savedAsset ) return true;

	if( compiledBuffer->GetGeometries().Empty() || !compiledBuffer->GetGeometries()[ 0 ].GetGeometry() ) return false;

	m_compiledBuffer = compiledBuffer;

	SCachedGeometry& compiledGeometry = const_cast< SCachedGeometry& >( compiledBuffer->GetGeometries()[ 0 ] );
	m_savedAsset = ( physx::apex::NxApexAsset* ) compiledGeometry.GetGeometry();

	// If we're not cooked, we need to grab material names from the apex asset, since they were adjusted when adding to
	// the collision cache. When we're cooked, m_apexMaterialNames is already filled in with proper names.
#ifdef USE_APEX
	if ( !IsCooked() )
	{
		ExtractApexMaterialNames( m_savedAsset->getAssetNxParameterized(), m_apexMaterialNames );
	}
#endif
	return true;
#endif
}


#ifndef NO_EDITOR

void CApexResource::AddAssetRef( physx::apex::NxApexAsset* asset )
{
#ifdef USE_APEX
	if( !asset ) return;

	SApexAssetUserData* data = ( SApexAssetUserData* )asset->userData;

	ASSERT( data != NULL, TXT("Apex Asset has NULL userData. Not created through CApexResource?") );
	ASSERT( data->resource == this, TXT("Trying to addref an Apex Asset through a CApexResource that does not own it.") );

	if ( data != NULL && data->resource == this )
	{
		data->refCount.Increment();
	}
#endif
}

void CApexResource::ReleaseAssetRef( physx::apex::NxApexAsset* asset )
{
	if ( !asset ) return;

#ifdef USE_APEX
	SApexAssetUserData* data = ( SApexAssetUserData* )asset->userData;
	
	ASSERT( data != NULL, TXT("Apex Asset has NULL userData. Not created through CApexResource?") );
	ASSERT( data->resource == this, TXT("Trying to release an Apex Asset through a CApexResource that does not own it.") );

	if ( data != NULL && data->resource == this )
	{
		Int32 refs = data->refCount.Decrement();
		ASSERT( refs >= 0, TXT("Negative refcount on Apex Asset. Released too many times?") );

		if ( refs == 0 )
		{
			delete data;
			asset->userData = NULL;
			asset->release();
		}
	}
#endif
}

#endif


void CApexResource::OnPostLoad()
{
#ifndef NO_EDITOR
	for ( Uint32 i = 0; i < m_materials.Size(); ++i )
	{
		if (m_materials[i].IsValid() && m_materials[i]->GetBaseMaterial())
		{
			IMaterialDefinition* materialDefinition = Cast<IMaterialDefinition>( m_materials[i]->GetBaseMaterial() );
			if ( materialDefinition && ! materialDefinition->CanUseOnApexMeshes() )
			{
				DATA_HALT( DES_Major, this, TXT("Rendering"), TXT("Chunk '%ls' uses a material that is not set to be used on apex meshes"), m_materialNames[i].AsChar() );
			}
		}
	}
#endif
	m_assetName = GetDepotPath();
	TBaseClass::OnPostLoad();
}

#ifndef NO_EDITOR
void CApexResource::OnSave()
{
	// If we have a saved asset, we want to release it and make sure it matches the preview asset.
	if ( m_savedAsset )
	{
		// Make sure changes are reflected in the assets.
		RebuildPreviewAsset( false );

		// Update our saved asset to match the preview.
		ReleaseAssetRef( m_savedAsset );
	}
	// If we don't have a saved asset, then we must have just been imported, so we need to do some initialization.
	else
	{
		// We don't get OnPostLoad() after import, so we'll do what would have been done there. Can't do the material name fix during import,
		// since we don't have a depot path yet.
		CreateDefaults();

		if ( !IsCooked() )
		{
			AdjustApexMaterialNames( GetDepotPath(), const_cast< NxParameterized::Interface* >( m_defaultParameters ), m_apexMaterialNames, &m_materialNames );
		}


		m_assetName = GetDepotPath();

		// When importing a new apex resource, we want to make sure all stored parameters are at their default values (as defined in the APB file).
		RebuildPreviewAsset( true );
	}

	m_savedAsset = m_previewAsset;
	AddAssetRef( m_savedAsset );
	
	TBaseClass::OnSave();
}
#endif

#ifdef USE_APEX

Float CApexResource::GetDefaultParameterF32( const StringAnsi& name, Float defVal ) const
{
	RED_ASSERT( m_defaultParameters );
	// getParam*() does not touch val if it is not successful.
	Float val = defVal;
	NxParameterized::getParamF32( *m_defaultParameters, name.AsChar(), val );
	return val;
}

Uint32 CApexResource::GetDefaultParameterU32( const StringAnsi& name, Uint32 defVal ) const
{
	RED_ASSERT( m_defaultParameters );
	physx::PxU32 val = defVal;
	NxParameterized::getParamU32( *m_defaultParameters, name.AsChar(), val );
	return val;
}
Bool CApexResource::GetDefaultParameterBool( const StringAnsi& name, Bool defVal ) const
{
	RED_ASSERT( m_defaultParameters );
	Bool val = defVal;
	NxParameterized::getParamBool( *m_defaultParameters, name.AsChar(), val );
	return val;
}
Vector3 CApexResource::GetDefaultParameterVec3( const StringAnsi& name, const Vector3& defVal ) const
{
	RED_ASSERT( m_defaultParameters );
	physx::PxVec3 pxVal;
	if ( NxParameterized::getParamVec3( *m_defaultParameters, name.AsChar(), pxVal ) )
	{
		return Vector3( pxVal.x, pxVal.y, pxVal.z );
	}
	return defVal;
}


void CApexResource::SetParameterF32( NxParameterized::Interface* params, const StringAnsi& name, Float val ) const
{
	NxParameterized::setParamF32( *params, name.AsChar(), val );
}
void CApexResource::SetParameterU32( NxParameterized::Interface* params, const StringAnsi& name, Uint32 val ) const
{
	NxParameterized::setParamU32( *params, name.AsChar(), val );
}
void CApexResource::SetParameterBool( NxParameterized::Interface* params, const StringAnsi& name, Bool val ) const
{
	NxParameterized::setParamBool( *params, name.AsChar(), val );
}
void CApexResource::SetParameterVec3( NxParameterized::Interface* params, const StringAnsi& name, const Vector3& val ) const
{
	NxParameterized::setParamVec3( *params, name.AsChar(), physx::PxVec3( val.X, val.Y, val.Z ) );
}

#endif

#ifndef NO_EDITOR
void CApexResource::RebuildPreviewAsset( Bool useDefaultParameters )
{
#ifdef USE_APEX
	if ( !m_defaultParameters ) return;
	NxApexSDK* apexSdk = NxGetApexSDK();
	if( !apexSdk )
	{
		return;
	}

	// Start out with the default parameters. We need to do a clone even when using defaults, because the new asset takes ownership of the params.
	NxParameterized::Interface *params = 0;
	m_defaultParameters->clone( params );
	
	// If we don't want to use defaults, overwrite parameters with appropriate values.
	if ( !useDefaultParameters )
	{
		ConfigureNewAsset( params );
	}
	else
	{
		// Make sure any locally-stored parameters reflect the default settings. We don't need to do anything with params, since it'll already
		// have the defaults in it.
		RestoreDefaults();
	}

	// Release any existing asset.
	ReleaseAssetRef( m_previewAsset );

	// Using a combination of the current time and a random number, we should be pretty sure not to get a duplicate name. This is only needed
	// in the editor, since a resource may be reloaded at any time.
	// Apex has a (hidden?) max length of 120 characters for asset names, so we need to make sure the name we give is not too long...
	StringAnsi suffix = StringAnsi::Printf( "%f:%d", ( Float ) EngineTime::GetNow(), GEngine->GetRandomNumberGenerator().Get< Int32 >() );
	StringAnsi prefix = UNICODE_TO_ANSI( m_assetName.LeftString( 120 - suffix.Size() ).AsChar() );
	StringAnsi assetNameStr = prefix + suffix;

#ifndef RED_FINAL_BUILD
	CPhysXLogger::ClearLastError();
#endif
	m_previewAsset = apexSdk->createAsset( params, assetNameStr.AsChar() );
#ifndef RED_FINAL_BUILD
	if( CPhysXLogger::GetLastErrorString().Size() && CPhysXLogger::IsLastErrorFromSameThread() )
	{
		DATA_HALT( DES_Major, this, TXT("Apex Resource"), CPhysXLogger::GetLastErrorString().AsChar() );
		CPhysXLogger::ClearLastError();
	}
#endif

	m_previewAsset->userData = new SApexAssetUserData( this );

	AddAssetRef( m_previewAsset );
#endif
}
#endif

#ifndef NO_EDITOR
void CApexResource::RevertPreviewAsset()
{
	ReleaseAssetRef( m_previewAsset );
	m_previewAsset = m_savedAsset;
	AddAssetRef( m_previewAsset );

#ifdef USE_APEX
	if( !m_savedAsset ) return;
	// Restore saved property values
	const NxParameterized::Interface* oldDefaultParams = m_defaultParameters;
	m_defaultParameters = m_savedAsset->getAssetNxParameterized();
	RestoreDefaults();
	m_defaultParameters = oldDefaultParams;
#endif
}

void CApexResource::FillStatistics( C2dArray* array )
{
#ifdef USE_APEX
	CompiledCollisionPtr compiledCollision;
	RED_MESSAGE( "TODO: this is not supporting asynchronous collision cache, STALLS on main thread possible" );
	GCollisionCache->FindCompiled_Sync( compiledCollision, GetDepotPath(), GetFileTime() );

	if( !compiledCollision ) return;
	String name = String::Printf( TXT( "%s %f.csv" ), m_assetName.AsChar(), ( Float ) EngineTime::GetNow() ) ;

	CApexResourceAsyncTask testTask( compiledCollision, name );

#ifndef RED_FINAL_BUILD
	CPhysXLogger::ClearLastError();
#endif
	double start = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	testTask.Run();
	double end = Red::System::Clock::GetInstance().GetTimer().GetSeconds();

#ifndef RED_FINAL_BUILD
	if( CPhysXLogger::GetLastErrorString().Size() && CPhysXLogger::IsLastErrorFromSameThread() )
	{
		DATA_HALT( DES_Major, this, TXT("Apex Resource"), CPhysXLogger::GetLastErrorString().AsChar() );
		CPhysXLogger::ClearLastError();
	}
#endif

	Int32 row = array->GetRowIndex( TXT("Asset"), GetFriendlyName() );
	array->SetValue( String::Printf( TXT( "%f" ), ( float ) end - start ),  String( TXT("LoadTime") ), row );

	if( m_savedAsset )
	{
		testTask.m_savedAsset->release();
		return;
	}
	m_savedAsset = testTask.m_savedAsset;
	m_savedAsset->userData = new SApexAssetUserData( this );
	SApexAssetUserData* data = ( SApexAssetUserData* ) m_savedAsset->userData;
	data->refCount.Increment();
#endif
}

void CApexResource::FillLODStatistics( C2dArray* array ){}

#endif


CApexResource* CApexResource::FromAsset( physx::apex::NxApexAsset* asset )
{
#ifdef USE_APEX
#ifdef NO_EDITOR
	return ( CApexResource* )asset->userData;
#else
	SApexAssetUserData* data = ( SApexAssetUserData* )asset->userData;
	ASSERT( data != NULL && data->resource != NULL, TXT("CApexResource::FromAsset() called for Apex Asset not created by CApexResource?") );
	return data->resource;
#endif
#else
	return NULL;
#endif
}

CompiledCollisionPtr CApexResource::CompileCollision( CObject* parent ) const
{
#ifndef USE_APEX
	return CompiledCollisionPtr();
#else
	NxApexSDK* apexSdk = NxGetApexSDK();

	NxApexSDKCachedData& cachedData = apexSdk->getCachedData();
	cachedData.clear();

	NxParameterized::Serializer::DeserializedData data;
	physx::PxFileBuf* stream = 0;
	stream = apexSdk->createMemoryReadStream( m_apexBinaryAsset.TypedData(), m_apexBinaryAsset.Size() );

	NxParameterized::Serializer::SerializeType serType = NxGetApexSDK()->getSerializeType(*stream);
	NxParameterized::Serializer * ser = apexSdk->createSerializer(serType);
	if( !ser )
	{
		ERR_ENGINE( TXT("CApexResource::CompileCollision broken asset '%ls' "), GetFriendlyName().AsChar() );
		return CompiledCollisionPtr();
	}
	NxParameterized::Serializer::ErrorType error = ser->deserialize(*stream, data);
	ser->release();
	stream->release();
	ConfigureNewAsset( data[0] );

	// Just build the names into a temporary array. We're just compiling into the collision cache here, don't want to modify
	// our stored names. That will be done when cooking the resource.
	TDynArray< String > apexMtlNames;
#ifdef NO_EDITOR
	AdjustApexMaterialNames( GetDepotPath(), data[0], apexMtlNames, nullptr );
#else
	AdjustApexMaterialNames( GetDepotPath(), data[0], apexMtlNames, &m_materialNames );
#endif


	stream = apexSdk->createMemoryWriteStream();
	ser = apexSdk->createSerializer(::NxParameterized::Serializer::SerializeType::NST_BINARY);
	const NxParameterized::Interface* parameters = data[0];
	ser->serialize( *stream, &parameters, 1 );

	CompiledCollisionPtr compiled ( new CCompiledCollision() );
	static Uint32 counter = 0;
	++counter;
	StringAnsi tempAssetName = 	StringAnsi::Printf( "apex%i", counter );
	physx::apex::NxApexAsset* asset = apexSdk->createAsset( data[0], tempAssetName.AsChar() );

	if( asset )
	{
		SCachedGeometry & geometry = compiled->InsertGeometry();
		geometry.m_geometryType = ( char ) PxGeometryType::eGEOMETRY_COUNT;
		geometry.m_densityScaler = 1;
		geometry.m_assetId = counter;

		NxApexSDKCachedData& cachedData = apexSdk->getCachedData();
		NxApexModuleCachedData* moduleCache = cachedData.getCacheForModule( asset->getObjTypeID() );
		if( moduleCache )
		{
			moduleCache->getCachedDataForAssetAtScale( *asset, PxVec3( 1.0f, 1.0f, 1.0f ) );
			physx::general_PxIOStream2::PxFileBuf* buf = apexSdk->createMemoryWriteStream();
			physx::general_PxIOStream2::PxFileBuf& resultBuf = moduleCache->serializeSingleAsset( *asset, *buf );

			void * geometryBuffer = geometry.AllocateCompiledData( stream->getFileLength() + buf->getFileLength() );
			stream->read( geometryBuffer, stream->getFileLength() );
			buf->read( static_cast< Uint8* >(geometryBuffer) + stream->getFileLength(), buf->getFileLength() );
			buf->release();
		}
		else
		{
			void * geometryBuffer = geometry.AllocateCompiledData( stream->getFileLength() );
			stream->read( geometryBuffer, stream->getFileLength() );
		}
		
		asset->release();
	}

	ser->release();
	stream->release();
	
	return compiled;
#endif
}

void CApexResource::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

#ifdef NO_EDITOR
	if( file.IsReader() )
	{
		m_apexBinaryAsset.Clear();
	}
#endif

	if( file.IsGarbageCollector() ) 
	{
		return;
	}

	if( !file.IsCooker() || !file.IsWriter() ) return;

	DecideToIncludeToCollisionCache();
	m_apexBinaryAsset.Clear();
}


#ifndef NO_RESOURCE_COOKING

void CApexResource::OnCook( ICookerFramework& cooker )
{
#ifndef NO_EDITOR
#ifdef USE_APEX
	// Adjust our stored material names.
	CreateDefaults();
	NxParameterized::Interface* params = nullptr;
	m_defaultParameters->clone( params );
	AdjustApexMaterialNames( GetDepotPath(), params, m_apexMaterialNames, &m_materialNames );

	params->destroy();
#endif
#endif
	TBaseClass::OnCook( cooker );
}

#endif

void CApexResource::DecideToIncludeToCollisionCache()
{
	CompiledCollisionPtr compiled;
	GCollisionCache->Compile( compiled, this, GetDepotPath(), GetFileTime() );
}
