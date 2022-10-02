cmake_minimum_required(VERSION 3.0)
cmake_policy(SET CMP0074 NEW)

set(CMAKE_CXX_STANDARD 11)

project("ftinspect")

if (MSVC)
  add_compile_options("/utf-8")
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 COMPONENTS Widgets REQUIRED)
find_package(Freetype REQUIRED)

add_executable(ftinspect
  "engine/charmap.cpp"
  "engine/engine.cpp"
  "engine/fontfilemanager.cpp"
  "engine/fontinfo.cpp"
  "engine/fontinfonamesmapping.cpp"
  "engine/mmgx.cpp"
  "engine/paletteinfo.cpp"
  "engine/rendering.cpp"
  "engine/stringrenderer.cpp"

  "glyphcomponents/glyphbitmap.cpp"
  "glyphcomponents/glyphcontinuous.cpp"
  "glyphcomponents/glyphoutline.cpp"
  "glyphcomponents/glyphpointnumbers.cpp"
  "glyphcomponents/glyphpoints.cpp"
  "glyphcomponents/graphicsdefault.cpp"
  "glyphcomponents/grid.cpp"

  "models/customcomboboxmodels.cpp"
  "models/fontinfomodels.cpp"

  "panels/comparator.cpp"
  "panels/continuous.cpp"
  "panels/glyphdetails.cpp"
  "panels/info.cpp"
  "panels/settingpanel.cpp"
  "panels/settingpanelmmgx.cpp"
  "panels/singular.cpp"

  "widgets/charmapcombobox.cpp"
  "widgets/customwidgets.cpp"
  "widgets/fontsizeselector.cpp"
  "widgets/glyphindexselector.cpp"
  "widgets/tripletselector.cpp"

  "ftinspect.cpp"
  "maingui.cpp"
  "uihelper.cpp"
)

target_link_libraries(ftinspect
  Freetype::Freetype
  Qt5::Core
  Qt5::Widgets
)

# Fix for CMake prior to 3.15
string(REGEX REPLACE
  "/W[3|4]" ""
  CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if (MSVC)
  target_compile_options(ftinspect
    PRIVATE "/W4" "/wd4100")
else (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
      OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(ftinspect
    PRIVATE "-Wall" "-Wextra" "-Wpedantic" "-Wno-unused-parameter")
endif ()
