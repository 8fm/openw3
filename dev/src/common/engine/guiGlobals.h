/**
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#ifndef NO_LOG

#define GUI_LOG( format, ... )						RED_LOG( Gui, format, ## __VA_ARGS__ )
#define GUI_WARN( format, ... )						RED_LOG_WARNING( Gui, format, ## __VA_ARGS__ )
#define GUI_WARN_UNTRANSLATED_TEXT( format, ... )	RED_LOG_WARNING( GuiUntranslated, format, ## __VA_ARGS__ )
#define GUI_ERROR( format, ... )					RED_LOG_ERROR( Gui, format, ## __VA_ARGS__ )
#define GUI_SCRIPT_WARN( format, ... )				RED_LOG_WARNING( Gui, format, ## __VA_ARGS__ )

// For now after new log merge...
#define GUI_SCRIPT_WARN_ONCE( stack, txt, ... )		RED_LOG_WARNING( Gui, txt, ## __VA_ARGS__ )
// #define GUI_SCRIPT_WARN_ONCE( stack, txt, ... ) if ( GLog ) {										\
// 	static TSortedSet< Uint64 > warnedAlready; 														\
// union 																								\
// 	{																								\
// 	Uint64	word; 																					\
// struct 																								\
// 		{ 																							\
// 		const void* ptr1; 																			\
// 		const void*	ptr2; 																			\
// 		}; 																							\
// 	} id; 																							\
// 	id.ptr1	= stack.m_function; 																	\
// 	id.ptr2 = reinterpret_cast< void* > ( stack.m_line ); 											\
// 	if ( false == warnedAlready.Exist( id.word ) ) 													\
// 	{ 																								\
// 	warnedAlready.Insert( id.word ); 																\
// 	SLog::GetInstance().Log( CNAME( Gui ), txt, __VA_ARGS__ );				 						\
// 	SCRIPT_DUMP_STACK_TO_LOG( stack );																\
// 	} 																								\
// }

#else

#define GUI_LOG( format, ... ) 
#define GUI_WARN( format, ... )
#define GUI_WARN_UNTRANSLATED_TEXT( format, ... )
#define GUI_ERROR( format, ... )
#define GUI_SCRIPT_WARN( format, ... )
#define GUI_SCRIPT_WARN_ONCE( stack, txt, ... )

#endif