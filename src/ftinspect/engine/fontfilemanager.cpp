// fontfilemanager.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.


#include "engine.hpp"
#include "fontfilemanager.hpp"

#include <QCoreApplication>
#include <QGridLayout>
#include <QMessageBox>


FontFileManager::FontFileManager(Engine* engine)
: engine_(engine)
{
  fontWatcher_ = new QFileSystemWatcher(this);
  // if the current input file is invalid we retry once a second to load it.
  watchTimer_ = new QTimer;
  watchTimer_->setInterval(1000);

  connect(fontWatcher_, &QFileSystemWatcher::fileChanged,
          this, &FontFileManager::onWatcherFire);
  connect(watchTimer_, &QTimer::timeout,
          this, &FontFileManager::onTimerFire);
}


int
FontFileManager::size()
{
  return fontFileNameList_.size();
}


void
FontFileManager::append(QStringList const& newFileNames,
                        bool alertNotExist)
{
  QStringList failedFiles;
  for (auto& name : newFileNames)
  {
    auto info = QFileInfo(name);
    info.setCaching(false);

    // Filter out non-file elements.
    if (!info.isFile())
    {
      if (alertNotExist)
        failedFiles.append(name);
      continue;
    }

    auto err = validateFontFile(name);
    if (err)
    {
      if (alertNotExist)
      {
        auto errString = FT_Error_String(err);
        if (!errString)
          failedFiles.append(QString("- %1: %2")
                             .arg(name)
                             .arg(err));
        else
          failedFiles.append(QString("- %1: %2 (%3)")
                             .arg(name)
                             .arg(errString)
                             .arg(err));
      }
      continue;
    }

    // Uniquify elements.
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

    if (info.size() >= INT_MAX)
      return; // Prevent overflow.
    fontFileNameList_.append(info);
  }

  if (alertNotExist && !failedFiles.empty())
  {
    auto msg = new QMessageBox;
    msg->setAttribute(Qt::WA_DeleteOnClose);
    msg->setStandardButtons(QMessageBox::Ok);
    if (failedFiles.size() == 1)
    {
      msg->setWindowTitle(tr("Failed to load file"));
      msg->setText(tr("File failed to load:\n%1")
                   .arg(failedFiles.join("\n")));
    }
    else
    {
      msg->setWindowTitle(tr("Failed to load some files"));
      msg->setText(tr("Files failed to load:\n%1")
                   .arg(failedFiles.join("\n")));
    }

    msg->setIcon(QMessageBox::Warning);
    msg->setModal(false);
    msg->open();
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
  // we thus fall back to polling.
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
FontFileManager::loadFromCommandLine()
{
  // TODO: To support more complicated command line, we need to move this
  //       away and use `QCommandLineParser`
  auto args = QCoreApplication::arguments();
  if (!args.empty())
    args.removeFirst();
  append(args, true);
}


void
FontFileManager::onWatcherFire()
{
  watchTimer_->stop();
  emit currentFileChanged();
}


FT_Error
FontFileManager::validateFontFile(QString const& fileName)
{
  return FT_New_Face(engine_->ftLibrary(), fileName.toUtf8(), -1, NULL);
}


void
FontFileManager::onTimerFire()
{
  periodicUpdating_ = true;
  onWatcherFire();
  periodicUpdating_ = false;
}


// end of fontfilemanager.hpp
