// maingui.hpp

// Copyright (C) 2016-2023 by
// Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "panels/abstracttab.hpp"
#include "panels/comparator.hpp"
#include "panels/continuous.hpp"
#include "panels/glyphdetails.hpp"
#include "panels/info.hpp"
#include "panels/settingpanel.hpp"
#include "panels/singular.hpp"
#include "widgets/tripletselector.hpp"

#include <vector>

#include <QAction>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDockWidget>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QTabWidget>


class MainGUI
: public QMainWindow
{
  Q_OBJECT

public:
  MainGUI(Engine* engine);
  ~MainGUI() override;

  friend class Engine;
  friend FT_Error faceRequester(FTC_FaceID,
                                FT_Library,
                                FT_Pointer,
                                FT_Face*);

protected:
  void closeEvent(QCloseEvent*) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

private slots:
  void about();
  void aboutQt();
  void repaintCurrentTab();
  void reloadCurrentTabFont();
  void loadFonts();
  void onTripletChanged();
  void switchTab();
  void switchToSingular(int glyphIndex,
                        double sizePoint);
  void closeDockWidget();

private:
  Engine* engine_;

  int currentNumberOfGlyphs_;

  // Layout-related stuff.
  QAction *aboutAct_;
  QAction *aboutQtAct_;
  QAction *closeFontAct_;
  QAction *exitAct_;
  QAction *loadFontsAct_;

  QVBoxLayout *ftinspectLayout_;
  QHBoxLayout *mainPartLayout_;

  QLocale *locale_;

  QMenu *menuFile_;
  QMenu *menuHelp_;

  TripletSelector* tripletSelector_;

  QVBoxLayout *leftLayout_;
  QVBoxLayout *rightLayout_;

  QWidget *ftinspectWidget_;
  QWidget *leftWidget_;
  QWidget *rightWidget_;

  SettingPanel* settingPanel_;

  QTabWidget* tabWidget_;
  std::vector<AbstractTab*> tabs_;
  SingularTab* singularTab_;
  ContinuousTab* continuousTab_;
  ComparatorTab* comparatorTab_;
  InfoTab* infoTab_;
  QWidget* lastTab_ = NULL;

  QDockWidget* glyphDetailsDockWidget_;
  GlyphDetails* glyphDetails_;

  void openFonts(QStringList const& fileNames);

  void applySettings();

  void createActions();
  void createConnections();
  void createLayout();
  void createMenus();
  void setupDragDrop();

  void readSettings();
  void writeSettings();

  void loadCommandLine();
};


// end of maingui.hpp
