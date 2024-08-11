
#pragma once

class CReslinkerVersionControl : public ISourceControl
{
	virtual Bool Delete( CDiskFile &resource, Bool confirm = true)
	{
		resource.SetLocal();
		return resource.Delete();
	}
};