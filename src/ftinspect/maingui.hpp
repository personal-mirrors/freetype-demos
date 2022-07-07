// maingui.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "widgets/customwidgets.hpp"
#include "widgets/glyphindexselector.hpp"
#include "models/ttsettingscomboboxmodel.hpp"
#include "panels/settingpanel.hpp"
#include "panels/singular.hpp"

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
  void dragEnterEvent(QDragEnterEvent* event);
  void dropEvent(QDropEvent* event);

private slots:
  void about();
  void aboutQt();
  void checkCurrentFaceIndex();
  void checkCurrentFontIndex();
  void checkCurrentNamedInstanceIndex();
  void closeFont();
  void showFont();
  void repaintCurrentTab();
  void reloadCurrentTabFont();
  void loadFonts();
  void nextFace();
  void nextFont();
  void nextNamedInstance();
  void previousFace();
  void previousFont();
  void previousNamedInstance();
  void watchCurrentFont();

private:
  Engine* engine_;
  
  int currentFontIndex_;

  long currentNumberOfFaces_;
  long currentFaceIndex_;

  int currentNumberOfNamedInstances_;
  int currentNamedInstanceIndex_;

  int currentNumberOfGlyphs_;

  // layout related stuff
  QAction *aboutAct_;
  QAction *aboutQtAct_;
  QAction *closeFontAct_;
  QAction *exitAct_;
  QAction *loadFontsAct_;

  QGridLayout *fontLayout;

  QHBoxLayout *ftinspectLayout_;
  QHBoxLayout *infoLeftLayout_;

  QLabel *fontFilenameLabel_;
  QLabel *fontNameLabel_;

  QLocale *locale_;

  QMenu *menuFile_;
  QMenu *menuHelp_;

  QPushButton *nextFaceButton_;
  QPushButton *nextFontButton_;
  QPushButton *nextNamedInstanceButton_;
  QPushButton *previousFaceButton_;
  QPushButton *previousFontButton_;
  QPushButton *previousNamedInstanceButton_;
  
  QVBoxLayout *leftLayout_;
  QVBoxLayout *rightLayout_;

  QWidget *ftinspectWidget_;
  QWidget *leftWidget_;
  QWidget *rightWidget_;

  SettingPanel* settingPanel_;

  QTabWidget* tabWidget_;
  QVector<AbstractTab*> tabs_;
  SingularTab* singularTab_;

  void openFonts(QStringList const& fileNames);

  void syncSettings();
  void clearStatusBar();

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void createStatusBar();
  void setupDragDrop();

  void readSettings();
  void writeSettings();
};


// end of maingui.hpp
