// maingui.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "rendering/glyphbitmap.hpp"
#include "rendering/glyphoutline.hpp"
#include "rendering/glyphpointnumbers.hpp"
#include "rendering/glyphpoints.hpp"
#include "rendering/grid.hpp"
#include "widgets/custom_widgets.hpp"
#include "models/ttsettingscomboboxmodel.hpp"
#include "panels/settingpanel.hpp"

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileSystemWatcher>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QPen>
#include <QPushButton>
#include <QScrollBar>
#include <QSignalMapper>
#include <QSlider>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>

#include <ft2build.h>
#include <freetype/ftlcdfil.h>


class MainGUI
: public QMainWindow
{
  Q_OBJECT

public:
  MainGUI(Engine* engine);
  ~MainGUI();

  void setDefaults();

  friend class Engine;
  friend FT_Error faceRequester(FTC_FaceID,
                                FT_Library,
                                FT_Pointer,
                                FT_Face*);

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void about();
  void aboutQt();
  void adjustGlyphIndex(int);
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentNamedInstanceIndex();
  void checkUnits();
  void closeFont();
  void showFont();
  void drawGlyph();
  void loadFonts();
  void nextFace();
  void nextFont();
  void nextNamedInstance();
  void previousFace();
  void previousFont();
  void previousNamedInstance();
  void watchCurrentFont();
  void zoom();
  void backToCenter();
  void updateGrid();
  void wheelZoom(QWheelEvent* event);
  void wheelResize(QWheelEvent* event);

private:
  Engine* engine_;
  
  int currentFontIndex_;

  long currentNumberOfFaces_;
  long currentFaceIndex_;

  int currentNumberOfNamedInstances_;
  int currentNamedInstanceIndex_;

  int currentNumberOfGlyphs_;
  int currentGlyphIndex_;

  // layout related stuff
  GlyphOutline *currentGlyphOutlineItem_;
  GlyphPoints *currentGlyphPointsItem_;
  GlyphPointNumbers *currentGlyphPointNumbersItem_;
  GlyphBitmap *currentGlyphBitmapItem_;
  Grid *gridItem_ = NULL;
  QLabel* mouseUsageHint_;

  QAction *aboutAct_;
  QAction *aboutQtAct_;
  QAction *closeFontAct_;
  QAction *exitAct_;
  QAction *loadFontsAct_;

  QComboBox *unitsComboBox_;

  QDoubleSpinBox *sizeDoubleSpinBox_;

  QGraphicsScene *glyphScene_;
  QGraphicsViewx *glyphView_;

  QGridLayout *fontLayout;
  QGridLayout *infoRightLayout;

  QHBoxLayout *ftinspectLayout_;
  QHBoxLayout *infoLeftLayout_;
  QHBoxLayout *navigationLayout_;
  QHBoxLayout *sizeLayout_;

  QLabel *dpiLabel_;
  QLabel *fontFilenameLabel_;
  QLabel *fontNameLabel_;
  QLabel *glyphIndexLabel_;
  QLabel *glyphNameLabel_;
  QLabel *sizeLabel_;
  QLabel *zoomLabel_;

  QLocale *locale_;

  QMenu *menuFile_;
  QMenu *menuHelp_;

  QPen axisPen_;
  QPen blueZonePen_;
  QPen gridPen_;
  QPen offPen_;
  QPen onPen_;
  QPen outlinePen_;
  QPen segmentPen_;

  QPushButton *centerGridButton_;
  QPushButton *nextFaceButton_;
  QPushButton *nextFontButton_;
  QPushButton *nextNamedInstanceButton_;
  QPushButton *previousFaceButton_;
  QPushButton *previousFontButton_;
  QPushButton *previousNamedInstanceButton_;

  QPushButtonx *toEndButtonx_;
  QPushButtonx *toM1000Buttonx_;
  QPushButtonx *toM100Buttonx_;
  QPushButtonx *toM10Buttonx_;
  QPushButtonx *toM1Buttonx_;
  QPushButtonx *toP1000Buttonx_;
  QPushButtonx *toP100Buttonx_;
  QPushButtonx *toP10Buttonx_;
  QPushButtonx *toP1Buttonx_;
  QPushButtonx *toStartButtonx_;

  QSignalMapper *glyphNavigationMapper_;

  QSpinBox *dpiSpinBox_;
  QSpinBoxx *zoomSpinBox_;
  
  QVBoxLayout *leftLayout_;
  QVBoxLayout *rightLayout_;

  QVector<QRgb> grayColorTable_;
  QVector<QRgb> monoColorTable_;

  QWidget *ftinspectWidget_;
  QWidget *leftWidget_;
  QWidget *rightWidget_;

  SettingPanel* settingPanel_;

  enum Units
  {
    Units_px,
    Units_pt
  };

  void syncSettings();
  void clearStatusBar();

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void createStatusBar();
  void setGraphicsDefaults();

  void readSettings();
  void writeSettings();
};


// end of maingui.hpp
