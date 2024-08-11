/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define DEBUG_TASKS_LEVEL_HIGH    3
#define DEBUG_TASKS_LEVEL_MEDIUM  2
#define DEBUG_TASKS_LEVEL_LOW     1
#define DEBUG_TASKS_LEVEL_NONE    0

#ifndef RED_FINAL_BUILD
# ifdef _DEBUG
#  define DEBUG_TASKS_LEVEL DEBUG_TASKS_LEVEL_HIGH
# else
#  define DEBUG_TASKS_LEVEL DEBUG_TASKS_LEVEL_NONE
# endif // _DEBUG
#else
# define DEBUG_TASKS_LEVEL  DEBUG_TASKS_LEVEL_NONE
#endif // ! RED_FINAL_BUILD

//#define DEBUG_TASKS_PUT_TASK_ON_LOCAL_QUE