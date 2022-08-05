// uihelper.hpp

// Copyright (C) 2022 by Charlie Jiang.

#pragma once

#include <QPushButton>
#include <QLabel>
#include <QLayoutItem>
#include <QWidget>
#include <QGridLayout>

// we want buttons that are horizontally as small as possible
void setButtonNarrowest(QPushButton* btn);
void setLabelSelectable(QLabel* label);
void gridLayout2ColAddLayout(QGridLayout* layout, QLayout* layoutSingle);
void gridLayout2ColAddWidget(QGridLayout* layout, QWidget* widgetSingle);
void gridLayout2ColAddWidget(QGridLayout* layout, 
                             QWidget* widgetL, QWidget* widgetR);
void gridLayout2ColAddItem(QGridLayout* layout, QLayoutItem* item);


// end of uihelper.hpp
