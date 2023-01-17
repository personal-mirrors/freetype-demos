// uihelper.hpp

// Copyright (C) 2022-2023 by
// Charlie Jiang.

#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QWidget>


// We want buttons that are horizontally as small as possible.
void setButtonNarrowest(QPushButton* btn);
void setLabelSelectable(QLabel* label);

// All those grid layout functions rely on `QGridLayout::rowCount`, which
// isn't so reliable since it returns the count of rows *allocated
// internally* inside the layout, but not the actual number of rows occupied
// by layout items.  Always use the returned value to refer to the row
// position, and never mix code using hard-coded row indices with these
// functions.  If you need to add your own rows, use function `rowCount` as
// well or you can increment the last returned row position.
int gridLayout2ColAddLayout(QGridLayout* layout,
                            QLayout* layoutSingle);
int gridLayout2ColAddWidget(QGridLayout* layout,
                            QWidget* widgetSingle);
int gridLayout2ColAddWidget(QGridLayout* layout,
                            QWidget* widgetL,
                            QWidget* widgetR);
int gridLayout2ColAddItem(QGridLayout* layout,
                          QLayoutItem* item);


// end of uihelper.hpp
