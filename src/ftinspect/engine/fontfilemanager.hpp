// fontfilemanager.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QObject>
#include <QList>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QFileInfo>


// Class to manage all opened font files, as well as monitoring local file
// change.

class FontFileManager
: public QObject
{
  Q_OBJECT
public:
  FontFileManager();
  ~FontFileManager() override = default;

  int size();
  void append(QStringList newFileNames, bool alertNotExist = false);
  void remove(int index);

  QFileInfo& operator[](int index);
  void updateWatching(int index);
  void timerStart();
  void loadFromCommandLine();

  bool currentReloadDueToPeriodicUpdate() { return periodicUpdating_; }

signals:
  void currentFileChanged();

private slots:
  void onTimerFire();
  void onWatcherFire();

private:
  QList<QFileInfo> fontFileNameList_;
  QFileSystemWatcher* fontWatcher_;
  QTimer* watchTimer_;

  bool periodicUpdating_ = false;
};


// end of fontfilemanager.hpp
