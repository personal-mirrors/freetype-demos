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


// end of uihelper.cpp
