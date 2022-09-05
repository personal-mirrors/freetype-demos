// continuous.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/glyphindexselector.hpp"
#include "../widgets/fontsizeselector.hpp"
#include "../widgets/charmapcombobox.hpp"
#include "../glyphcomponents/graphicsdefault.hpp"
#include "../glyphcomponents/glyphcontinuous.hpp"
#include "../engine/engine.hpp"

#include <vector>
#include <QWidget>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include <QBoxLayout>
#include <QPlainTextEdit>
#include <QCheckBox>

class WaterfallConfigDialog;
class ContinuousTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  ContinuousTab(QWidget* parent, Engine* engine);
  ~ContinuousTab() override = default;

  void repaintGlyph() override;
  void reloadFont() override;
  void highlightGlyph(int index);
  void applySettings();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  
private:
  Engine* engine_;

  int currentGlyphCount_;
  int lastCharMapIndex_ = 0;
  int glyphLimitIndex_ = 0;

  GlyphContinuous* canvas_;
  QFrame* canvasFrame_;
  FontSizeSelector* sizeSelector_;

  QComboBox* modeSelector_;
  QComboBox* sourceSelector_;
  CharMapComboBox* charMapSelector_ = NULL;
  QComboBox* sampleStringSelector_;

  QPushButton* resetPositionButton_;
  QPushButton* waterfallConfigButton_;
  QPushButton* helpButton_;

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
  
  QHBoxLayout* canvasFrameLayout_;
  QHBoxLayout* sizeHelpLayout_;
  QGridLayout* bottomLayout_;
  QVBoxLayout* mainLayout_;

  WaterfallConfigDialog* wfConfigDialog_;

  void createLayout();
  void createConnections();

  void updateLimitIndex();
  void checkModeSource();

  // This doesn't trigger immediate repaint
  void setGlyphCount(int count);

  // But they do
  void setGlyphBeginindex(int index);
  void checkModeSourceAndRepaint();
  void charMapChanged();
  void sourceTextChanged();
  void presetStringSelected();
  void reloadGlyphsAndRepaint();
  void openWaterfallConfig();
  void showToolTip();

  void wheelNavigate(int steps);
  void wheelZoom(int steps);
  void wheelResize(int steps);

  void setDefaults();
  QString formatIndex(int index);
};


class WaterfallConfigDialog
: public QDialog
{
  Q_OBJECT

public:
  WaterfallConfigDialog(QWidget* parent);

  double startSize();
  double endSize();

signals:
  void sizeUpdated();

private:
  QLabel* startLabel_;
  QLabel* endLabel_;

  QDoubleSpinBox* startSpinBox_;
  QDoubleSpinBox* endSpinBox_;

  QCheckBox* autoBox_;

  QGridLayout* layout_;

  void createLayout();
  void createConnections();

  void checkAutoStatus();
};


// end of continuous.hpp
