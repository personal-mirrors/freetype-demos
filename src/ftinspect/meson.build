#
# Meson project file for the ftinspect program.
#

# Copyright (C) 2021-2022 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

qt5 = import('qt5')
qt5_dep = dependency('qt5',
  required: false,
  modules: ['Core', 'Gui', 'Widgets'],
  version : '>=5.15')

# Don't compile `ftinspect` if Qt5 is not found.  This can happen
# when cross-compiling to a different system.
if qt5_dep.found()
  sources = files([
    'engine/engine.cpp',
    'engine/fontfilemanager.cpp',
    'engine/rendering.cpp',
    'engine/paletteinfo.cpp',
    'engine/mmgx.cpp',
    'engine/fontinfo.cpp',
    'engine/stringrenderer.cpp',
    'engine/charmap.cpp',

    'glyphcomponents/glyphbitmap.cpp',
    'glyphcomponents/glyphcontinuous.cpp',
    'glyphcomponents/glyphoutline.cpp',
    'glyphcomponents/glyphpointnumbers.cpp',
    'glyphcomponents/glyphpoints.cpp',
    'glyphcomponents/grid.cpp',
    'glyphcomponents/graphicsdefault.cpp',

    'widgets/customwidgets.cpp',
    'widgets/tripletselector.cpp',
    'widgets/glyphindexselector.cpp',
    'widgets/fontsizeselector.cpp',
    'widgets/charmapcombobox.cpp',

    'models/customcomboboxmodels.cpp',

    'panels/settingpanel.cpp',
    'panels/settingpanelmmgx.cpp',
    'panels/singular.cpp',
    'panels/continuous.cpp',
    'panels/glyphdetails.cpp',

    'ftinspect.cpp',
    'maingui.cpp',
    'uihelper.cpp',
])

  moc_files = qt5.preprocess(
    moc_headers: [
      'engine/fontfilemanager.hpp',
      'glyphcomponents/glyphbitmap.hpp',
      'glyphcomponents/glyphcontinuous.hpp',
      'widgets/customwidgets.hpp',
      'widgets/tripletselector.hpp',
      'widgets/glyphindexselector.hpp',
      'widgets/fontsizeselector.hpp',
      'widgets/charmapcombobox.hpp',
      'maingui.hpp',
      'models/customcomboboxmodels.hpp',
      'panels/settingpanel.hpp',
      'panels/settingpanelmmgx.hpp',
      'panels/singular.hpp',
      'panels/continuous.hpp',
      'panels/glyphdetails.hpp',
    ],
    dependencies: qt5_dep)

  executable('ftinspect',
    sources,
    moc_files,
    dependencies: [qt5_dep, libfreetype2_dep],
    override_options : ['cpp_std=c++11'],
    install: true)
endif

# EOF
