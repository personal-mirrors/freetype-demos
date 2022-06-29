// maingui.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "rendering/glyphbitmap.hpp"
#include "rendering/glyphoutline.hpp"
#include "rendering/glyphpointnumbers.hpp"
#include "rendering/glyphpoints.hpp"
#include "widgets/qcomboboxx.hpp"
#include "widgets/qgraphicsviewx.hpp"
#include "widgets/qpushbuttonx.hpp"
#include "widgets/qspinboxx.hpp"

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
  MainGUI();
  ~MainGUI();

  void setDefaults();
  void update(Engine*);

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
  void checkAntiAliasing();
  void checkAutoHinting();
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentNamedInstanceIndex();
  void checkHinting();
  void checkHintingMode();
  void checkLcdFilter();
  void checkShowPoints();
  void checkUnits();
  void closeFont();
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

private:
  Engine* engine_;
  
  int currentFontIndex_;

  long currentNumberOfFaces_;
  long currentFaceIndex_;

  int currentNumberOfNamedInstances_;
  int currentNamedInstanceIndex_;

  int currentNumberOfGlyphs_;
  int currentGlyphIndex_;

  int currentCFFHintingMode_;
  int currentTTInterpreterVersion_;

  // layout related stuff
  GlyphOutline *currentGlyphOutlineItem_;
  GlyphPoints *currentGlyphPointsItem_;
  GlyphPointNumbers *currentGlyphPointNumbersItem_;
  GlyphBitmap *currentGlyphBitmapItem_;

  QAction *aboutAct_;
  QAction *aboutQtAct_;
  QAction *closeFontAct_;
  QAction *exitAct_;
  QAction *loadFontsAct_;

  QCheckBox *autoHintingCheckBox_;
  QCheckBox *blueZoneHintingCheckBox_;
  QCheckBox *hintingCheckBox_;
  QCheckBox *horizontalHintingCheckBox_;
  QCheckBox *segmentDrawingCheckBox_;
  QCheckBox *showBitmapCheckBox_;
  QCheckBox *showOutlinesCheckBox_;
  QCheckBox *showPointNumbersCheckBox_;
  QCheckBox *showPointsCheckBox_;
  QCheckBox *verticalHintingCheckBox_;

  QComboBoxx *antiAliasingComboBoxx_;
  QComboBoxx *hintingModeComboBoxx_;
  QComboBox *lcdFilterComboBox_;
  QComboBox *unitsComboBox_;

  QDoubleSpinBox *sizeDoubleSpinBox_;

  QGraphicsScene *glyphScene_;
  QGraphicsViewx *glyphView_;

  QGridLayout *fontLayout;
  QGridLayout *infoRightLayout;

  QHash<int, int> hintingModesTrueTypeHash_;
  QHash<int, int> hintingModesCFFHash_;
  QHash<FT_LcdFilter, int> lcdFilterHash_;

  QHBoxLayout *antiAliasingLayout_;
  QHBoxLayout *blueZoneHintingLayout_;
  QHBoxLayout *ftinspectLayout_;
  QHBoxLayout *gammaLayout_;
  QHBoxLayout *hintingModeLayout_;
  QHBoxLayout *horizontalHintingLayout_;
  QHBoxLayout *infoLeftLayout_;
  QHBoxLayout *lcdFilterLayout_;
  QHBoxLayout *navigationLayout_;
  QHBoxLayout *pointNumbersLayout_;
  QHBoxLayout *segmentDrawingLayout_;
  QHBoxLayout *sizeLayout_;
  QHBoxLayout *verticalHintingLayout_;

  QLabel *antiAliasingLabel_;
  QLabel *dpiLabel_;
  QLabel *fontFilenameLabel_;
  QLabel *fontNameLabel_;
  QLabel *gammaLabel_;
  QLabel *glyphIndexLabel_;
  QLabel *glyphNameLabel_;
  QLabel *hintingModeLabel_;
  QLabel *lcdFilterLabel_;
  QLabel *sizeLabel_;
  QLabel *zoomLabel_;

  QList<int> hintingModesAlwaysDisabled_;

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

  QSlider *gammaSlider_;

  QSpinBox *dpiSpinBox_;
  QSpinBoxx *zoomSpinBox_;

  QTabWidget *tabWidget_;

  QVBoxLayout *generalTabLayout_;
  QVBoxLayout *leftLayout_;
  QVBoxLayout *rightLayout_;

  QVector<QRgb> grayColorTable_;
  QVector<QRgb> monoColorTable_;

  QWidget *ftinspectWidget_;
  QWidget *generalTabWidget_;
  QWidget *leftWidget_;
  QWidget *rightWidget_;
  QWidget *mmgxTabWidget_;
  
  enum HintingMode
  {
    HintingMode_TrueType_v35,
    HintingMode_TrueType_v38,
    HintingMode_TrueType_v40,
    HintingMode_CFF_FreeType,
    HintingMode_CFF_Adobe
  };
  enum LCDFilter
  {
    LCDFilter_Default,
    LCDFilter_Light,
    LCDFilter_None,
    LCDFilter_Legacy
  };
  enum Units
  {
    Units_px,
    Units_pt
  };

  void showFont();
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
