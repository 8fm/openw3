
-- Umbra command line interface for optimizer

umbraproject("process", { "Release", "Debug" }, "source")
folder "umbra3"
platforms { "x32", "x64" }
kind "ConsoleApp"
sourcedirs "source/process"
includedirs "source/common"
links { "optimizer" }
