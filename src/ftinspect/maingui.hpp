// maingui.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include "engine/engine.hpp"
#include "widgets/tripletselector.hpp"
#include "panels/settingpanel.hpp"
#include "panels/abstracttab.hpp"
#include "panels/singular.hpp"
#include "panels/continuous.hpp"

#include <vector>
#include <QAction>
#include <QCloseEvent>
#include <QGridLayout>
#include <QDockWidget>
#include <QBoxLayout>
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

private:
  Engine* engine_;
  
  int currentNumberOfGlyphs_;

  // layout related stuff
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
  QWidget* lastTab_ = NULL;

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
