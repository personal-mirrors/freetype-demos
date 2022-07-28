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

#include <vector>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QCheckBox>

class ContinuousTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  ContinuousTab(QWidget* parent, Engine* engine);
  ~ContinuousTab() override = default;

  void repaintGlyph() override;
  void reloadFont() override;
  void syncSettings();

  // -1: Glyph order, otherwise the char map index in the original list
  int charMapIndex();

  // This doesn't trigger immediate repaint
  void setGlyphCount(int count);
  void setDisplayingCount(int count);
  void setGlyphBeginindex(int index);

  void setCharMaps(std::vector<CharMapInfo>& charMaps);
  // This doesn't trigger either.
  void updateLimitIndex();
  void checkModeSource();
  void charMapChanged();
  void sourceTextChanged();
  void reloadGlyphsAndRepaint();
  void changeBeginIndexFromCanvas(int index);

signals:
  void switchToSingular(int glyphIndex, double sizePoint);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
  void wheelNavigate(int steps);
  void wheelResize(int steps);

private:
  Engine* engine_;

  int currentGlyphCount_;
  int lastCharMapIndex_ = 0;
  int glyphLimitIndex_ = 0;

  GlyphContinuous* canvas_;
  FontSizeSelector* sizeSelector_;

  QComboBox* modeSelector_;
  QComboBox* sourceSelector_;
  QComboBox* charMapSelector_;

  QPushButton* resetPositionButton_;

  QLabel* modeLabel_;
  QLabel* sourceLabel_;
  QLabel* charMapLabel_;
  QLabel* xEmboldeningLabel_;
  QLabel* yEmboldeningLabel_;
  QLabel* slantLabel_;
  QLabel* strokeRadiusLabel_;
  QLabel* rotationLabel_;

  QDoubleSpinBox* xEmboldeningSpinBox_;
  QDoubleSpinBox* yEmboldeningSpinBox_;
  QDoubleSpinBox* slantSpinBox_;
  QDoubleSpinBox* strokeRadiusSpinBox_;
  QDoubleSpinBox* rotationSpinBox_;

  QCheckBox* verticalCheckBox_;
  QCheckBox* waterfallCheckBox_;
  QCheckBox* kerningCheckBox_;

  GlyphIndexSelector* indexSelector_;
  QPlainTextEdit* sourceTextEdit_;

  std::vector<CharMapInfo> charMaps_;
  
  QGridLayout* bottomLayout_;
  QVBoxLayout* mainLayout_;

  void createLayout();
  void createConnections();

  QString formatIndex(int index);

  void setDefaults();
};


// end of continuous.hpp
