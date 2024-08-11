
-- Umbra validator application

umbraproject("qatool", nil, "dev")
folder "utils"
kind "ConsoleApp"
platforms { "x32", "x64" }
sourcedirs "utils/qatool"
includedirs { "source/optimizer", "source/runtime", "source/common", "utils/testdb" }
links { "runtime", "testdb", "common" }

configuration { "x32 or x64" }
links { "optimizer", "optimizer_static" }
