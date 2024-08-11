//--------------------------------------------------------------------------------------
// ErrorMsg.h
// 
//   Common error message handling functions
//
//--------------------------------------------------------------------------------------
// (C) 2005 ATI Research, Inc., All rights reserved.
//--------------------------------------------------------------------------------------
#ifndef ERRORMSG_H
#define ERRORMSG_H 

//#include <stdarg.h>

#ifndef WCHAR
#define WCHAR wchar_t
#endif

#define EM_EXIT_NO_ERROR                  0
#define EM_EXIT_NONFATAL_ERROR_OCCURRED  -1
#define EM_FATAL_ERROR                  -15


//set error function
void SetErrorMessageCallback( void(*a_MessageOutputFunc)(WCHAR *, WCHAR *) );


#endif //ERRORMSG_H 
