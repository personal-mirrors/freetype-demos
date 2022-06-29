// fontfilemanager.cpp

// Copyright (C) 2022 by Charlie Jiang.


#include "fontfilemanager.hpp"

FontFileManager::FontFileManager()
{
  fontWatcher = new QFileSystemWatcher(this);
  // if the current input file is invalid we retry once a second to load it
  watchTimer = new QTimer;
  watchTimer->setInterval(1000);

  connect(fontWatcher, &QFileSystemWatcher::fileChanged,
          this, &FontFileManager::onTimerOrWatcherFire);
  connect(watchTimer, &QTimer::timeout,
          this, &FontFileManager::onTimerOrWatcherFire);
}


FontFileManager::~FontFileManager()
{
}


int
FontFileManager::size()
{
  return fontFileNameList.size();
}


void
FontFileManager::append(QStringList newFileNames)
{
  for (auto& name : newFileNames)
  {
    auto info = QFileInfo(name);

    // Filter non-file elements
    if (!info.isFile())
      continue;

    // Uniquify elements
    auto absPath = info.absoluteFilePath();
    auto existing = false;
    for (auto& existingName : fontFileNameList)
      if (existingName.absoluteFilePath() == absPath)
      {
        existing = true;
        break;
      }
    if (existing)
      continue;

    fontFileNameList.append(info);
  }
}


void
FontFileManager::remove(int index)
{
  if (index < 0 || index >= size())
    return;

  fontWatcher->removePath(fontFileNameList[index].filePath());
  fontFileNameList.removeAt(index);
}


QFileInfo&
FontFileManager::operator[](int index)
{
  return fontFileNameList[index];
}


void
FontFileManager::updateWatching(int index)
{
  QFileInfo& fileInfo = fontFileNameList[index];

  auto watching = fontWatcher->files();
  if (!watching.empty())
    fontWatcher->removePaths(watching);

  // Qt's file watcher doesn't handle symlinks;
  // we thus fall back to polling
  if (fileInfo.isSymLink() || !fileInfo.exists())
    watchTimer->start();
  else
    fontWatcher->addPath(fileInfo.filePath());
}


void
FontFileManager::timerStart()
{
  watchTimer->start();
}


void
FontFileManager::onTimerOrWatcherFire()
{
  watchTimer->stop();
  emit currentFileChanged();
}


// end of fontfilemanager.hpp
