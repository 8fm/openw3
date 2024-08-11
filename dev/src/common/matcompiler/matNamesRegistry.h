#ifndef _H_MAT_NAMES_REGISTRY
#define _H_MAT_NAMES_REGISTRY

#if !defined( REGISTER_NAME )
#define REGISTER_NAME( name_ ) RED_DECLARE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DECLARE_NAMED_NAME( varname_, string_ )
#define REGISTER_NOT_REGISTERED
#endif

#if defined( REGISTER_NOT_REGISTERED )
#undef REGISTER_NAME
#undef REGISTER_NAMED_NAME
#undef REGISTER_NOT_REGISTERED
#endif

#endif
