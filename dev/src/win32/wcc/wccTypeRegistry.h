/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#ifndef _H_WCC_TYPE_REGISTRY
#define _H_WCC_TYPE_REGISTRY

// this file contains list of all types in 'core' project

// Not defined when included in wcc/build.h, but defined when included in wccClasses.cpp
#if !defined( REGISTER_RTTI_TYPE )
#define REGISTER_RTTI_TYPE( _className ) RED_DECLARE_RTTI_NAME( _className ) template<> struct TTypeName< _className >{ static const CName& GetTypeName() { return CNAME( _className ); } };
#define REGISTER_NOT_REGISTERED
#endif

#define REGISTER_RTTI_CLASS( _className ) class _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_STRUCT( _className ) struct _className; REGISTER_RTTI_TYPE( _className );
#define REGISTER_RTTI_ENUM( _className ) enum _className; REGISTER_RTTI_TYPE( _className );

REGISTER_RTTI_CLASS( CWccDummyGame );

REGISTER_RTTI_CLASS( CEntityTemplateCooker );

REGISTER_RTTI_CLASS( ICacheBuilder );
REGISTER_RTTI_CLASS( CTextureCacheBuilder );
REGISTER_RTTI_CLASS( CShaderCacheBuilder );
REGISTER_RTTI_CLASS( CPhysicsCacheBuilder );

REGISTER_RTTI_CLASS( IBaseCacheSplitter );
REGISTER_RTTI_CLASS( CPhysicsCacheSplitter );
REGISTER_RTTI_CLASS( CTextureCacheSplitter );

REGISTER_RTTI_CLASS( IBasePatchContentBuilder );
REGISTER_RTTI_CLASS( CPatchBuilder_Bundles );
REGISTER_RTTI_CLASS( CPatchBuilder_Strings );
REGISTER_RTTI_CLASS( CPatchBuilder_Speeches );
REGISTER_RTTI_CLASS( CPatchBuilder_Physics );
REGISTER_RTTI_CLASS( CPatchBuilder_TextureCache );
REGISTER_RTTI_CLASS( CPatchBuilder_Shaders );
REGISTER_RTTI_CLASS( CPatchBuilder_StaticShaders );
REGISTER_RTTI_CLASS( CPatchBuilder_FurShaders );

REGISTER_RTTI_CLASS( CWorldFileAnalyzer );
#ifndef WCC_LITE
REGISTER_RTTI_CLASS( CExtraDLCAnalyzer );
#endif

REGISTER_RTTI_CLASS( CAnalyzeCommandlet );
REGISTER_RTTI_CLASS( CBundleMetadataStoreCommandlet );
REGISTER_RTTI_CLASS( CCookCommandlet );
REGISTER_RTTI_CLASS( CFileVersionCommandlet );
REGISTER_RTTI_CLASS( CCacheBuilderCommandlet );
REGISTER_RTTI_CLASS( CCacheSplitterCommandlet );
REGISTER_RTTI_CLASS( CExportBundlesCommandlet );
REGISTER_RTTI_CLASS( CMaterialCookerCommandlet );
REGISTER_RTTI_CLASS( CResaveCommandlet );
REGISTER_RTTI_CLASS( CSoundCommandlet );
REGISTER_RTTI_CLASS( CStringCookerCommandlet );
REGISTER_RTTI_CLASS( CSwfImportCommandlet );
REGISTER_RTTI_CLASS( CSwfDumpCommandlet );
REGISTER_RTTI_CLASS( CTestMemCommandlet );
REGISTER_RTTI_CLASS( CVoiceoverConversionCommandlet );
REGISTER_RTTI_CLASS( CSplitFilesCommandlet );
REGISTER_RTTI_CLASS( CCookSubtitlesCommandlet );
REGISTER_RTTI_CLASS( CQuestLayoutDumpCommandlet );
REGISTER_RTTI_CLASS( CFileDumpCommandlet );
REGISTER_RTTI_CLASS( CDialogSceneLinesCommandlet );
REGISTER_RTTI_CLASS( CLoadTestCommandlet );
REGISTER_RTTI_CLASS( CVideoEncoderCommandlet );
REGISTER_RTTI_CLASS( CUnbundlerCommandlet );
REGISTER_RTTI_CLASS( CR4ExtractCharacters );
REGISTER_RTTI_CLASS( CGlueFilesCommandlet );
REGISTER_RTTI_CLASS( CR4ExtractCharactersDLC );
REGISTER_RTTI_CLASS( CGlueFilesDLCCommandlet );
REGISTER_RTTI_CLASS( CPatchCommandlet );
REGISTER_RTTI_CLASS( CPackCommandlet );
REGISTER_RTTI_CLASS( CImportCommandlet );
REGISTER_RTTI_CLASS( CExportCommandlet );
REGISTER_RTTI_CLASS( CUncookCommandlet );


#ifndef WCC_LITE
REGISTER_RTTI_CLASS( CAnimationValidatorCommandlet );
REGISTER_RTTI_CLASS( CScriptsHashCommandlet );
#endif


#undef REGISTER_RTTI_CLASS
#undef REGISTER_RTTI_STRUCT
#undef REGISTER_RTTI_ENUM

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NOT_REGISTERED
#endif

#endif
