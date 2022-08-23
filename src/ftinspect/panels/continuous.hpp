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
#include <QDockWidget>
#include <QCheckBox>

class GlyphDetails;
class WaterfallConfigDialog;
class ContinuousTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  ContinuousTab(QWidget* parent, Engine* engine,
                QDockWidget* gdWidget, GlyphDetails* glyphDetails);
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

  // This doesn't trigger either.
  void updateLimitIndex();
  void checkModeSource();

  // But they do
  void checkModeSourceAndRepaint();
  void charMapChanged();
  void sourceTextChanged();
  void presetStringSelected();
  void reloadGlyphsAndRepaint();
  void changeBeginIndexFromCanvas(int index);
  void updateGlyphDetails(GlyphCacheEntry* ctxt, 
                          int charMapIndex, 
                          bool open);
  void openWaterfallConfig();

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
  QFrame* canvasFrame_;
  FontSizeSelector* sizeSelector_;

  QComboBox* modeSelector_;
  QComboBox* sourceSelector_;
  CharMapComboBox* charMapSelector_ = NULL;
  QComboBox* sampleStringSelector_;

  QPushButton* resetPositionButton_;
  QPushButton* waterfallConfigButton_;

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
  QGridLayout* bottomLayout_;
  QVBoxLayout* mainLayout_;

  QDockWidget* glyphDetailsWidget_;
  GlyphDetails* glyphDetails_;

  WaterfallConfigDialog* wfConfigDialog_;

  void createLayout();
  void createConnections();

  QString formatIndex(int index);

  void setDefaults();
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
