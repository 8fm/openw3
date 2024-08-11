/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_MACRO_UTILS_H_
#define _RED_MEMORY_MACRO_UTILS_H_

#define RED_MEMORY_EMPTY()
#define RED_MEMORY_OPTIONAL_VAR_ARGS( ... ) , __VA_ARGS__, RED_MEMORY_EMPTY()
#define RED_MEMORY_CONCAT_APPLY( left, right ) left##right
#define RED_MEMORY_CONCAT( left, right ) RED_MEMORY_CONCAT_APPLY( left, right )

#define RED_MEMORY_VARIADIC_POSTFIX(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N 

#if defined( RED_COMPILER_MSC )
	#define RED_MEMORY_VARIADIC_SIZE(...) RED_MEMORY_CONCAT( RED_MEMORY_VARIADIC_POSTFIX(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, ), )
#else
	#define RED_MEMORY_VARIADIC_SIZE(...) RED_MEMORY_VARIADIC_POSTFIX(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, )
#endif

#define RED_MEMORY_OVERLOAD( prefix, ... ) RED_MEMORY_CONCAT( prefix, RED_MEMORY_VARIADIC_SIZE( __VA_ARGS__  ) )

#endif
