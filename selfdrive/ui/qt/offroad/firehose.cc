#include "selfdrive/ui/qt/offroad/firehose.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QStackedLayout>
#include <QProgressBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include "selfdrive/ui/ui_scale.h"

#ifdef SUNNYPILOT
#define UIState UIStateSP
#endif

FirehosePanel::FirehosePanel(SettingsWindow *parent) : QWidget((QWidget*)parent) {
  layout = new QVBoxLayout(this);
  layout->setContentsMargins(ui_scale::px_w(19), ui_scale::px_h(21), ui_scale::px_w(19), ui_scale::px_h(21));
  layout->setSpacing(ui_scale::px_h(9));

  // header
  QLabel *title = new QLabel(tr("🔥 Firehose Mode 🔥"));
  title->setStyleSheet(QString("font-size: %1px; font-weight: 237; font-family: 'Noto Color Emoji';").arg(ui_scale::px_w(47)));
  layout->addWidget(title, 0, Qt::AlignCenter);

  // Create a container for the content
  QFrame *content = new QFrame();
  content->setStyleSheet(QString("background-color: #292929; border-radius: %1px; padding: %2px %3px %2px %3px;").arg(ui_scale::px_w(8)).arg(ui_scale::px_h(10)).arg(ui_scale::px_w(10)));
  QVBoxLayout *content_layout = new QVBoxLayout(content);
  content_layout->setSpacing(ui_scale::px_h(9));

  // Top description
  QLabel *description = new QLabel(tr("openpilot learns to drive by watching humans, like you, drive.\n\nFirehose Mode allows you to maximize your training data uploads to improve openpilot's driving models. More data means bigger models, which means better Experimental Mode."));
  description->setStyleSheet(QString("font-size: %1px; padding-bottom: %2px;").arg(ui_scale::px_w(20)).arg(ui_scale::px_h(11)));
  description->setWordWrap(true);
  content_layout->addWidget(description);

  // Add a separator
  QFrame *line = new QFrame();
  line->setFrameShape(QFrame::HLine);
  line->setFrameShadow(QFrame::Sunken);
  line->setStyleSheet(QString("background-color: #444444; margin-top: %1px; margin-bottom: %1px;").arg(ui_scale::px_h(3)));
  content_layout->addWidget(line);

  toggle_label = new QLabel(tr("Firehose Mode: ACTIVE"));
  toggle_label->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: white;").arg(ui_scale::px_w(28)));
  content_layout->addWidget(toggle_label);

  // Add contribution label
  contribution_label = new QLabel();
  contribution_label->setStyleSheet(QString("font-size: %1px; margin-top: %2px; margin-bottom: %2px;").arg(ui_scale::px_w(26)).arg(ui_scale::px_h(6)));
  contribution_label->setWordWrap(true);
  contribution_label->hide();
  content_layout->addWidget(contribution_label);

  // Add a separator before detailed instructions
  QFrame *line2 = new QFrame();
  line2->setFrameShape(QFrame::HLine);
  line2->setFrameShadow(QFrame::Sunken);
  line2->setStyleSheet(QString("background-color: #444444; margin-top: %1px; margin-bottom: %1px;").arg(ui_scale::px_h(6)));
  content_layout->addWidget(line2);

  // Detailed instructions at the bottom
  detailed_instructions = new QLabel(tr(
    "For maximum effectiveness, bring your device inside and connect to a good USB-C adapter and Wi-Fi weekly.<br>"
    "<br>"
    "Firehose Mode can also work while you're driving if connected to a hotspot or unlimited SIM card.<br>"
    "<br><br>"
    "<b>Frequently Asked Questions</b><br><br>"
    "<i>Does it matter how or where I drive?</i> Nope, just drive as you normally would.<br><br>"
    "<i>Do all of my segments get pulled in Firehose Mode?</i> No, we selectively pull a subset of your segments.<br><br>"
    "<i>What's a good USB-C adapter?</i> Any fast phone or laptop charger should be fine.<br><br>"
    "<i>Does it matter which software I run?</i> Yes, only upstream openpilot (and particular forks) are able to be used for training."
  ));
  detailed_instructions->setStyleSheet(QString("font-size: %1px; color: #E4E4E4;").arg(ui_scale::px_w(18)));
  detailed_instructions->setWordWrap(true);
  content_layout->addWidget(detailed_instructions);

  layout->addWidget(content, 1);

  // Set up the API request for firehose stats
  const QString dongle_id = QString::fromStdString(Params().get("DongleId"));
  firehose_stats = new RequestRepeater(this, CommaApi::BASE_URL + "/v1/devices/" + dongle_id + "/firehose_stats",
                                       "ApiCache_FirehoseStats", 30, true);
  QObject::connect(firehose_stats, &RequestRepeater::requestDone, [=](const QString &response, bool success) {
    if (success) {
      QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
      QJsonObject json = doc.object();
      int count = json["firehose"].toInt();
      contribution_label->setText(tr("<b>%n segment(s)</b> of your driving is in the training dataset so far.", "", count));
      contribution_label->show();
    }
  });

  QObject::connect(uiState(), &UIState::uiUpdate, this, &FirehosePanel::refresh);
}

void FirehosePanel::refresh() {
  auto deviceState = (*uiState()->sm)["deviceState"].getDeviceState();
  auto networkType = deviceState.getNetworkType();
  bool networkMetered = deviceState.getNetworkMetered();

  bool is_active = !networkMetered && (networkType != cereal::DeviceState::NetworkType::NONE);
  if (is_active) {
    toggle_label->setText(tr("ACTIVE"));
    toggle_label->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: #2ecc71;").arg(ui_scale::px_w(28)));
  } else {
    toggle_label->setText(tr("<span stylesheet='font-size: %1px; font-weight: bold; color: #e74c3c;'>INACTIVE</span>: connect to unmetered network").arg(ui_scale::px_w(28)));
    toggle_label->setStyleSheet(QString("font-size: %1px;").arg(ui_scale::px_w(28)));
  }
}
