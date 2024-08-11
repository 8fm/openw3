/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "meshTypeResource.h"
#include "collisionContent.h"
#include "../physics/compiledCollision.h"

namespace physx
{
	namespace apex
	{
		class NxApexAsset;
	}
}
namespace NxParameterized
{
	class Interface;
}


class CApexResource : public CMeshTypeResource, public ICollisionContent
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CApexResource, CMeshTypeResource );
	friend class CApexImporter;

protected:
	/// We keep two apex assets: one which is used normally in-game or in-editor; and the other which is used for providing a live
	/// preview while editing an apex resource in the mesh editor. Unless there are previewable edits made, these should generally
	/// point to the same object, so no extra memory is wasted until needed.
	class physx::apex::NxApexAsset*		m_savedAsset;
#ifndef NO_EDITOR
	class physx::apex::NxApexAsset*		m_previewAsset;
#else
	CompiledCollisionPtr				m_compiledBuffer;
#endif

	String								m_assetName;

	TDynArray< String >					m_apexMaterialNames;
	Red::Threads::CAtomic< Int32 >		m_ref;
	TDynArray< Uint8, MC_Apex >			m_apexBinaryAsset;

	const NxParameterized::Interface*	m_defaultParameters;	//!< Default asset creation parameters specified by the imported asset.

	Float								m_shadowDistance;

#ifdef USE_APEX
	/// Convenience functions for getting default parameters, as specified in the originally imported asset.
	Float GetDefaultParameterF32( const StringAnsi& name, Float defVal = 0.0f ) const;
	Uint32 GetDefaultParameterU32( const StringAnsi& name, Uint32 defVal = 0 ) const;
	Bool GetDefaultParameterBool( const StringAnsi& name, Bool defVal = false ) const;
	Vector3 GetDefaultParameterVec3( const StringAnsi& name, const Vector3& defVal = Vector3(0, 0, 0) ) const;

	/// Convenience functions for setting new parameter values while creating the apex asset.
	void SetParameterF32( NxParameterized::Interface* params, const StringAnsi& name, Float val ) const;
	void SetParameterU32( NxParameterized::Interface* params, const StringAnsi& name, Uint32 val ) const;
	void SetParameterBool( NxParameterized::Interface* params, const StringAnsi& name, Bool val ) const;
	void SetParameterVec3( NxParameterized::Interface* params, const StringAnsi& name, const Vector3& val ) const;
#else
	/// In non-Apex build, Get* just returns the given default, and Set* do nothing...
	RED_INLINE Float GetDefaultParameterF32( const StringAnsi& name, Float defVal = 0.0f ) const { return defVal; }
	RED_INLINE Uint32 GetDefaultParameterU32( const StringAnsi& name, Uint32 defVal = 0 ) const { return defVal; }
	RED_INLINE Bool GetDefaultParameterBool( const StringAnsi& name, Bool defVal = false ) const { return defVal; }
	RED_INLINE Vector3 GetDefaultParameterVec3( const StringAnsi& name, const Vector3& defVal = Vector3(0, 0, 0) ) const { return defVal; }

	RED_INLINE void SetParameterF32( NxParameterized::Interface* params, const StringAnsi& name, Float val ) const {}
	RED_INLINE void SetParameterU32( NxParameterized::Interface* params, const StringAnsi& name, Uint32 val ) const {}
	RED_INLINE void SetParameterBool( NxParameterized::Interface* params, const StringAnsi& name, Bool val ) const {}
	RED_INLINE void SetParameterVec3( NxParameterized::Interface* params, const StringAnsi& name, const Vector3& val ) const {}
#endif

	/// Called while creating the Apex Asset object, allowing a subclass to modify the parameters given in the NxParameterized Interface.
	virtual void ConfigureNewAsset( NxParameterized::Interface* params ) const {}

#ifndef NO_EDITOR
	void RebuildPreviewAsset( Bool useDefaultParameters );
#endif

	void DestroyDefaults();
	void CreateDefaults();

	virtual void OnPropertyPostChange( IProperty* property );

public:

	/// Prepend a unique prefix to each material name, so that they are unique to the resource.
	static void AdjustApexMaterialNames( const String& namePrefix, NxParameterized::Interface* params, TDynArray< String >& outApexMaterialNames, const TDynArray< String >* namesToCheckAgainst = nullptr );
	/// Extract material names from NxParameterized. Similar to AdjustApexMaterialNames, except that params is not modified and names are not modified.
	static void ExtractApexMaterialNames( const NxParameterized::Interface* params, TDynArray< String >& outApexMaterialNames );


	static CApexResource* FromAsset( physx::apex::NxApexAsset* asset );


	CApexResource();
	virtual ~CApexResource();


	/// Get shadow distance for the resource.
	RED_INLINE Float GetShadowDistance() const { return m_shadowDistance < 0.0f ? m_autoHideDistance : Min( m_autoHideDistance, m_shadowDistance ); }

	/// Calculate a shadow distance for an apex component using this resource. If a distance has been explicitly set, then that
	/// will be returned. Otherwise, a default distance is calculated based on object size and world's cascade setup.
	Float CalcShadowDistance( CWorld* world, const Box& bounds ) const;


	virtual void AddRef();
	void ReleaseRef();
	Bool TryPreload( CompiledCollisionPtr& compiledBuffer );

#ifndef NO_EDITOR
	void AddAssetRef( physx::apex::NxApexAsset* asset );
	void ReleaseAssetRef( physx::apex::NxApexAsset* asset );
#else
	void AddAssetRef( physx::apex::NxApexAsset* asset ) {}
	void ReleaseAssetRef( physx::apex::NxApexAsset* asset ) {}
#endif

#ifndef NO_RESOURCE_COOKING

	virtual void OnCook( ICookerFramework& cooker ) override;

#endif

	virtual void OnPostLoad();
#ifndef NO_EDITOR
	virtual void OnSave();
#endif

	RED_INLINE const TDynArray< String >& GetApexMaterialNames() const { return m_apexMaterialNames; }

	class physx::apex::NxApexAsset* GetAsset() const { return m_savedAsset; }
#ifndef NO_EDITOR
	class physx::apex::NxApexAsset* GetPreviewAsset() const { return m_previewAsset; }
#endif

	virtual const char* GetAssetTypeName() { return nullptr; }

	/// Allows a subclass to restore any stored values to those given by default in the original imported Asset object.
	virtual void RestoreDefaults() {}

#ifndef NO_EDITOR
	/// Rebuild the preview asset. The saved asset will not be modified, so the new parameter values will only be visible where the
	/// preview asset is used.
	void RebuildPreviewAsset() { RebuildPreviewAsset( false ); }

	// Revert the preview asset to the saved copy. Restore properties to their last-saved values.
	void RevertPreviewAsset();
	virtual void FillStatistics( C2dArray* array );
	virtual void FillLODStatistics( C2dArray* array );
#endif

	virtual CompiledCollisionPtr CompileCollision( CObject* parent ) const override final;

	virtual void OnSerialize( IFile& file );

	void DecideToIncludeToCollisionCache();

};

BEGIN_ABSTRACT_CLASS_RTTI( CApexResource );
	PARENT_CLASS( CMeshTypeResource );
	PROPERTY(m_apexBinaryAsset);
	// When uncooked, this is regenerated on load. But when the resource is cooked, this keeps the final adjusted names.
	PROPERTY(m_apexMaterialNames);

	PROPERTY_EDIT(m_shadowDistance, TXT("Distance to render shadows. If < 0, will use a default value based on the object size. Shadows will be limited to autohide distance if this is larger."));
END_CLASS_RTTI();
