-- A solution contains projects, and defines the available configurations
solution "Pyrite"
  configurations { "Debug", "Release" }
 
   -- A project defines one build target
  project "Pyrite"
    kind "SharedLib"
    language "C++"
    files { "*.h", "*.cpp" }
    links{'OpenCL', 'gts', 'noise','glib-2.0'}
    buildoptions {"`pkg-config --libs --cflags glib-2.0` " } ---Wall -Wextra
    configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols" }
 
    configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }
  project "Sample"
    kind "ConsoleApp"
    language "C++"
    files {"Samples/*.h", "Samples/*.cpp"}
    includedirs {"."}
    links{'Pyrite', 'OpenCL', 'gts', 'noise','glib-2.0'}
    buildoptions {"`pkg-config --libs --cflags glib-2.0` " } ---Wall -Wextra
    configuration "Debug"
      defines { "DEBUG" }
      flags { "Symbols" }

    configuration "Release"
      defines { "NDEBUG" }
      flags { "Optimize" }

  newaction {
    trigger     = "install",
    description = "Install the library",
    execute = function ()

      os.execute("sudo mkdir -p /usr/local/include/pyrite/")
      os.execute("sudo cp *.h /usr/local/include/pyrite/")
      os.execute("sudo cp *.so /usr/local/lib/")
    end
  }
