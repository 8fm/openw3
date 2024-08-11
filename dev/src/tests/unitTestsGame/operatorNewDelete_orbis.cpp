#include "build.h"

#ifdef RED_PLATFORM_ORBIS

// PS4 has its own hooks for global new / delete because... Sony. 
// Note, it MUST be linked to main (but not defined in the main object file) to link correctly to other PRXs, CRT, etc

	#define OP_NEW user_new
	#define OP_NEW_ARRAY user_new_array
	#define OP_DELETE user_delete
	#define OP_DELETE_ARRAY user_delete_array
	#define THROWS_BAD_ALLOC throw(std::bad_alloc)
	#include "../../common/core/operatorNewDelete.inl"

#endif