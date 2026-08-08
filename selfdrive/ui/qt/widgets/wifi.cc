#include "selfdrive/ui/qt/widgets/wifi.h"
#include "selfdrive/ui/ui_scale.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>

WiFiPromptWidget::WiFiPromptWidget(QWidget *parent) : QFrame(parent) {
  // Setup Firehose Mode
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(ui_scale::px_w(21), ui_scale::px_h(15), ui_scale::px_w(21), ui_scale::px_h(15));
  main_layout->setSpacing(ui_scale::px_h(16));

  QLabel *title = new QLabel(tr("<span style='font-family: \"Noto Color Emoji\";'>🔥</span> Firehose Mode <span style='font-family: Noto Color Emoji;'>🔥</span>"));
  title->setStyleSheet(QString("font-size: %1px; font-weight: 185;").arg(ui_scale::px_w(24)));
  main_layout->addWidget(title);

  QLabel *desc = new QLabel(tr("Maximize your training data uploads to improve openpilot's driving models."));
  desc->setStyleSheet(QString("font-size: %1px; font-weight: 148;").arg(ui_scale::px_w(15)));
  desc->setWordWrap(true);
  main_layout->addWidget(desc);

  QPushButton *settings_btn = new QPushButton(tr("Open"));
  connect(settings_btn, &QPushButton::clicked, [=]() { emit openSettings(1, "FirehosePanel"); });
  settings_btn->setStyleSheet(QString(R"(
    QPushButton {
      font-size: %1px;
      font-weight: 185;
      border-radius: %2px;
      background-color: #465BEA;
      padding: %3px %4px %3px %4px;
    }
    QPushButton:pressed {
      background-color: #3049F4;
    }
  )").arg(ui_scale::px_w(18)).arg(ui_scale::px_w(4)).arg(ui_scale::px_h(12)).arg(ui_scale::px_w(12)));
  main_layout->addWidget(settings_btn);

  setStyleSheet(QString(R"(
    WiFiPromptWidget {
      background-color: #333333;
      border-radius: %1px;
    }
  )").arg(ui_scale::px_w(4)));
}