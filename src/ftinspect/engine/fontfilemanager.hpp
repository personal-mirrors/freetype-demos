// fontfilemanager.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QObject>
#include <QList>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QFileInfo>

#include <freetype/freetype.h>


// Class to manage all opened font files, as well as monitoring local file
// change.

class Engine;
class FontFileManager
: public QObject
{
  Q_OBJECT
public:
  FontFileManager(Engine* engine);
  ~FontFileManager() override = default;

  int size();
  void append(QStringList const& newFileNames, bool alertNotExist = false);
  void remove(int index);

  QFileInfo& operator[](int index);
  void updateWatching(int index);
  void timerStart();
  
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

  FT_Error validateFontFile(QString const& fileName);
};


// end of fontfilemanager.hpp
