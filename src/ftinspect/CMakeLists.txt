cmake_minimum_required (VERSION 3.0)
cmake_policy(SET CMP0074 NEW)
set(CMAKE_CXX_STANDARD 11)

project("ftinspect")

if (WIN32)
  add_compile_options("/utf-8")
endif ()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Freetype REQUIRED)

add_executable(ftinspect
  "ftinspect.cpp"
  "maingui.cpp"
  "uihelper.cpp"
  
  "engine/engine.cpp"
  "engine/fontfilemanager.cpp"

  "rendering/glyphbitmap.cpp"
  "rendering/glyphoutline.cpp"
  "rendering/glyphpointnumbers.cpp"
  "rendering/glyphpoints.cpp"
  "rendering/grid.cpp"

  "widgets/customwidgets.cpp"
  "widgets/glyphindexselector.cpp"

  "models/ttsettingscomboboxmodel.cpp"

  "panels/settingpanel.cpp"
  "panels/singular.cpp"
)
target_link_libraries(ftinspect
  Qt5::Core Qt5::Widgets
  Freetype::Freetype
)
