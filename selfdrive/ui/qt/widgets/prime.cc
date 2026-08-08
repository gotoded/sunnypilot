#include "selfdrive/ui/qt/widgets/prime.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <QrCode.hpp>

#include "selfdrive/ui/qt/request_repeater.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/wifi.h"
#include "selfdrive/ui/ui_scale.h"

using qrcodegen::QrCode;

PairingQRWidget::PairingQRWidget(QWidget* parent) : QWidget(parent) {
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &PairingQRWidget::refresh);
}

void PairingQRWidget::showEvent(QShowEvent *event) {
  refresh();
  timer->start(5 * 60 * 1000);
  device()->setOffroadBrightness(100);
}

void PairingQRWidget::hideEvent(QHideEvent *event) {
  timer->stop();
  device()->setOffroadBrightness(BACKLIGHT_OFFROAD);
}

void PairingQRWidget::refresh() {
  QString pairToken = CommaApi::create_jwt({{"pair", true}});
  QString qrString = "https://connect.comma.ai/?pair=" + pairToken;
  this->updateQrCode(qrString);
  update();
}

void PairingQRWidget::updateQrCode(const QString &text) {
  QrCode qr = QrCode::encodeText(text.toUtf8().data(), QrCode::Ecc::LOW);
  qint32 sz = qr.getSize();
  QImage im(sz, sz, QImage::Format_RGB32);

  QRgb black = qRgb(0, 0, 0);
  QRgb white = qRgb(255, 255, 255);
  for (int y = 0; y < sz; y++) {
    for (int x = 0; x < sz; x++) {
      im.setPixel(x, y, qr.getModule(x, y) ? black : white);
    }
  }

  // Integer division to prevent anti-aliasing
  int final_sz = ((width() / sz) - 1) * sz;
  img = QPixmap::fromImage(im.scaled(final_sz, final_sz, Qt::KeepAspectRatio), Qt::MonoOnly);
}

void PairingQRWidget::paintEvent(QPaintEvent *e) {
  QPainter p(this);
  p.fillRect(rect(), Qt::white);

  QSize s = (size() - img.size()) / 2;
  p.drawPixmap(s.width(), s.height(), img);
}


PairingPopup::PairingPopup(QWidget *parent) : DialogBase(parent) {
  QHBoxLayout *hlayout = new QHBoxLayout(this);
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  setStyleSheet("PairingPopup { background-color: #E0E0E0; }");

  // text
  QVBoxLayout *vlayout = new QVBoxLayout();
  vlayout->setContentsMargins(ui_scale::px_w(31), ui_scale::px_h(26), ui_scale::px_w(18), ui_scale::px_h(26));
  vlayout->setSpacing(ui_scale::px_h(18));
  hlayout->addLayout(vlayout, 1);
  {
    QPushButton *close = new QPushButton(QIcon(":/icons/close.svg"), "", this);
    close->setIconSize(QSize(ui_scale::px_w(30), ui_scale::px_h(30)));
    close->setStyleSheet("border: none;");
    vlayout->addWidget(close, 0, Qt::AlignLeft);
    QObject::connect(close, &QPushButton::clicked, this, &QDialog::reject);

    vlayout->addSpacing(ui_scale::px_h(11));

    QLabel *title = new QLabel(tr("Pair your device to your comma account"), this);
    title->setStyleSheet(QString("font-size: %1px; color: black;").arg(ui_scale::px_w(28)));
    title->setWordWrap(true);
    vlayout->addWidget(title);

    QLabel *instructions = new QLabel(QString(R"(
      <ol type='1' style='margin-left: %1px;'>
        <li style='margin-bottom: %2px;'>%3</li>
        <li style='margin-bottom: %4px;'>%5</li>
        <li style='margin-bottom: %6px;'>%7</li>
      </ol>
    )").arg(ui_scale::px_w(6))
    .arg(ui_scale::px_h(18)).arg(tr("Go to https://connect.comma.ai on your phone"))
    .arg(ui_scale::px_h(18)).arg(tr("Click \"add new device\" and scan the QR code on the right"))
    .arg(ui_scale::px_h(18)).arg(tr("Bookmark connect.comma.ai to your home screen to use it like an app")), this);

    instructions->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: black;").arg(ui_scale::px_w(17)));
    instructions->setWordWrap(true);
    vlayout->addWidget(instructions);

    vlayout->addStretch();
  }

  // QR code
  PairingQRWidget *qr = new PairingQRWidget(this);
  hlayout->addWidget(qr, 1);
}

int PairingPopup::exec() {
  if (!util::system_time_valid()) {
    ConfirmationDialog::alert(tr("Please connect to Wi-Fi to complete initial pairing"), parentWidget());
    return QDialog::Rejected;
  }
  return DialogBase::exec();
}


PrimeUserWidget::PrimeUserWidget(QWidget *parent) : QFrame(parent) {
  setObjectName("primeWidget");
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(ui_scale::px_w(21), ui_scale::px_h(15), ui_scale::px_w(21), ui_scale::px_h(15));
  mainLayout->setSpacing(ui_scale::px_h(7));

  QLabel *subscribed = new QLabel(tr("✓ SUBSCRIBED"));
  subscribed->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: #86FF4E;").arg(ui_scale::px_w(15)));
  mainLayout->addWidget(subscribed);

  QLabel *commaPrime = new QLabel(tr("comma prime"));
  commaPrime->setStyleSheet(QString("font-size: %1px; font-weight: bold;").arg(ui_scale::px_w(28)));
  mainLayout->addWidget(commaPrime);
}


