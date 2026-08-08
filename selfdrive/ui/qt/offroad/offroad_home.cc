/**
 * Copyright (c) 2021-, Haibin Wen, sunnypilot, and a number of other contributors.
 *
 * This file is part of sunnypilot and is licensed under the MIT License.
 * See the LICENSE.md file in the root directory for more details.
 */

#include "selfdrive/ui/qt/offroad/offroad_home.h"

#include "selfdrive/ui/ui_scale.h"
#include "selfdrive/ui/qt/offroad/experimental_mode.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/widgets/prime.h"

// OffroadHome: the offroad home page

OffroadHome::OffroadHome(QWidget* parent) : QFrame(parent) {
  // 根布局：road 摄像头背景 + 内容叠加（webcam 模式下 offroad 也能看到画面）
  QStackedLayout* root_layout = new QStackedLayout(this);
  root_layout->setStackingMode(QStackedLayout::StackAll);

  camera_widget = new CameraWidget("camerad", VISION_STREAM_ROAD, this);
  camera_widget->setAttribute(Qt::WA_TransparentForMouseEvents);
  root_layout->addWidget(camera_widget);

  QWidget* content_widget = new QWidget(this);
  QVBoxLayout* main_layout = new QVBoxLayout(content_widget);
  main_layout->setContentsMargins(ui_scale::px_w(19), ui_scale::px_h(21), ui_scale::px_w(19), ui_scale::px_h(21));

  // top header
  QHBoxLayout* header_layout = new QHBoxLayout();
  header_layout->setContentsMargins(0, 0, 0, 0);
  header_layout->setSpacing(ui_scale::px_w(8));

  update_notif = new QPushButton(tr("UPDATE"));
  update_notif->setVisible(false);
  update_notif->setStyleSheet("background-color: #364DEF;");
  QObject::connect(update_notif, &QPushButton::clicked, [=]() { center_layout->setCurrentIndex(1); });
  header_layout->addWidget(update_notif, 0, Qt::AlignHCenter | Qt::AlignLeft);

  alert_notif = new QPushButton();
  alert_notif->setVisible(false);
  alert_notif->setStyleSheet("background-color: #E22C2C;");
  QObject::connect(alert_notif, &QPushButton::clicked, [=] { center_layout->setCurrentIndex(2); });
  header_layout->addWidget(alert_notif, 0, Qt::AlignHCenter | Qt::AlignLeft);

  version = new ElidedLabel();
  header_layout->addWidget(version, 0, Qt::AlignHCenter | Qt::AlignRight);

  main_layout->addLayout(header_layout);

  // main content
  main_layout->addSpacing(ui_scale::px_h(13));
  center_layout = new QStackedLayout();

  QWidget *home_widget = new QWidget(this);
  {
    home_layout = new QHBoxLayout(home_widget);
    home_layout->setContentsMargins(0, 0, 0, 0);
    home_layout->setSpacing(ui_scale::px_w(14));

#ifndef SUNNYPILOT
    // left: PrimeAdWidget
    QStackedWidget *left_widget = new QStackedWidget(this);
    QVBoxLayout *left_prime_layout = new QVBoxLayout();
    left_prime_layout->setContentsMargins(0, 0, 0, 0);
    QWidget *prime_user = new PrimeUserWidget();
    prime_user->setStyleSheet(QString(R"(
    border-radius: %1px;
    background-color: #333333;
    )").arg(ui_scale::px_w(4)));
    left_prime_layout->addWidget(prime_user);
    left_prime_layout->addStretch();
    left_widget->addWidget(new LayoutWidget(left_prime_layout));
    left_widget->addWidget(new PrimeAdWidget);
    left_widget->setStyleSheet(QString("border-radius: %1px;").arg(ui_scale::px_w(4)));

    connect(uiState()->prime_state, &PrimeState::changed, [left_widget]() {
      left_widget->setCurrentIndex(uiState()->prime_state->isSubscribed() ? 0 : 1);
    });

    home_layout->addWidget(left_widget, 1);
#endif

    // right: ExperimentalModeButton, SetupWidget
    QWidget* right_widget = new QWidget(this);
    QVBoxLayout* right_column = new QVBoxLayout(right_widget);
    right_column->setContentsMargins(0, 0, 0, 0);
    right_widget->setFixedWidth(ui_scale::px_w(355));
    right_column->setSpacing(ui_scale::px_h(14));

    ExperimentalModeButton *experimental_mode = new ExperimentalModeButton(this);
    QObject::connect(experimental_mode, &ExperimentalModeButton::openSettings, this, &OffroadHome::openSettings);
    right_column->addWidget(experimental_mode, 1);

    SetupWidget *setup_widget = new SetupWidget;
    QObject::connect(setup_widget, &SetupWidget::openSettings, this, &OffroadHome::openSettings);
    right_column->addWidget(setup_widget, 1);

    home_layout->addWidget(right_widget, 1);
  }
  center_layout->addWidget(home_widget);

  // add update & alerts widgets
  update_widget = new UpdateAlert();
  QObject::connect(update_widget, &UpdateAlert::dismiss, [=]() { center_layout->setCurrentIndex(0); });
  center_layout->addWidget(update_widget);
  alerts_widget = new OffroadAlert();
  QObject::connect(alerts_widget, &OffroadAlert::dismiss, [=]() { center_layout->setCurrentIndex(0); });
  center_layout->addWidget(alerts_widget);

  main_layout->addLayout(center_layout, 1);

  // set up refresh timer
  timer = new QTimer(this);
  timer->callOnTimeout(this, &OffroadHome::refresh);

  setStyleSheet(QString(R"(
    * {
      color: white;
    }
    OffroadHome {
      background-color: black;
    }
    OffroadHome > QPushButton {
      padding: %1px %2px;
      border-radius: %3px;
      font-size: %4px;
      font-weight: 236;
    }
    OffroadHome > QLabel {
      font-size: %5px;
    }
  )").arg(ui_scale::px_h(8)).arg(ui_scale::px_w(15)).arg(ui_scale::px_w(3)).arg(ui_scale::px_w(18)).arg(ui_scale::px_w(26)));

  root_layout->addWidget(content_widget);
}

void OffroadHome::showEvent(QShowEvent *event) {
  refresh();
  timer->start(10 * 1000);
}

void OffroadHome::hideEvent(QHideEvent *event) {
  timer->stop();
}

void OffroadHome::refresh() {
  version->setText(getBrand() + " " +  QString::fromStdString(params.get("UpdaterCurrentDescription")));

  bool updateAvailable = update_widget->refresh();
  int alerts = alerts_widget->refresh();

  // pop-up new notification
  int idx = center_layout->currentIndex();
  if (!updateAvailable && !alerts) {
    idx = 0;
  } else if (updateAvailable && (!update_notif->isVisible() || (!alerts && idx == 2))) {
    idx = 1;
  } else if (alerts && (!alert_notif->isVisible() || (!updateAvailable && idx == 1))) {
    idx = 2;
  }
  center_layout->setCurrentIndex(idx);

  update_notif->setVisible(updateAvailable);
  alert_notif->setVisible(alerts);
  if (alerts) {
    alert_notif->setText(QString::number(alerts) + (alerts > 1 ? tr(" ALERTS") : tr(" ALERT")));
  }
}
