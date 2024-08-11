/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

// Removes warnings caused by wxWidget's use of standard cstring functions without window's _s suffix
#define _CRT_SECURE_NO_WARNINGS 1

// Removes warnings in stl caused by turning off exceptions
#define _HAS_EXCEPTIONS 0

#include "../../common/redSystem/os.h"
#include "../../common/redSystem/error.h"

// STL
#include <vector>
#include <string>
#include <deque>
#include <algorithm>
#include <list>

#pragma warning(push)
#pragma warning(disable:4702) // c:\program files (x86)\microsoft visual studio 11.0\vc\include\xtree(1829): warning C4702: unreachable code
#include <map>
#pragma warning(pop)

using namespace std;

template< class T >
void ClearVector( vector<T>& theMap )
{
	for ( vector<T>::iterator i=theMap.begin(); i!=theMap.end(); ++i )
	{
		delete *i;
	}
	theMap.clear();
}

template <class T> 
T Max(const T& a, const T& b)
{
	return ( a >=b ) ? a : b;
}

template <class T>
T Min(const T& a, const T& b)
{
	return ( a <= b ) ? a : b;
}

// Disable some warnings
#pragma warning (disable: 4100 )		// warning C4100: 'id' : unreferenced formal parameter

#ifdef wxUSE_STC
#	undef wxUSE_STC
#endif

#define wxUSE_STC 1

// Red system (replace it with core.h after fixing memory allocation dependencies)
// IMPORTANT - Do NOT make script studio dependent on core!
#include "../../common/redSystem/compilerExtensions.h"

#include <um\urlmon.h>

// Widgets
#include "wx/wxprec.h"
#include "wx/aui/aui.h"
#include "wx/panel.h"
#include "wx/sizer.h"
#include "wx/frame.h"
#include "wx/xrc/xmlres.h"
#include "wx/splitter.h"
#include "wx/statusbr.h"
#include "wx/menu.h"
#include "wx/treectrl.h"
#include "wx/toolbar.h"
#include "wx/spinctrl.h"
#include "wx/combobox.h"
#include "wx/image.h"
#include "wx/fileconf.h"
#include "wx/listctrl.h"
#include "wx/stdpaths.h"
#include "wx/listbox.h"
#include "wx/stc/stc.h"
#include "wx/stc/private.h"
#include "wx/cmdline.h"
#include "wx/clrpicker.h"
#include "wx/fontpicker.h"
#include "wx/listbook.h"
#include "wx/popupwin.h"
#include "wx/timer.h"
#include "wx/regex.h"
#include "wx/treelistctrl.h"
#include "wx/wfstream.h"
#include "wx/dir.h"