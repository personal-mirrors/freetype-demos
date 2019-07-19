// maingui.hpp

// Copyright (C) 2016-2019 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "rendering/glyphbitmap.hpp"
#include "rendering/glyphoutline.hpp"
#include "rendering/glyphpointnumbers.hpp"
#include "rendering/glyphsegment.hpp"
#include "rendering/glyphpoints.hpp"
#include "rendering/view.hpp"
#include "rendering/grid.hpp"
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
#include <QRadioButton>

#include <ft2build.h>
#include FT_LCD_FILTER_H
#include FT_COLOR_H


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
  void showCharmapsInfo();
  void showFontType();
  void showFontName();
  void adjustGlyphIndex(int);
  void checkAntiAliasing();
  void checkAutoHinting();
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentNamedInstanceIndex();
  void checkHinting();
  void checkHintingMode();
  void checkRenderingMode();
  void checkKerningMode();
  void checkKerningDegree();
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
  void renderAll();
  void gridViewRender();

private:
  Engine* engine;

  
  int render_mode = 1;
  int kerning_mode = 0;
  int kerning_degree = 0;

  QStringList fontList;
  int currentFontIndex;

  long currentNumberOfFaces;
  long currentFaceIndex;

  int currentNumberOfNamedInstances;
  int currentNamedInstanceIndex;

  int currentNumberOfGlyphs;
  int currentGlyphIndex;

  int currentCFFHintingMode;
  int currentTTInterpreterVersion;



  // layout related stuff
  GlyphOutline *currentGlyphOutlineItem;
  GlyphPoints *currentGlyphPointsItem;
  GlyphSegment *currentGlyphSegmentItem;
  GlyphPointNumbers *currentGlyphPointNumbersItem;
  GlyphBitmap *currentGlyphBitmapItem;
  RenderAll *currentRenderAllItem;
  Grid *currentGridItem;

  QAction *aboutAct;
  QAction *aboutQtAct;
  QAction *closeFontAct;
  QAction *exitAct;
  QAction *loadFontsAct;
  QAction *showCharmapsInfoAct;
  QAction *showFontTypeAct;
  QAction *showFontNameAct;

  QCheckBox *autoHintingCheckBox;
  QCheckBox *blueZoneHintingCheckBox;
  QCheckBox *hintingCheckBox;
  QCheckBox *horizontalHintingCheckBox;
  QCheckBox *segmentDrawingCheckBox;
  QCheckBox *showBitmapCheckBox;
  QCheckBox *showOutlinesCheckBox;
  QCheckBox *showPointNumbersCheckBox;
  QCheckBox *showPointsCheckBox;
  QCheckBox *verticalHintingCheckBox;
  QCheckBox *warpingCheckBox;
  

  QComboBoxx *antiAliasingComboBoxx;
  QComboBoxx *hintingModeComboBoxx;
  QComboBoxx *renderingModeComboBoxx;
  QComboBoxx *kerningModeComboBoxx;
  QComboBoxx *kerningDegreeComboBoxx;
  QComboBox *lcdFilterComboBox;
  QComboBox *unitsComboBox;

  QDoubleSpinBox *sizeDoubleSpinBox;

  QFileSystemWatcher *fontWatcher;

  QGraphicsScene *glyphScene;
  QGraphicsViewx *glyphView;

  QGridLayout *fontLayout;
  QGridLayout *infoRightLayout;

  QHash<int, int> hintingModesTrueTypeHash;
  QHash<int, int> hintingModesCFFHash;
  QHash<FT_LcdFilter, int> lcdFilterHash;

  QHBoxLayout *antiAliasingLayout;
  QHBoxLayout *blueZoneHintingLayout;
  QHBoxLayout *ftinspectLayout;
  QHBoxLayout *gammaLayout;
  QHBoxLayout *hintingModeLayout;
  QHBoxLayout *horizontalHintingLayout;
  QHBoxLayout *infoLeftLayout;
  QHBoxLayout *lcdFilterLayout;
  QHBoxLayout *navigationLayout;
  QHBoxLayout *pointNumbersLayout;
  QHBoxLayout *segmentDrawingLayout;
  QHBoxLayout *sizeLayout;
  QHBoxLayout *verticalHintingLayout;
  QHBoxLayout *warpingLayout;
  QHBoxLayout *programNavigationLayout;
  QHBoxLayout *renderLayout;
  QHBoxLayout *kerningLayout;
  QHBoxLayout *degreeLayout;
  QHBoxLayout *emboldenVertLayout;
  QHBoxLayout *emboldenHorzLayout;
  QHBoxLayout *slantLayout;
  QHBoxLayout *strokeLayout;

  QLabel *antiAliasingLabel;
  QLabel *dpiLabel;
  QLabel *fontFilenameLabel;
  QLabel *fontNameLabel;
  QLabel *gammaLabel;
  QLabel *glyphIndexLabel;
  QLabel *glyphNameLabel;
  QLabel *hintingModeLabel;
  QLabel *renderingModeLabel;
  QLabel *kerningModeLabel;
  QLabel *kerningDegreeLabel;
  QLabel *lcdFilterLabel;
  QLabel *sizeLabel;
  QLabel *zoomLabel;
  QLabel *xLabel;
  QLabel *yLabel;
  QLabel *slantLabel;
  QLabel *strokeLabel;

  QRadioButton *gridView = new QRadioButton(tr("Grid View"));
  QRadioButton *allGlyphs = new QRadioButton(tr("All Glyphs"));
  QRadioButton *stringView = new QRadioButton(tr("Render String"));
  QRadioButton *multiView = new QRadioButton(tr("Multi View"));

  QList<int> hintingModesAlwaysDisabled;

  QLocale *locale;

  QMenu *menuFile;
  QMenu *menuHelp;
  QMenu *menuInfo;

  QPen axisPen;
  QPen blueZonePen;
  QPen gridPen;
  QPen offPen;
  QPen onPen;
  QPen outlinePen;
  QPen segmentPen;

  QPushButton *nextFaceButton;
  QPushButton *nextFontButton;
  QPushButton *nextNamedInstanceButton;
  QPushButton *previousFaceButton;
  QPushButton *previousFontButton;
  QPushButton *previousNamedInstanceButton;

  QPushButtonx *toEndButtonx;
  QPushButtonx *toM1000Buttonx;
  QPushButtonx *toM100Buttonx;
  QPushButtonx *toM10Buttonx;
  QPushButtonx *toM1Buttonx;
  QPushButtonx *toP1000Buttonx;
  QPushButtonx *toP100Buttonx;
  QPushButtonx *toP10Buttonx;
  QPushButtonx *toP1Buttonx;
  QPushButtonx *toStartButtonx;

  QSignalMapper *glyphNavigationMapper;

  QSlider *gammaSlider;
  QSlider *embolden_x_Slider;
  QSlider *embolden_y_Slider;
  QSlider *stroke_Slider;
  QSlider *slant_Slider;

  QSpinBox *dpiSpinBox;
  QSpinBoxx *zoomSpinBox;

  QTabWidget *tabWidget;

  QTimer *timer;

  QVBoxLayout *generalTabLayout;
  QVBoxLayout *leftLayout;
  QVBoxLayout *rightLayout;
  QVBoxLayout *viewTabLayout;

  QVector<QRgb> grayColorTable;
  QVector<QRgb> monoColorTable;

  QWidget *ftinspectWidget;
  QWidget *generalTabWidget;
  QWidget *leftWidget;
  QWidget *rightWidget;
  QWidget *mmgxTabWidget;
  QWidget *viewTabWidget;

  enum AntiAliasing
  {
    AntiAliasing_None,
    AntiAliasing_Normal,
    AntiAliasing_Light,
    AntiAliasing_LCD,
    AntiAliasing_LCD_BGR,
    AntiAliasing_LCD_Vertical,
    AntiAliasing_LCD_Vertical_BGR
  };
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
  enum RenderingMode
  {
    Normal,
    Fancy,
    Stroked,
    Text_String,
    Waterfall,
    Kerning_Comparison
  };
  enum KerningMode
  {
    KERNING_MODE_NONE,      /* 0: no kerning;                  */
    KERNING_MODE_NORMAL,        /* 1: `kern' values                */
    KERNING_MODE_SMART         /* 2: `kern' + side bearing errors */
  };
  enum KerningDegree
  {
    KERNING_DEGREE_NONE,
    KERNING_DEGREE_LIGHT,
    KERNING_DEGREE_MEDIUM,
    KERNING_DEGREE_TIGHT
  };

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void clearStatusBar();
  void createStatusBar();
  void readSettings();
  void setGraphicsDefaults();
  void showFont();
  void writeSettings();
};


// end of maingui.hpp
