// tripletselector.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <vector>

#include <QBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>


class Engine;

class TripletSelector
: public QWidget
{
  Q_OBJECT

public:
  TripletSelector(QWidget* parent,
                  Engine* engine);
  ~TripletSelector() override;

  void repopulateFonts();
  void closeCurrentFont();

signals:
  void tripletChanged();

private:
  Engine* engine_;

  QComboBox* fontComboBox_;
  QComboBox* faceComboBox_;
  QComboBox* niComboBox_;

  QToolButton* closeFontButton_;

  QToolButton* fontUpButton_;
  QToolButton* fontDownButton_;
  QToolButton* faceUpButton_;
  QToolButton* faceDownButton_;
  QToolButton* niUpButton_;
  QToolButton* niDownButton_;

  QHBoxLayout* layout_;

  void checkButtons();
  void watchCurrentFont();

  void createLayout();
  void createConnections();

  void repopulateFaces(bool fontSwitched = true);
  void repopulateNamedInstances(bool fontSwitched = true);
  void updateFont();
  void updateFace();
  void updateNI();
  void loadTriplet();

  static void nextComboBoxItem(QComboBox* c);
  static void previousComboBoxItem(QComboBox* c);
};


// end of tripletselector.hpp
