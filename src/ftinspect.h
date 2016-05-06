// ftinspect.h

#ifndef FTINSPECT_H_
#define FTINSPECT_H_

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CFF_DRIVER_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H
#include FT_TRUETYPE_DRIVER_H

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGraphicsView>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QTabWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>


class MainGUI;


struct Font
{
  QString filePathname;
  // the number of instances per face;
  // the size of the list gives the number of faces
  QList<int> numInstancesList;
};


struct FaceID
{
  int fontIndex;
  int faceIndex;
  int instanceIndex;
};


class Engine
{
public:
  Engine(MainGUI*);
  ~Engine();

  void update();
  int numFaces(int);
  int numInstances(int, int);

  friend class MainGUI;

private:
  MainGUI* gui;

  FT_Library library;
  FTC_Manager cacheManager;
  FTC_ImageCache imageCache;
  FTC_SBitCache sbitsCache;

  int cffHintingEngineDefault;
  int cffHintingEngineOther;

  int ttInterpreterVersionDefault;
  int ttInterpreterVersionOther;
  int ttInterpreterVersionOther1;

  int haveWarping;

  double pointSize;
  double pixelSize;
  int dpi;
  int zoom;

  bool doHorizontalHinting;
  bool doVerticalHinting;
  bool doBlueZoneHinting;
  bool showSegments;
  bool doWarping;

  bool showBitmap;
  bool showPoints;
  bool showPointIndices;
  bool showOutlines;

  double gamma;
};


// we want to grey out items in a combo box;
// since Qt doesn't provide a function for this we derive a class
class QComboBoxx
: public QComboBox
{
  Q_OBJECT

public:
  void setItemEnabled(int, bool);
};


// we want buttons that are horizontally as small as possible
class QPushButtonx
: public QPushButton
{
  Q_OBJECT

public:
  QPushButtonx(const QString&, QWidget* = 0);
  virtual ~QPushButtonx(){}
};


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
  void checkAntiAliasing();
  void checkAutoHinting();
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentInstanceIndex();
  void checkHinting();
  void checkHintingMode();
  void checkLcdFilter();
  void checkShowPoints();
  void checkUnits();
  void closeFont();
  void loadFonts();
  void nextFace();
  void nextFont();
  void nextInstance();
  void previousFace();
  void previousFont();
  void previousInstance();

private:
  Engine* engine;

  QList<Font> fonts;
  int currentFontIndex;
  int currentFaceIndex;
  int currentInstanceIndex;

  // layout related stuff
  QAction *aboutAct;
  QAction *aboutQtAct;
  QAction *closeFontAct;
  QAction *exitAct;
  QAction *loadFontsAct;

  QCheckBox *autoHintingCheckBox;
  QCheckBox *blueZoneHintingCheckBox;
  QCheckBox *hintingCheckBox;
  QCheckBox *horizontalHintingCheckBox;
  QCheckBox *segmentDrawingCheckBox;
  QCheckBox *showBitmapCheckBox;
  QCheckBox *showOutlinesCheckBox;
  QCheckBox *showPointIndicesCheckBox;
  QCheckBox *showPointsCheckBox;
  QCheckBox *verticalHintingCheckBox;
  QCheckBox *warpingCheckBox;

  QComboBoxx *antiAliasingComboBoxx;
  QComboBoxx *hintingModeComboBoxx;
  QComboBox *lcdFilterComboBox;
  QComboBox *unitsComboBox;

  QDoubleSpinBox *sizeDoubleSpinBox;

  QGraphicsView *glyphView;

  QGridLayout *fontLayout;

  QHash<int, int> hintingModesTrueTypeHash;
  QHash<int, int> hintingModesCFFHash;
  QHash<FT_LcdFilter, int> lcdFilterHash;

  QHBoxLayout *antiAliasingLayout;
  QHBoxLayout *blueZoneHintingLayout;
  QHBoxLayout *ftinspectLayout;
  QHBoxLayout *gammaLayout;
  QHBoxLayout *hintingModeLayout;
  QHBoxLayout *horizontalHintingLayout;
  QHBoxLayout *lcdFilterLayout;
  QHBoxLayout *navigationLayout;
  QHBoxLayout *pointIndicesLayout;
  QHBoxLayout *segmentDrawingLayout;
  QHBoxLayout *sizeLayout;
  QHBoxLayout *verticalHintingLayout;
  QHBoxLayout *warpingLayout;
  QHBoxLayout *watchLayout;

  QLabel *antiAliasingLabel;
  QLabel *dpiLabel;
  QLabel *gammaLabel;
  QLabel *hintingModeLabel;
  QLabel *lcdFilterLabel;
  QLabel *sizeLabel;
  QLabel *zoomLabel;

  QList<int> hintingModesAlwaysDisabled;

  QLocale *locale;

  QMenu *menuFile;
  QMenu *menuHelp;

  QPushButton *nextFaceButton;
  QPushButton *nextFontButton;
  QPushButton *nextInstanceButton;
  QPushButton *previousFaceButton;
  QPushButton *previousFontButton;
  QPushButton *previousInstanceButton;
  QPushButton *watchButton;

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

  QSlider *gammaSlider;

  QSpinBox *dpiSpinBox;
  QSpinBox *zoomSpinBox;

  QTabWidget *tabWidget;

  QVBoxLayout *generalTabLayout;
  QVBoxLayout *leftLayout;
  QVBoxLayout *rightLayout;

  QWidget *ftinspectWidget;
  QWidget *generalTabWidget;
  QWidget *leftWidget;
  QWidget *rightWidget;
  QWidget *mmgxTabWidget;

  enum AntiAliasing
  {
    AntiAliasing_None,
    AntiAliasing_Normal,
    AntiAliasing_Slight,
    AntiAliasing_LCD,
    AntiAliasing_LCD_BGR,
    AntiAliasing_LCD_Vertical,
    AntiAliasing_LCD_Vertical_BGR
  };
  enum HintingModes
  {
    HintingMode_TrueType_v35,
    HintingMode_TrueType_v38,
    HintingMode_TrueType_v40,
    HintingMode_CFF_FreeType,
    HintingMode_CFF_Adobe,
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

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void clearStatusBar();
  void createStatusBar();
  void readSettings();
  void showFont();
  void writeSettings();
};


#endif // FTINSPECT_H_


// end of ftinspect.h
