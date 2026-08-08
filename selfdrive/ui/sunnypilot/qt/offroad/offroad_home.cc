/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/sunnypilot/qt/offroad/offroad_home.h"

#include <QStackedWidget>

#include "selfdrive/ui/ui_scale.h"
#include "selfdrive/ui/sunnypilot/qt/widgets/drive_stats.h"

OffroadHomeSP::OffroadHomeSP(QWidget *parent) : OffroadHome(parent) {
  QStackedWidget *left_widget = new QStackedWidget(this);
  DriveStats *driveStatsWidget = new DriveStats(this);
  driveStatsWidget->setStyleSheet(QString("QLabel[type=\"title\"] { font-size: %1px; font-weight: 237; } QLabel[type=\"number\"] { font-size: %2px; font-weight: 237; } QLabel[type=\"unit\"] { font-size: %3px; font-weight: 142; color: #A0A0A0; }")
    .arg(ui_scale::px_w(24)).arg(ui_scale::px_w(37)).arg(ui_scale::px_w(24)));
  left_widget->addWidget(driveStatsWidget);
  left_widget->setStyleSheet(QString("border-radius: %1px;").arg(ui_scale::px_w(4)));

  home_layout->insertWidget(0, left_widget);
}
