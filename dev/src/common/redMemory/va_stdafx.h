/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */


// DO NOT INCLUDE THIS FILE. IT IS ONLY FOR VISUAL ASSIST PARSING OF MEMORY MACRO.

#define RED_NEW( type, ... ) new type
#define RED_NEW_ARRAY( type, count, ... ) new type[ count ]

#define RED_DELETE( ptr, ... ) delete ptr
#define RED_DELETE_ARRAY( ptr, count, ... ) delete [] ptr
