// uihelper.cpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#include "uihelper.hpp"

#include <QFontMetrics>
#include <QString>
#include <QStyleOptionButton>


// Code derived from Qt 4.8.7, function `QPushButton::sizeHint`,
// file `src/gui/widgets/qpushbutton.cpp`.

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


int
gridLayout2ColAddLayout(QGridLayout* layout,
                        QLayout* layoutSingle)
{
  auto r = layout->rowCount();
  layout->addLayout(layoutSingle, r, 0, 1, 2);
  return r;
}


int
gridLayout2ColAddWidget(QGridLayout* layout,
                        QWidget* widgetSingle)
{
  auto r = layout->rowCount();
  layout->addWidget(widgetSingle, r, 0, 1, 2);
  return r;
}


int
gridLayout2ColAddWidget(QGridLayout* layout,
                        QWidget* widgetL,
                        QWidget* widgetR)
{
  auto r = layout->rowCount();
  layout->addWidget(widgetL, r, 0);
  layout->addWidget(widgetR, r, 1);
  return r;
}


int
gridLayout2ColAddItem(QGridLayout* layout,
                      QLayoutItem* item)
{
  auto r = layout->rowCount();
  layout->addItem(item, r, 0, 1, 2);
  return r;
}


// end of uihelper.cpp
