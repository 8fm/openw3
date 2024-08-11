/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_IO_H
#define RED_IO_H
#pragma once

#include "redIOCommon.h"
#include "redIOAsyncReadToken.h"
#include "redIOFile.h"
#include "redIOAsyncIO.h"

REDIO_NAMESPACE_BEGIN

Bool Initialize();
void Shutdown();

REDIO_NAMESPACE_END

#endif // RED_IO_H
