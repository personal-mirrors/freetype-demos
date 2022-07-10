// continuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/glyphindexselector.hpp"
#include "../widgets/fontsizeselector.hpp"
#include "../rendering/graphicsdefault.hpp"
#include "../rendering/glyphcontinuous.hpp"
#include "../engine/engine.hpp"

#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QVector>
#include <QGridLayout>
#include <QBoxLayout>

class ContinousAllGlyphsTab;

class ContinuousTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  ContinuousTab(QWidget* parent, Engine* engine);
  ~ContinuousTab() override = default;

  void repaintGlyph() override;
  void reloadFont() override;

  // Info about current font (glyph count, charmaps...) is flowed to subtab
  // via `updateCurrentSubTab`.
  // Settings and parameters (e.g. mode) are flowed from subtab to `this` via
  // `updateFromCurrentSubTab`.
  // SubTabs can notify `this` via signals, see `createConnections`
  void updateCurrentSubTab();
  void updateFromCurrentSubTab();

private slots:
  void changeTab();
  void wheelNavigate(int steps);
  void wheelResize(int steps);

private:
  Engine* engine_;

  int currentGlyphCount_;
  GlyphContinuous* canvas_;
  
  FontSizeSelector* sizeSelector_;

  QTabWidget* tabWidget_;
  ContinousAllGlyphsTab* allGlyphsTab_;

  enum Tabs
  {
    AllGlyphs = 0
  };

  QVBoxLayout* mainLayout_;
  
  void createLayout();
  void createConnections();
};


class ContinousAllGlyphsTab
: public QWidget
{
  Q_OBJECT
public:
  explicit ContinousAllGlyphsTab(QWidget* parent);
  ~ContinousAllGlyphsTab() override = default;

  int glyphBeginindex();
  int glyphLimitIndex();
  GlyphContinuous::SubModeAllGlyphs subMode();

  // -1: Glyph order, otherwise the char map index in the original list
  int charMapIndex();
  void setGlyphBeginindex(int index);

  // This doesn't trigger immediate repaint
  void setGlyphCount(int count) { currentGlyphCount_ = count; }
  void setDisplayingCount(int count);

  void setCharMaps(QVector<CharMapInfo>& charMaps);
  // This doesn't trigger either.
  void updateCharMapLimit();

signals:
  void changed();

private:
  int lastCharMapIndex_ = 0;
  int currentGlyphCount_;
  int glyphLimitIndex_ = 0;

  GlyphIndexSelector* indexSelector_;
  QComboBox* modeSelector_;
  QComboBox* charMapSelector_;

  QLabel* modeLabel_;
  QLabel* charMapLabel_;

  QGridLayout* layout_;

  QVector<CharMapInfo> charMaps_;

  void createLayout();
  void createConnections();

  QString formatIndex(int index);
  void charMapChanged();
};


// end of continuous.hpp
