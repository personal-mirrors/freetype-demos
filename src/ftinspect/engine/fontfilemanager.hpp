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
  ~FontFileManager() override;

  int size();
  void append(QStringList newFileNames);
  void remove(int index);

  QFileInfo& operator[](int index);
  void updateWatching(int index);
  void timerStart();

signals:
  void currentFileChanged();

private slots:
  void onTimerOrWatcherFire();

private:
  QList<QFileInfo> fontFileNameList_;
  QFileSystemWatcher* fontWatcher_;
  QTimer* watchTimer_;
};


// end of fontfilemanager.hpp
