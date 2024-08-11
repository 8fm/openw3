#pragma once

#ifndef RED_FINAL_BUILD
#define _DBG_ONLY_CODE_( ... ) __VA_ARGS__
#else
#define _DBG_ONLY_CODE_( ... )
#endif