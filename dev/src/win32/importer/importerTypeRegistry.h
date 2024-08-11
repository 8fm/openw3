/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_IMPORTER_TYPE_REGISTRY
#define _H_IMPORTER_TYPE_REGISTRY

// this file contains list of all types in 'core' project


// Not defined when included in importer/build.h, but defined when included in build.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );

REGISTER_RTTI_CLASS( CBitmapImporter );
REGISTER_RTTI_CLASS( CREAAnimImporter );
REGISTER_RTTI_CLASS( CREARigImporter );
REGISTER_RTTI_CLASS( CFACMimicFaceImporter );
REGISTER_RTTI_CLASS( CFACRigImporter );
REGISTER_RTTI_CLASS( CDyngImporter );

REGISTER_RTTI_CLASS( CREAAnimExporter );

#ifdef USE_APEX
REGISTER_RTTI_CLASS( CApexImporter );
REGISTER_RTTI_CLASS( CApexClothImporter );
REGISTER_RTTI_CLASS( CApexDestructionImporter );
#endif
REGISTER_RTTI_CLASS( CTTFFontImporter );
REGISTER_RTTI_CLASS( CFlashImporter );
//REGISTER_RTTI_CLASS( CSRTImporter );
REGISTER_RTTI_CLASS( CCutsceneImporter );

/************************************************************************/
/*																		*/
/* LMF/REM/MMM/SUX importers commented out.								*/
/* It was made to unify importing assets from 3ds max by RE exporter	*/
/* (REXMesh). Right now asset with verts and indices and faces limits	*/
/* per material chunk should be prepare by artist in 3ds max			*/
/* instead of cutting it in engine. It was done for more control		*/
/* in bounding boxes of chunks, for better occlusion and performance.	*/
/*																		*/
/* still need to comment out NIF and HKX								*/
/*																		*/
/************************************************************************/

// Mesh
REGISTER_RTTI_CLASS( CREXMeshImporter );
REGISTER_RTTI_CLASS( CFBXMeshImporter );
REGISTER_RTTI_CLASS( CFBXMeshExporter );
REGISTER_RTTI_CLASS( CFBXEntityExporter );
REGISTER_RTTI_CLASS( CFbxImporter );
REGISTER_RTTI_CLASS( CHairWorksImporter );
REGISTER_RTTI_CLASS( CHairWorksExporter );
REGISTER_RTTI_CLASS( CDestructionImporter );

REGISTER_RTTI_CLASS( CRepXImporter );

//REGISTER_RTTI_CLASS( CWorldMapImporter );

// Exporters
REGISTER_RTTI_CLASS( CBitmapExporter );
REGISTER_RTTI_CLASS( CMesh3DSExporter );

// Factories
REGISTER_RTTI_CLASS( CAnimSetFactory );
REGISTER_RTTI_CLASS( CEnvironmentDefinitionFactory );
REGISTER_RTTI_CLASS( CTextureArrayFactory );
REGISTER_RTTI_CLASS( CBehaviorGraphFactory );
REGISTER_RTTI_CLASS( CCubeTextureFactory );
REGISTER_RTTI_CLASS( CEntityTemplateFactory );
REGISTER_RTTI_CLASS( CCharacterEntityTemplateFactory );
REGISTER_RTTI_CLASS( CGenericMaterialFactory );
REGISTER_RTTI_CLASS( CMaterialInstanceFactory );
REGISTER_RTTI_CLASS( CParticleSystemFactory );
REGISTER_RTTI_CLASS( C2dArrayFactory );
REGISTER_RTTI_CLASS( CSimplexTreeFactory );
REGISTER_RTTI_CLASS( CDLCDefinitionFactory );

// Thumbnails
REGISTER_RTTI_CLASS( CBitmapThumbnailGenerator );
REGISTER_RTTI_CLASS( CMeshThumbnailGenerator );
REGISTER_RTTI_CLASS( CEntityTemplateThumbnailGenerator );
REGISTER_RTTI_CLASS( CMaterialTemplateThumbnailGenerator );
REGISTER_RTTI_CLASS( CSpeedTreeThumbnailGenerator );
REGISTER_RTTI_CLASS( CFlashThumbnailGenerator );

#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
