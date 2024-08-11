
-- Umbra testing utilities

umbraproject("testdb", nil, "source")
folder "utils"
kind "StaticLib"
sourcedirs "utils/testdb"
includedirs { "source/common" }
