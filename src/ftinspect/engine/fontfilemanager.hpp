// fontfilemanager.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QList>
#include <QObject>
#include <QTimer>

#include <freetype/freetype.h>


// Class to manage all opened font files, as well as monitoring local file
// changes.

class Engine;

class FontFileManager
: public QObject
{
  Q_OBJECT
public:
  FontFileManager(Engine* engine);
  ~FontFileManager() override = default;

  int size();
  void append(QStringList const& newFileNames,
              bool alertNotExist = false);
  void remove(int index);

  QFileInfo& operator[](int index);
  void updateWatching(int index);
  void timerStart();
  void loadFromCommandLine();

  // If this is true, the current font reloading is due to a periodic
  // reloading for symbolic font files.  Use this if you want to omit some
  // updating for periodic reloading.
  bool currentReloadDueToPeriodicUpdate() { return periodicUpdating_; }

signals:
  void currentFileChanged();

private slots:
  void onTimerFire();
  void onWatcherFire();

private:
  Engine* engine_;
  QList<QFileInfo> fontFileNameList_;
  QFileSystemWatcher* fontWatcher_;
  QTimer* watchTimer_;

  bool periodicUpdating_ = false;

  FT_Error validateFontFile(QString const& fileName);
};


// end of fontfilemanager.hpp
