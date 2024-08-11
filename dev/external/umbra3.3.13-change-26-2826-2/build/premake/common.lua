
-- Umbra common library

umbraproject("common", { "Release", "Debug" }, { "source", "rt-source" })
kind "StaticLib"
sourcedirs { "source/common", "source/standard" }
files { "interface/*.hpp" }
includedirs { "external/uuid" }

configuration "windows"
sourcedirs "source/common/windows"

if hasplatform("ps3") then sourcedirs "source/common/cell" end
if hasplatform("psvita") then sourcedirs "source/common/psvita" end
if hasplatform("cafe") then sourcedirs "source/common/cafe" end
if hasplatform("orbis") then sourcedirs "source/common/orbis" end
if hasplatform("posix") then sourcedirs "source/common/posix" end

configuration { "windows", "x32" }
includedirs "external/curl/win32/include"

configuration { "windows", "x64" }
includedirs "external/curl/win64/include"

configuration "linux or macosx or ios_*"
sourcedirs "source/common/posix"
excludes { "source/common/umbraProcessWin32.cpp",
           "source/common/umbraWin32Thread.inl" }
defines { "HAVE_NANOSLEEP" }
sourcedirs "external/uuid"

configuration { "linux", "x32" }
includedirs "external/curl/linux32/include"
configuration { "linux", "x64" }
includedirs "external/curl/linux64/include"

-- SPU version
umbraproject("spucommon", "Release", "source", "spursjob")
folder "umbra3"
platforms "ps3"
kind "StaticLib"
flags "OptimizeSpeed"
sourcedirs { "source/common", "source/common/cell" }
-- includedirs { "interface/runtime", "source/runtime/ps3", "source/common" }
-- files "source/runtime/ps3/umbraSIMD_PS3SPU.hpp"
