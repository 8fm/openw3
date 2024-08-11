/**
* Copyright © 2007 CDProjekt Red, Inc. All Rights Reserved.
*
* This file contains operating system functions
*/
#pragma once

void SInitializePlatform( const wchar_t* commandLine );
void SShutdownPlatform();

Bool SPumpMessages();

void SInitializeVersionControl();
