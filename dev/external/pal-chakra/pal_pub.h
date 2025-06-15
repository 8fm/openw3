#pragma once

extern "C" {

// pal/inc/pal_mstypes.h
#define PALIMPORT

//typedef int LONG;       // NOTE: diff from windows.h, for LP64 compat
//typedef unsigned int ULONG; // NOTE: diff from windows.h, for LP64 compat
//typedef long LONGLONG;
typedef char16_t WCHAR;

// pal/inc/pal.h
PALIMPORT int __cdecl memcpy_s(void *, size_t, const void *, size_t);
PALIMPORT int __cdecl memmove_s(void *, size_t, const void *, size_t);
PALIMPORT void * __cdecl memmove_xplat(void *, const void *, size_t);
PALIMPORT char * __cdecl _strlwr(char *);
PALIMPORT int __cdecl _stricmp(const char *, const char *);
PALIMPORT int __cdecl _snprintf(char *, size_t, const char *, ...);
PALIMPORT char * __cdecl _gcvt_s(char *, int, double, int);
PALIMPORT char * __cdecl _ecvt(double, int, int *, int *);
PALIMPORT int __cdecl __iscsym(int);
PALIMPORT size_t __cdecl _mbslen(const unsigned char *);
PALIMPORT unsigned char * __cdecl _mbsinc(const unsigned char *);
PALIMPORT unsigned char * __cdecl _mbsninc(const unsigned char *, size_t);
PALIMPORT unsigned char * __cdecl _mbsdec(const unsigned char *, const unsigned char *);
PALIMPORT int __cdecl _wcsicmp(const WCHAR *, const WCHAR*);
PALIMPORT int __cdecl _wcsnicmp(const WCHAR *, const WCHAR *, size_t);
PALIMPORT int __cdecl _vsnprintf(char *, size_t, const char *, va_list);
PALIMPORT int __cdecl _vsnwprintf(WCHAR *, size_t, const WCHAR *, va_list);
PALIMPORT WCHAR * __cdecl _itow(int, WCHAR *, int);
PALIMPORT WCHAR * __cdecl _ltow(long, WCHAR *, int);

PALIMPORT size_t __cdecl PAL_wcslen(const WCHAR *);
PALIMPORT int __cdecl PAL_wcscmp(const WCHAR*, const WCHAR*);
PALIMPORT int __cdecl PAL_wmemcmp(const WCHAR *, const WCHAR *, size_t);
PALIMPORT int __cdecl PAL_wcsncmp(const WCHAR *, const WCHAR *, size_t);
PALIMPORT WCHAR * __cdecl PAL_wcscat(WCHAR *, const WCHAR *);
PALIMPORT WCHAR * __cdecl PAL_wcsncat(WCHAR *, const WCHAR *, size_t);
PALIMPORT WCHAR * __cdecl PAL_wcscpy(WCHAR *, const WCHAR *);
PALIMPORT WCHAR * __cdecl PAL_wcsncpy(WCHAR *, const WCHAR *, size_t);
PALIMPORT const WCHAR * __cdecl PAL_wcschr(const WCHAR *, WCHAR);
PALIMPORT const WCHAR * __cdecl PAL_wcsrchr(const WCHAR *, WCHAR);
//PALIMPORT WCHAR _WConst_return * __cdecl PAL_wcspbrk(const WCHAR *, const WCHAR *);
PALIMPORT WCHAR const * __cdecl PAL_wcspbrk(const WCHAR *, const WCHAR *);
//PALIMPORT WCHAR _WConst_return * __cdecl PAL_wcsstr(const WCHAR *, const WCHAR *);
PALIMPORT WCHAR const * __cdecl PAL_wcsstr(const WCHAR *, const WCHAR *);
PALIMPORT WCHAR * __cdecl PAL_wcstok(WCHAR *, const WCHAR *);
PALIMPORT size_t __cdecl PAL_wcscspn(const WCHAR *, const WCHAR *);
PALIMPORT int __cdecl PAL_swprintf(WCHAR *, const WCHAR *, ...);
PALIMPORT int __cdecl PAL_vswprintf(WCHAR *, const WCHAR *, va_list);
PALIMPORT int __cdecl _snwprintf(WCHAR *, size_t, const WCHAR *, ...);
PALIMPORT int __cdecl PAL_swscanf(const WCHAR *, const WCHAR *, ...);
//PALIMPORT LONG __cdecl PAL_wcstol(const WCHAR *, WCHAR **, int);
PALIMPORT int __cdecl PAL_wcstol(const WCHAR *, WCHAR **, int);
//PALIMPORT ULONG __cdecl PAL_wcstoul(const WCHAR *, WCHAR **, int);
PALIMPORT unsigned int __cdecl PAL_wcstoul(const WCHAR *, WCHAR **, int);
//PALIMPORT LONGLONG __cdecl PAL_wcstoll(const WCHAR *, WCHAR **, int);
PALIMPORT long __cdecl PAL_wcstoll(const WCHAR *, WCHAR **, int);
PALIMPORT size_t __cdecl PAL_wcsspn (const WCHAR *, const WCHAR *);
PALIMPORT double __cdecl PAL_wcstod(const WCHAR *, WCHAR **);
PALIMPORT int __cdecl PAL_iswalpha(WCHAR);
PALIMPORT int __cdecl PAL_iswprint(WCHAR);
PALIMPORT int __cdecl PAL_iswupper(WCHAR);
PALIMPORT int __cdecl PAL_iswspace(WCHAR);
PALIMPORT int __cdecl PAL_iswdigit(WCHAR);
PALIMPORT int __cdecl PAL_iswxdigit(WCHAR);
PALIMPORT WCHAR __cdecl PAL_towlower(WCHAR);
PALIMPORT WCHAR __cdecl PAL_towupper(WCHAR);

extern "C++" {
inline WCHAR *PAL_wcschr(WCHAR *_S, WCHAR _C)
        {return ((WCHAR *)PAL_wcschr((const WCHAR *)_S, _C)); }
inline WCHAR *PAL_wcsrchr(WCHAR *_S, WCHAR _C)
        {return ((WCHAR *)PAL_wcsrchr((const WCHAR *)_S, _C)); }
inline WCHAR *PAL_wcspbrk(WCHAR *_S, const WCHAR *_P)
        {return ((WCHAR *)PAL_wcspbrk((const WCHAR *)_S, _P)); }
inline WCHAR *PAL_wcsstr(WCHAR *_S, const WCHAR *_P)
        {return ((WCHAR *)PAL_wcsstr((const WCHAR *)_S, _P)); }
}

// pal/inc/rt/safecrt.h
#if !defined(_TRUNCATE)
#define _TRUNCATE ((size_t)-1)
#endif

int __cdecl strncpy_s(char *_Dst, size_t _SizeInBytes, const char *_Src, size_t _Count);
int __cdecl wcsncpy_s(WCHAR *_Dst, size_t _SizeInWords, const WCHAR *_Src, size_t _Count);
int __cdecl strncat_s(char *_Dst, size_t _SizeInBytes, const char *_Src, size_t _Count);
int __cdecl wcsncat_s(WCHAR *_Dst, size_t _SizeInWords, const WCHAR *_Src, size_t _Count);

int __cdecl vsprintf_s(char *_Dst, size_t _SizeInBytes, const char *_Format, va_list _ArgList);
int __cdecl vswprintf_s(WCHAR *_Dst, size_t _SizeInWords, const WCHAR *_Format, va_list _ArgList);
int __cdecl _vsnprintf_s(char *_Dst, size_t _SizeInBytes, size_t _Count, const char *_Format, va_list _ArgList);
int __cdecl _vsnwprintf_s(WCHAR *_Dst, size_t _SizeInWords, size_t _Count, const WCHAR *_Format, va_list _ArgList);

}
