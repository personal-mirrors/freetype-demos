// continuous.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../engine/engine.hpp"
#include "../glyphcomponents/glyphcontinuous.hpp"
#include "../glyphcomponents/graphicsdefault.hpp"
#include "../widgets/charmapcombobox.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/fontsizeselector.hpp"
#include "../widgets/glyphindexselector.hpp"

#include <vector>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QWidget>


class GlyphDetails;
class WaterfallConfigDialog;

class ContinuousTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  ContinuousTab(QWidget* parent,
                Engine* engine,
                QDockWidget* gdWidget,
                GlyphDetails* glyphDetails);
  ~ContinuousTab() override = default;

  void repaintGlyph() override;
  void reloadFont() override;
  void highlightGlyph(int index);
  void applySettings();

signals:
  // Don't change size if `sizePoint` <= 0.
  void switchToSingular(int glyphIndex,
                        double sizePoint);

protected:
  bool eventFilter(QObject* watched,
                   QEvent* event) override;

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

  QDockWidget* glyphDetailsWidget_;
  GlyphDetails* glyphDetails_;

  WaterfallConfigDialog* wfConfigDialog_;

  void createLayout();
  void createConnections();

  void updateLimitIndex();
  void checkModeSource();

  // This doesn't trigger immediate repaint...
  void setGlyphCount(int count);

  // ... but these do.
  void setGlyphBeginindex(int index);
  void checkModeSourceAndRepaint();
  void charMapChanged();
  void sourceTextChanged();
  void presetStringSelected();
  void reloadGlyphsAndRepaint();
  void updateGlyphDetails(GlyphCacheEntry* ctxt,
                          int charMapIndex,
                          bool open);
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
