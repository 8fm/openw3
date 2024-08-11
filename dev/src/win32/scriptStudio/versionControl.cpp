#include "build.h"

ISourceControl *GVersionControl;

void SInitializeVersionControl()
{
	GVersionControl = new ISourceControl;
}