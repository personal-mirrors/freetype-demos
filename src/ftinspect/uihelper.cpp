// uihelper.cpp

// Copyright (C) 2022 by Charlie Jiang.

#include "uihelper.hpp"

#include <QStyleOptionButton>
#include <QFontMetrics>
#include <QString>

// code derived from Qt 4.8.7, function `QPushButton::sizeHint',
// file `src/gui/widgets/qpushbutton.cpp'

void
setButtonNarrowest(QPushButton* btn)
{
  QStyleOptionButton opt;
  opt.initFrom(btn);
  QString s(btn->text());
  QFontMetrics fm = btn->fontMetrics();
  QSize sz = fm.size(Qt::TextShowMnemonic, s);
  opt.rect.setSize(sz);

  sz = btn->style()->sizeFromContents(QStyle::CT_PushButton, &opt, sz, btn);
  btn->setFixedWidth(sz.width());
}


void
setLabelSelectable(QLabel* label)
{

  label->setTextInteractionFlags(Qt::TextSelectableByMouse
                                 | Qt::TextSelectableByKeyboard);
  label->setCursor(Qt::IBeamCursor);
}


void
gridLayout2ColAddLayout(QGridLayout* layout,
                        QLayout* layoutSingle)
{
  layout->addLayout(layoutSingle, layout->rowCount(), 0, 1, 2);
}


void
gridLayout2ColAddWidget(QGridLayout* layout,
                        QWidget* widgetSingle)
{
  layout->addWidget(widgetSingle, layout->rowCount(), 0, 1, 2);
}


void
gridLayout2ColAddWidget(QGridLayout* layout,
                        QWidget* widgetL, QWidget* widgetR)
{
  auto r = layout->rowCount();
  layout->addWidget(widgetL, r, 0);
  layout->addWidget(widgetR, r, 1);
}


void
gridLayout2ColAddItem(QGridLayout* layout,
                      QLayoutItem* item)
{
  layout->addItem(item, layout->rowCount(), 0, 1, 2);
}


// end of uihelper.cpp
