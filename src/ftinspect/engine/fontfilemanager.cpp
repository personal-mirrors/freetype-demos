// fontfilemanager.cpp

// Copyright (C) 2022 by Charlie Jiang.


#include "fontfilemanager.hpp"

FontFileManager::FontFileManager()
{
  fontWatcher_ = new QFileSystemWatcher(this);
  // if the current input file is invalid we retry once a second to load it
  watchTimer_ = new QTimer;
  watchTimer_->setInterval(1000);

  connect(fontWatcher_, &QFileSystemWatcher::fileChanged,
          this, &FontFileManager::onTimerOrWatcherFire);
  connect(watchTimer_, &QTimer::timeout,
          this, &FontFileManager::onTimerOrWatcherFire);
}


FontFileManager::~FontFileManager()
{
}


int
FontFileManager::size()
{
  return fontFileNameList_.size();
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
    for (auto& existingName : fontFileNameList_)
      if (existingName.absoluteFilePath() == absPath)
      {
        existing = true;
        break;
      }
    if (existing)
      continue;

    fontFileNameList_.append(info);
  }
}


void
FontFileManager::remove(int index)
{
  if (index < 0 || index >= size())
    return;

  fontWatcher_->removePath(fontFileNameList_[index].filePath());
  fontFileNameList_.removeAt(index);
}


QFileInfo&
FontFileManager::operator[](int index)
{
  return fontFileNameList_[index];
}


void
FontFileManager::updateWatching(int index)
{
  QFileInfo& fileInfo = fontFileNameList_[index];

  auto watching = fontWatcher_->files();
  if (!watching.empty())
    fontWatcher_->removePaths(watching);

  // Qt's file watcher doesn't handle symlinks;
  // we thus fall back to polling
  if (fileInfo.isSymLink() || !fileInfo.exists())
    watchTimer_->start();
  else
    fontWatcher_->addPath(fileInfo.filePath());
}


void
FontFileManager::timerStart()
{
  watchTimer_->start();
}


void
FontFileManager::onTimerOrWatcherFire()
{
  watchTimer_->stop();
  emit currentFileChanged();
}


// end of fontfilemanager.hpp
