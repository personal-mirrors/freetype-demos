// singular.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/glyphindexselector.hpp"
#include "../widgets/fontsizeselector.hpp"
#include "../rendering/glyphbitmap.hpp"
#include "../rendering/glyphoutline.hpp"
#include "../rendering/glyphpointnumbers.hpp"
#include "../rendering/glyphpoints.hpp"
#include "../rendering/grid.hpp"
#include "../rendering/graphicsdefault.hpp"
#include "../engine/engine.hpp"
#include "../models/ttsettingscomboboxmodel.hpp"

#include <QWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>
#include <QLabel>
#include <QComboBox>
#include <QPen>
#include <QCheckBox>
#include <QVector>
#include <QGridLayout>
#include <QBoxLayout>

class SingularTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  SingularTab(QWidget* parent, Engine* engine);
  ~SingularTab() override;

  void repaintGlyph() override;
  void reloadFont() override;

private slots:
  void setGlyphIndex(int);
  void drawGlyph();
  
  void checkShowPoints();

  void zoom();
  void backToCenter();
  void wheelZoom(QWheelEvent* event);
  void wheelResize(QWheelEvent* event);

private:
  int currentGlyphIndex_;
  int currentGlyphCount_;

  Engine* engine_;

  QGraphicsScene* glyphScene_;
  QGraphicsViewx* glyphView_;

  GlyphOutline* currentGlyphOutlineItem_;
  GlyphPoints* currentGlyphPointsItem_;
  GlyphPointNumbers* currentGlyphPointNumbersItem_;
  GlyphBitmap* currentGlyphBitmapItem_;
  Grid* gridItem_ = NULL;
  QLabel* mouseUsageHint_;

  GlyphIndexSelector* indexSelector_;
  FontSizeSelector* sizeSelector_;
  QLabel* zoomLabel_;
  ZoomSpinBox* zoomSpinBox_;
  QPushButton* centerGridButton_;

  QLabel* glyphIndexLabel_;
  QLabel* glyphNameLabel_;

  QCheckBox* showBitmapCheckBox_;
  QCheckBox* showOutlinesCheckBox_;
  QCheckBox* showPointNumbersCheckBox_;
  QCheckBox* showPointsCheckBox_;

  QVBoxLayout* mainLayout_;
  QHBoxLayout* checkBoxesLayout_;
  QHBoxLayout* sizeLayout_;
  QGridLayout* glyphOverlayLayout_;
  QHBoxLayout* glyphOverlayIndexLayout_;

  GraphicsDefault* graphicsDefault_;

  void createLayout();
  void createConnections();
  
  void updateGrid();
  void syncSettings();
  void setDefaults();
};

// end of singular.hpp
