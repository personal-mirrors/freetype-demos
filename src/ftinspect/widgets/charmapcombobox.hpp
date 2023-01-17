// charmapcombobox.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "../engine/charmap.hpp"

#include <vector>

#include <QComboBox>


class Engine;

class CharMapComboBox
: public QComboBox
{
  Q_OBJECT

public:
  CharMapComboBox(QWidget* parent,
                  Engine* engine,
                  bool haveGlyphOrder = true);
  ~CharMapComboBox() override = default;

  bool haveGlyphOrder_;

  std::vector<CharMapInfo>& charMaps() { return charMaps_; }
  int currentCharMapIndex();
  int defaultFirstGlyphIndex();
  void repopulate();
  void repopulate(std::vector<CharMapInfo>& charMaps);

signals:
  void forceUpdateLimitIndex();

private:
  Engine* engine_;
  std::vector<CharMapInfo> charMaps_;

  void updateToolTip();
};


// charmapcombobox.hpp
