/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _EXTERN_DEFINITIONS_H_
#define _EXTERN_DEFINITIONS_H_

// This is essentially a hack. We need to register certain classes with the engine as they exist in the editor
// but are called from scripts
void InitialiseExternalClasses();

#endif