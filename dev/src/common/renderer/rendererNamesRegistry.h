#ifndef _H_RENDERER_NAMES_REGISTRY
#define _H_RENDERER_NAMES_REGISTRY

#if !defined( REGISTER_NAME )
#define REGISTER_NAME( name_ ) RED_DECLARE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DECLARE_NAMED_NAME( varname_, string_ )
#define REGISTER_NOT_REGISTERED
#endif

#ifndef RED_FINAL_BUILD
REGISTER_NAME( CharacterEmissive );
REGISTER_NAME( CharacterNormalmapGloss );
REGISTER_NAME( HeadDiffuse );
REGISTER_NAME( HeadDiffuseWithAlpha );
REGISTER_NAME( HeadNormal );
#endif
REGISTER_NAME( RenderFurMesh )

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_NAME
#undef REGISTER_NAMED_NAME
#undef REGISTER_NOT_REGISTERED
#endif

#endif // _H_RENDERER_NAMES_REGISTRY