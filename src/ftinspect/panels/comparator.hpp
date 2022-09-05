// comparator.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include "abstracttab.hpp"
#include "../engine/engine.hpp"
#include "../widgets/customwidgets.hpp"
#include "../widgets/charmapcombobox.hpp"
#include "../widgets/fontsizeselector.hpp"
#include "../panels/settingpanel.hpp"
#include "../glyphcomponents/glyphcontinuous.hpp"

#include <vector>

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QGridLayout>
#include <QBoxLayout>
#include <QPlainTextEdit>

class ComparatorTab
: public QWidget, public AbstractTab
{
  Q_OBJECT
public:
  ComparatorTab(QWidget* parent, Engine* engine);
  ~ComparatorTab() override;

  void repaintGlyph() override;
  void reloadFont() override;

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  constexpr static int ColumnWidth = 3;

  Engine* engine_;

  FontSizeSelector* sizeSelector_;
  QLabel* charMapLabel_;
  CharMapComboBox* charMapSelector_;
  QPlainTextEdit* sourceTextEdit_;

  std::vector<GlyphContinuous*> canvas_;
  std::vector<SettingPanel*> settingPanels_;
  std::vector<QFrame*> frames_;

  QWidget* sourceWidget_;

  QVBoxLayout* sourceLayout_;
  QGridLayout* layout_;

  void createLayout();
  void createConnections();
  void setupCanvases();
  void forceEqualWidths();

  void reloadStringAndRepaint();
  void reloadGlyphsAndRepaint();
  void sourceTextChanged();
  void applySettings(int index);

  void wheelResize(int steps);
  void wheelZoom(int steps);
};


// end of comparator.hpp
