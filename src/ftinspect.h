// ftinspect.h

#ifndef FTINSPECT_H_
#define FTINSPECT_H_

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDoubleSpinBox>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
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


class MainGUI
: public QMainWindow
{
  Q_OBJECT

public:
  MainGUI();
  ~MainGUI();

protected:
  void closeEvent(QCloseEvent*);

private slots:
  void about();
  void checkAntiAliasing();
  void checkHintingMode();
  void checkShowPoints();

private:
  QAction *aboutAct;
  QAction *aboutQtAct;
  QAction *exitAct;

  QCheckBox *blueZoneHintingCheckBox;
  QCheckBox *horizontalHintingCheckBox;
  QCheckBox *segmentDrawingCheckBox;
  QCheckBox *showBitmapsCheckBox;
  QCheckBox *showOutlinesCheckBox;
  QCheckBox *showPointIndicesCheckBox;
  QCheckBox *showPointsCheckBox;
  QCheckBox *verticalHintingCheckBox;
  QCheckBox *warpingCheckBox;

  QComboBox *antiAliasingComboBox;
  QComboBox *hintingModeComboBox;
  QComboBox *lcdFilterComboBox;

  QDoubleSpinBox *sizeDoubleSpinBox;

  QGraphicsView *glyphView;

  QHBoxLayout *antiAliasingLayout;
  QHBoxLayout *fontLayout;
  QHBoxLayout *gammaLayout;
  QHBoxLayout *hintingModeLayout;
  QHBoxLayout *ftinspectLayout;
  QHBoxLayout *lcdFilterLayout;
  QHBoxLayout *navigationLayout;
  QHBoxLayout *watchLayout;

  QLabel *antiAliasingLabel;
  QLabel *gammaLabel;
  QLabel *hintingModeLabel;
  QLabel *lcdFilterLabel;
  QLabel *sizeLabel;
  QLabel *zoomLabel;

  QLocale *locale;

  QMenu *menuFile;
  QMenu *menuHelp;

  QPushButton *nextFontButton;
  QPushButton *previousFontButton;
  QPushButton *toEndButton;
  QPushButton *toM1000Button;
  QPushButton *toM100Button;
  QPushButton *toM10Button;
  QPushButton *toM1Button;
  QPushButton *toP1000Button;
  QPushButton *toP100Button;
  QPushButton *toP10Button;
  QPushButton *toP1Button;
  QPushButton *toStartButton;
  QPushButton *watchButton;

  QSlider *gammaSlider;

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
    HintingMode_AutoHinting
  };
  enum LCDFilter
  {
    LCDFilter_Default,
    LCDFilter_Light,
    LCDFilter_None,
    LCDFilter_Legacy
  };

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void clearStatusBar();
  void createStatusBar();
  void readSettings();
  void setDefaults();
  void writeSettings();
};


#endif // FTINSPECT_H_


// end of ftinspect.h