PrimeAdWidget::PrimeAdWidget(QWidget* parent) : QFrame(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(ui_scale::px_w(30), ui_scale::px_h(33), ui_scale::px_w(30), ui_scale::px_h(22));
  main_layout->setSpacing(0);

  QLabel *upgrade = new QLabel(tr("Upgrade Now"));
  upgrade->setStyleSheet(QString("font-size: %1px; font-weight: bold;").arg(ui_scale::px_w(28)));
  main_layout->addWidget(upgrade, 0, Qt::AlignTop);
  main_layout->addSpacing(ui_scale::px_h(18));

  QLabel *description = new QLabel(tr("Become a comma prime member at connect.comma.ai"));
  description->setStyleSheet(QString("font-size: %1px; font-weight: light; color: white;").arg(ui_scale::px_w(21)));
  description->setWordWrap(true);
  main_layout->addWidget(description, 0, Qt::AlignTop);

  main_layout->addStretch();

  QLabel *features = new QLabel(tr("PRIME FEATURES:"));
  features->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: #E5E5E5;").arg(ui_scale::px_w(15)));
  main_layout->addWidget(features, 0, Qt::AlignBottom);
  main_layout->addSpacing(ui_scale::px_h(11));

  QVector<QString> bullets = {tr("Remote access"), tr("24/7 LTE connectivity"), tr("1 year of drive storage"), tr("Remote snapshots")};
  for (auto &b : bullets) {
    const QString check = "<b><font color='#465BEA'>✓</font></b> ";
    QLabel *l = new QLabel(check + b);
    l->setAlignment(Qt::AlignLeft);
    l->setStyleSheet(QString("font-size: %1px; margin-bottom: %2px;").arg(ui_scale::px_w(18)).arg(ui_scale::px_h(5)));
    main_layout->addWidget(l, 0, Qt::AlignBottom);
  }

  setStyleSheet(QString(R"(
    PrimeAdWidget {
      border-radius: %1px;
      background-color: #333333;
    }
  )").arg(ui_scale::px_w(4)));
}


SetupWidget::SetupWidget(QWidget* parent) : QFrame(parent) {
  mainLayout = new QStackedWidget;

  // Unpaired, registration prompt layout

  QFrame* finishRegistration = new QFrame;
  finishRegistration->setObjectName("primeWidget");
  QVBoxLayout* finishRegistrationLayout = new QVBoxLayout(finishRegistration);
  finishRegistrationLayout->setSpacing(ui_scale::px_h(14));
  finishRegistrationLayout->setContentsMargins(ui_scale::px_w(24), ui_scale::px_h(18), ui_scale::px_w(24), ui_scale::px_h(18));

  QLabel* registrationTitle = new QLabel(tr("Finish Setup"));
  registrationTitle->setStyleSheet(QString("font-size: %1px; font-weight: bold;").arg(ui_scale::px_w(28)));
  finishRegistrationLayout->addWidget(registrationTitle);

  QLabel* registrationDescription = new QLabel(tr("Pair your device with comma connect (connect.comma.ai) and claim your comma prime offer."));
  registrationDescription->setWordWrap(true);
  registrationDescription->setStyleSheet(QString("font-size: %1px; font-weight: light;").arg(ui_scale::px_w(18)));
  finishRegistrationLayout->addWidget(registrationDescription);

  finishRegistrationLayout->addStretch();

  QPushButton* pair = new QPushButton(tr("Pair device"));
  pair->setStyleSheet(QString(R"(
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
  )").arg(ui_scale::px_w(20)).arg(ui_scale::px_w(4)).arg(ui_scale::px_h(24)).arg(ui_scale::px_w(24)));
  finishRegistrationLayout->addWidget(pair);

  popup = new PairingPopup(this);
  QObject::connect(pair, &QPushButton::clicked, popup, &PairingPopup::exec);

  mainLayout->addWidget(finishRegistration);

  // build stacked layout
  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setContentsMargins(0, 0, 0, 0);
  outer_layout->addWidget(mainLayout);

  QWidget *content = new QWidget;
  content_layout = new QVBoxLayout(content);
  content_layout->setContentsMargins(0, 0, 0, 0);
  content_layout->setSpacing(ui_scale::px_h(11));

  WiFiPromptWidget *wifi_prompt = new WiFiPromptWidget;
  QObject::connect(wifi_prompt, &WiFiPromptWidget::openSettings, this, &SetupWidget::openSettings);
  content_layout->addWidget(wifi_prompt);
  content_layout->addStretch();

  mainLayout->addWidget(content);

  mainLayout->setCurrentIndex(1);

  setStyleSheet(QString(R"(
    #primeWidget {
      border-radius: %1px;
      background-color: #333333;
    }
  )").arg(ui_scale::px_w(4)));

  // Retain size while hidden
  QSizePolicy sp_retain = sizePolicy();
  sp_retain.setRetainSizeWhenHidden(true);
  setSizePolicy(sp_retain);

  QObject::connect(uiState()->prime_state, &PrimeState::changed, [this](PrimeState::Type type) {
    if (type == PrimeState::PRIME_TYPE_UNPAIRED) {
      mainLayout->setCurrentIndex(0);  // Display "Pair your device" widget
    } else {
      popup->reject();
      mainLayout->setCurrentIndex(1);  // Display Wi-Fi prompt widget
    }
  });
}
