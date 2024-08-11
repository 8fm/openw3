
-- Umbra runtime library

spursjobs = { "portal", "connectivity", "frustum" }

-- Standard version, PPU wrapper on PS3
umbraproject("runtime", { "Release", "Debug" }, { "source", "rt-source" })
folder "umbra3"
kind "StaticLib"
includedirs  { "interface/runtime", "source/common" }
sourcedirs { "source/runtime" }
files { "interface/runtime/*.hpp" }
if hasplatform("xbox360") then sourcedirs "source/runtime/xbox360" end
if hasplatform("ps3") then 
	includedirs "source/runtime/spursruntime" 
	files "source/runtime/ps3/umbraSpuQueryWrapper.cpp"
	files "source/runtime/ps3/umbraSIMD_PS3PPU.hpp"
end
-- TODO: a hack to include NEON SIMD implementation in source package. Should only
-- include on Neon platforms instead of x32|x64
if hasplatform("x32") or hasplatform("x64") then sourcedirs "source/runtime/neon" end
-- links { "common" }

configuration "ps3"
for _, job in pairs(spursjobs) do
	links("spursruntime_" .. job)
end

-- do not inject iterator debug level linker directive,
-- makes runtime library not link in debug builds
configuration { "windows", "x32 or x64" }
defines "_ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH"


-- SPU version
umbraproject("spuruntime", "Release", "source", "spursjob")
folder "umbra3"
platforms "ps3"
kind "StaticLib"
flags "OptimizeSpeed"
sourcedirs { "source/runtime" }
includedirs { "interface/runtime", "source/runtime/ps3", "source/common" }
files "source/runtime/ps3/umbraSIMD_PS3SPU.hpp"

-- SPURS jobs

for _, job in pairs(spursjobs) do
	umbraproject("spursruntime_" .. job, nil, "source", "spursjob")
	folder "umbra3"
	platforms "ps3"
	kind "ConsoleApp"
	links { "spuruntime", "spucommon" }
	files("source/runtime/ps3/umbraspursruntime_" .. job ..".cpp")
	files("source/runtime/ps3/umbraQueryJob.hpp")
	includedirs { "interface/runtime", "source/runtime", "source/runtime/ps3", "source/common" }
end
