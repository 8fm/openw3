
#include "build.h"

void SInitializeVersionControl()
{
	GVersionControl = new CReslinkerVersionControl;

	//CVersionControlInterface *iface = new CVersionControlInterface;
	//GVersionControl = new CSourceControlP4( iface );
}