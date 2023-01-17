// singular.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../engine/engine.hpp"
#include "../glyphcomponents/glyphbitmap.hpp"
#include "../glyphcomponents/glyphoutline.hpp"
#include "../glyphcomponents/glyphpointnumbers.hpp"
#include "../glyphcomponents/glyphpoints.hpp"
#include "../glyphcomponents/graphicsdefault.hpp"
#include "../glyphcomponents/grid.hpp"
#include "../models/customcomboboxmodels.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/glyphindexselector.hpp"
#include "../widgets/fontsizeselector.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGridLayout>
#include <QLabel>
#include <QPen>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QVector>
#include <QWidget>


class SingularTab
: public QWidget,
  public AbstractTab
{
  Q_OBJECT

public:
  SingularTab(QWidget* parent,
              Engine* engine);
  ~SingularTab() override;

  void repaintGlyph() override;
  void reloadFont() override;
  // The size remains unchanged if `sizePoint` <= 0.
  void setCurrentGlyphAndSize(int glyphIndex,
                              double sizePoint);
  int currentGlyph();

private slots:
  void setGlyphIndex(int);
  void drawGlyph();

  void checkShowPoints();

  void zoom();
  void backToCenter();
  void wheelZoom(QWheelEvent* event);
  void wheelResize(QWheelEvent* event);
  void setGridVisible();
  void showToolTip();

protected:
  bool eventFilter(QObject* watched,
                   QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  int currentGlyphIndex_ = 0;
  int currentGlyphCount_ = 0;

  Engine* engine_;

  QGraphicsScene* glyphScene_;
  QGraphicsViewx* glyphView_;

  GlyphOutline* currentGlyphOutlineItem_;
  GlyphPoints* currentGlyphPointsItem_;
  GlyphPointNumbers* currentGlyphPointNumbersItem_;
  GlyphBitmap* currentGlyphBitmapItem_;
  Grid* gridItem_ = NULL;

  GlyphIndexSelector* indexSelector_;
  FontSizeSelector* sizeSelector_;
  QPushButton* centerGridButton_;
  QPushButton* helpButton_;

  QLabel* glyphIndexLabel_;
  QLabel* glyphNameLabel_;

  QCheckBox* showBitmapCheckBox_;
  QCheckBox* showOutlinesCheckBox_;
  QCheckBox* showPointNumbersCheckBox_;
  QCheckBox* showPointsCheckBox_;
  QCheckBox* showGridCheckBox_;
  QCheckBox* showAuxLinesCheckBox_;

  QVBoxLayout* mainLayout_;
  QHBoxLayout* checkBoxesLayout_;
  QHBoxLayout* indexHelpLayout_;
  QHBoxLayout* sizeLayout_;
  QGridLayout* glyphOverlayLayout_;
  QHBoxLayout* glyphOverlayIndexLayout_;

  GraphicsDefault* graphicsDefault_;

  int initialPositionSetCount_ = 2; // See `resizeEvent`.

  void createLayout();
  void createConnections();

  void updateGrid();
  void applySettings();
  void setDefaults();
};


// end of singular.hpp
