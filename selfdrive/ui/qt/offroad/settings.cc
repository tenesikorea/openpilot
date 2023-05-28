#include "selfdrive/ui/qt/offroad/settings.h"

#include <cassert>
#include <cmath>
#include <string>

#include <QDebug>

#include "selfdrive/ui/qt/offroad/networking.h"

#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_settings.h"
#endif

#include "common/params.h"
#include "common/watchdog.h"
#include "common/util.h"
#include "system/hardware/hw.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"
#include "selfdrive/ui/qt/widgets/toggle.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"
#include "selfdrive/ui/qt/widgets/input.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QScroller>
#include <QListView>
#include <QListWidget>
#include <QProcess> //tmux 터미널등

TogglesPanel::TogglesPanel(SettingsWindow *parent) : ListWidget(parent) {
  // param, title, desc, icon, confirm
  std::vector<std::tuple<QString, QString, QString, QString>> toggle_defs{
    {
      "OpenpilotEnabledToggle",
      tr("Enable openpilot"),
      tr("Use the openpilot system for adaptive cruise control and lane keep driver assistance. Your attention is required at all times to use this feature. Changing this setting takes effect when the car is powered off."),
      "../assets/offroad/icon_openpilot.png",
    },
    {
      "ExperimentalMode",
      tr("Experimental Mode"),
      "",
      "../assets/img_experimental_white.svg",
    },
    {
      "ExperimentalLongitudinalEnabled",
      tr("openpilot Longitudinal Control (Alpha)"),
      QString("<b>%1</b><br><br>%2")
      .arg(tr("WARNING: openpilot longitudinal control is in alpha for this car and will disable Automatic Emergency Braking (AEB)."))
      .arg(tr("On this car, openpilot defaults to the car's built-in ACC instead of openpilot's longitudinal control. Enable this to switch to openpilot longitudinal control. Enabling Experimental mode is recommended when enabling openpilot longitudinal control alpha.")),
      "../assets/offroad/icon_speed_limit.png",
    },
    {
      "IsLdwEnabled",
      tr("Enable Lane Departure Warnings"),
      tr("Receive alerts to steer back into the lane when your vehicle drifts over a detected lane line without a turn signal activated while driving over 31 mph (50 km/h)."),
      "../assets/offroad/icon_warning.png",
    },
    {
      "IsMetric",
      tr("Use Metric System"),
      tr("Display speed in km/h instead of mph."),
      "../assets/offroad/icon_metric.png",
    },
    {
      "RecordFront",
      tr("Record and Upload Driver Camera"),
      tr("Upload data from the driver facing camera and help improve the driver monitoring algorithm."),
      "../assets/offroad/icon_monitoring.png",
    },
    {
      "DisengageOnAccelerator",
      tr("Disengage on Accelerator Pedal"),
      tr("When enabled, pressing the accelerator pedal will disengage openpilot."),
      "../assets/offroad/icon_disengage_on_accelerator.svg",
    },
#ifdef ENABLE_MAPS
    {
      "NavSettingTime24h",
      tr("Show ETA in 24h Format"),
      tr("Use 24h format instead of am/pm"),
      "../assets/offroad/icon_metric.png",
    },
    {
      "NavSettingLeftSide",
      tr("Show Map on Left Side of UI"),
      tr("Show map on left side when in split screen view."),
      "../assets/offroad/icon_road.png",
    },
#endif
  };

  for (auto &[param, title, desc, icon] : toggle_defs) {
    auto toggle = new ParamControl(param, title, desc, icon, this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
  }

  // Toggles with confirmation dialogs
  toggles["ExperimentalMode"]->setActiveIcon("../assets/img_experimental.svg");
  toggles["ExperimentalMode"]->setConfirmation(true, true);
  toggles["ExperimentalLongitudinalEnabled"]->setConfirmation(true, false);

  connect(toggles["ExperimentalLongitudinalEnabled"], &ToggleControl::toggleFlipped, [=]() {
    updateToggles();
  });
}

void TogglesPanel::expandToggleDescription(const QString &param) {
  toggles[param.toStdString()]->showDescription();
}

void TogglesPanel::showEvent(QShowEvent *event) {
  updateToggles();
}

void TogglesPanel::updateToggles() {
  auto e2e_toggle = toggles["ExperimentalMode"];
  auto op_long_toggle = toggles["ExperimentalLongitudinalEnabled"];
  const QString e2e_description = QString("%1<br>"
                                          "<h4>%2</h4><br>"
                                          "%3<br>"
                                          "<h4>%4</h4><br>"
                                          "%5")
                                  .arg(tr("openpilot defaults to driving in <b>chill mode</b>. Experimental mode enables <b>alpha-level features</b> that aren't ready for chill mode. Experimental features are listed below:"))
                                  .arg(tr("🌮 End-to-End Longitudinal Control 🌮"))
                                  .arg(tr("Let the driving model control the gas and brakes. openpilot will drive as it thinks a human would, including stopping for red lights and stop signs. "
                                       "Since the driving model decides the speed to drive, the set speed will only act as an upper bound. This is an alpha quality feature; mistakes should be expected."))
                                  .arg(tr("New Driving Visualization"))
                                  .arg(tr("The driving visualization will transition to the road-facing wide-angle camera at low speeds to better show some turns. The Experimental mode logo will also be shown in the top right corner."));

  const bool is_release = params.getBool("IsReleaseBranch");
  auto cp_bytes = params.get("CarParamsPersistent");
  if (!cp_bytes.empty()) {
    AlignedBuffer aligned_buf;
    capnp::FlatArrayMessageReader cmsg(aligned_buf.align(cp_bytes.data(), cp_bytes.size()));
    cereal::CarParams::Reader CP = cmsg.getRoot<cereal::CarParams>();

    if (!CP.getExperimentalLongitudinalAvailable() || is_release) {
      params.remove("ExperimentalLongitudinalEnabled");
    }
    op_long_toggle->setVisible(CP.getExperimentalLongitudinalAvailable() && !is_release);

    const bool op_long = CP.getOpenpilotLongitudinalControl() && !CP.getExperimentalLongitudinalAvailable();
    const bool exp_long_enabled = CP.getExperimentalLongitudinalAvailable() && params.getBool("ExperimentalLongitudinalEnabled");
    if (op_long || exp_long_enabled) {
      // normal description and toggle
      e2e_toggle->setEnabled(true);
      e2e_toggle->setDescription(e2e_description);
    } else {
      // no long for now
      e2e_toggle->setEnabled(false);
      params.remove("ExperimentalMode");

      const QString unavailable = tr("Experimental mode is currently unavailable on this car since the car's stock ACC is used for longitudinal control.");

      QString long_desc = unavailable + " " + \
                          tr("openpilot longitudinal control may come in a future update.");
      if (CP.getExperimentalLongitudinalAvailable()) {
        if (is_release) {
          long_desc = unavailable + " " + tr("An experimental version of openpilot longitudinal control can be tested, along with Experimental mode, on non-release branches.");
        } else {
          long_desc = tr("Enable experimental longitudinal control to allow Experimental mode.");
        }
      }
      e2e_toggle->setDescription("<b>" + long_desc + "</b><br><br>" + e2e_description);
    }

    e2e_toggle->refresh();
  } else {
    e2e_toggle->setDescription(e2e_description);
    op_long_toggle->setVisible(false);
  }
}

DevicePanel::DevicePanel(SettingsWindow *parent) : ListWidget(parent) {
  setSpacing(50);
  addItem(new LabelControl(tr("Dongle ID"), getDongleId().value_or(tr("N/A"))));
  addItem(new LabelControl(tr("Serial"), params.get("HardwareSerial").c_str()));

  QHBoxLayout *reset_layout = new QHBoxLayout();
  reset_layout->setSpacing(30);

  /*QPushButton *restart_openpilot_btn = new QPushButton(tr("Soft restart"));
  restart_openpilot_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #393939;");
  reset_layout->addWidget(restart_openpilot_btn);
  QObject::connect(restart_openpilot_btn, &QPushButton::released, [=]() {
    emit closeSettings();
    QTimer::singleShot(100, []() {
      Hardware::reboot();
    });
  });*/

  QPushButton *restart_openpilot_btn = new QPushButton(tr("Reboot"));
  restart_openpilot_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #393939;");
  reset_layout->addWidget(restart_openpilot_btn);
  QObject::connect(restart_openpilot_btn, &QPushButton::released, [=]() {
    std::system("rm /data/openpilot/prebuilt");
    //QProcess::execute("/data/openpilot/selfdrive/assets/dtc/restart.sh");
    emit closeSettings();
    QTimer::singleShot(100, []() {
      Hardware::reboot();
    });
  });

  // 고장코드 삭제 메뉴 dtc_btn MDPS DTC
  QPushButton *dtc_btn = new QPushButton(tr("MDPS DTC"));
  dtc_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #E22C2C;");
  reset_layout->addWidget(dtc_btn);
  const char* dtc_run = "/data/openpilot/selfdrive/assets/dtc/dtc.sh ''";
  QObject::connect(dtc_btn, &QPushButton::released, [=]() {
    //if (ConfirmationDialog::confirm(tr("제네시스DH MDPS 고장코드 삭제! \n 엔튠에서 재부팅을 하세요!!! "), tr("실 행?"), this)) {
      std::system(dtc_run);
      //std::system("touch /data/openpilot/prebuilt");
    /*emit closeSettings();
    QTimer::singleShot(100, []() {
      Hardware::reboot();
    });*/
    //}
  });

  // reset calibration button
  QPushButton *reset_calib_btn = new QPushButton(tr("Reset Calibration"));
  reset_calib_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #393939;");
  reset_layout->addWidget(reset_calib_btn);
  QObject::connect(reset_calib_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to reset calibration and live params?"), tr("Reset"), this)) {
      Params().remove("CalibrationParams");
      Params().remove("LiveParameters");
      emit closeSettings();
      QTimer::singleShot(100, []() {
        Hardware::reboot();
      });
    }
  });

  addItem(reset_layout);

  // offroad-only buttons

  auto dcamBtn = new ButtonControl(tr("Driver Camera"), tr("PREVIEW"),
                                   tr("Preview the driver facing camera to ensure that driver monitoring has good visibility. (vehicle must be off)"));
  connect(dcamBtn, &ButtonControl::clicked, [=]() { emit showDriverView(); });
  addItem(dcamBtn);

  auto resetCalibBtn = new ButtonControl(tr("Reset Calibration"), tr("RESET"), "");
  connect(resetCalibBtn, &ButtonControl::showDescriptionEvent, this, &DevicePanel::updateCalibDescription);
  connect(resetCalibBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to reset calibration?"), tr("Reset"), this)) {
      params.remove("CalibrationParams");
      params.remove("LiveTorqueParameters");
    }
  });
  addItem(resetCalibBtn);

  if (!params.getBool("Passive")) {
    auto retrainingBtn = new ButtonControl(tr("Review Training Guide"), tr("REVIEW"), tr("Review the rules, features, and limitations of openpilot"));
    connect(retrainingBtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm(tr("Are you sure you want to review the training guide?"), tr("Review"), this)) {
        emit reviewTrainingGuide();
      }
    });
    addItem(retrainingBtn);
  }

  if (Hardware::TICI()) {
    auto regulatoryBtn = new ButtonControl(tr("Regulatory"), tr("VIEW"), "");
    connect(regulatoryBtn, &ButtonControl::clicked, [=]() {
      const std::string txt = util::read_file("../assets/offroad/fcc.html");
      ConfirmationDialog::rich(QString::fromStdString(txt), this);
    });
    addItem(regulatoryBtn);
  }

  auto translateBtn = new ButtonControl(tr("Change Language"), tr("CHANGE"), "");
  connect(translateBtn, &ButtonControl::clicked, [=]() {
    QMap<QString, QString> langs = getSupportedLanguages();
    QString selection = MultiOptionDialog::getSelection(tr("Select a language"), langs.keys(), langs.key(uiState()->language), this);
    if (!selection.isEmpty()) {
      // put language setting, exit Qt UI, and trigger fast restart
      Params().put("LanguageSetting", langs[selection].toStdString());
      qApp->exit(18);
      watchdog_kick(0);
    }
  });
  addItem(translateBtn);

  /*QObject::connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    for (auto btn : findChildren<ButtonControl *>()) {
      btn->setEnabled(offroad);
    }
  });*/

  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(30);

  QPushButton *reboot_btn = new QPushButton(tr("Reboot"));
  reboot_btn->setObjectName("reboot_btn");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::clicked, this, &DevicePanel::reboot);

  QPushButton *rebuild_btn = new QPushButton(tr("Rebuild"));
  rebuild_btn->setObjectName("rebuild_btn");
  power_layout->addWidget(rebuild_btn);
  QObject::connect(rebuild_btn, &QPushButton::clicked, [=]() {

    if (ConfirmationDialog::confirm(tr("Are you sure you want to rebuild?"), tr("Rebuild"), this)) {
      std::system("cd /data/openpilot && scons -c");
      std::system("rm /data/openpilot/.sconsign.dblite");
      std::system("rm /data/openpilot/prebuilt");
      std::system("rm -rf /tmp/scons_cache");
      if (Hardware::TICI())
        std::system("sudo reboot");
      else
        std::system("reboot");
    }
  });

  QPushButton *poweroff_btn = new QPushButton(tr("Power Off"));
  poweroff_btn->setObjectName("poweroff_btn");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::clicked, this, &DevicePanel::poweroff);

  if (!Hardware::PC()) {
    connect(uiState(), &UIState::offroadTransition, poweroff_btn, &QPushButton::setVisible);
  }

  setStyleSheet(R"(
    #reboot_btn { height: 120px; border-radius: 15px; background-color: #393939; }
    #reboot_btn:pressed { background-color: #4a4a4a; }
    #rebuild_btn { height: 120px; border-radius: 15px; background-color: #393939; }
    #rebuild_btn:pressed { background-color: #4a4a4a; }
    #poweroff_btn { height: 120px; border-radius: 15px; background-color: #E22C2C; }
    #poweroff_btn:pressed { background-color: #FF2424; }
  )");
  addItem(power_layout);
}

void DevicePanel::updateCalibDescription() {
  QString desc =
      tr("openpilot requires the device to be mounted within 4° left or right and "
         "within 5° up or 8° down. openpilot is continuously calibrating, resetting is rarely required.");
  std::string calib_bytes = Params().get("CalibrationParams");
  if (!calib_bytes.empty()) {
    try {
      AlignedBuffer aligned_buf;
      capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
      auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
      if (calib.getCalStatus() != cereal::LiveCalibrationData::Status::UNCALIBRATED) {
        double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
        double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
        desc += tr(" Your device is pointed %1° %2 and %3° %4.")
                    .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? tr("down") : tr("up"),
                         QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? tr("left") : tr("right"));
      }
    } catch (kj::Exception) {
      qInfo() << "invalid CalibrationParams";
    }
  }
  qobject_cast<ButtonControl *>(sender())->setDescription(desc);
}

void DevicePanel::reboot() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to reboot?"), tr("Reboot"), this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        Params().putBool("DoReboot", true);
      }
    }
  } else {
    ConfirmationDialog::alert(tr("Disengage to Reboot"), this);
  }
}

void DevicePanel::poweroff() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to power off?"), tr("Power Off"), this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        Params().putBool("DoShutdown", true);
      }
    }
  } else {
    ConfirmationDialog::alert(tr("Disengage to Power Off"), this);
  }
}

void SettingsWindow::showEvent(QShowEvent *event) {
  setCurrentPanel(0);
}

void SettingsWindow::setCurrentPanel(int index, const QString &param) {
  panel_widget->setCurrentIndex(index);
  nav_btns->buttons()[index]->setChecked(true);
  if (!param.isEmpty()) {
    emit expandToggleDescription(param);
  }
}

SettingsWindow::SettingsWindow(QWidget *parent) : QFrame(parent) {

  // setup two main layouts
  sidebar_widget = new QWidget;
  QVBoxLayout *sidebar_layout = new QVBoxLayout(sidebar_widget);
  sidebar_layout->setMargin(0);
  panel_widget = new QStackedWidget();
  panel_widget->setStyleSheet(R"(
    border-radius: 30px;
    background-color: #292929;
  )");

  // close button
  QPushButton *close_btn = new QPushButton(tr("← Back"));
  close_btn->setStyleSheet(R"(
    QPushButton {
      font-size: 50px;
      font-weight: bold;
      margin: 0px;
      padding: 15px;
      border-width: 0;
      border-radius: 30px;
      color: #dddddd;
      background-color: #444444;
    }
    QPushButton:pressed {
      background-color: #3B3B3B;
    }
  )");
  close_btn->setFixedSize(300, 110);
  sidebar_layout->addSpacing(10);
  sidebar_layout->addWidget(close_btn, 0, Qt::AlignRight);
  sidebar_layout->addSpacing(10);
  QObject::connect(close_btn, &QPushButton::clicked, this, &SettingsWindow::closeSettings);

  // setup panels
  DevicePanel *device = new DevicePanel(this);
  QObject::connect(device, &DevicePanel::reviewTrainingGuide, this, &SettingsWindow::reviewTrainingGuide);
  QObject::connect(device, &DevicePanel::showDriverView, this, &SettingsWindow::showDriverView);
  QObject::connect(device, &DevicePanel::closeSettings, this, &SettingsWindow::closeSettings);

  TogglesPanel *toggles = new TogglesPanel(this);
  QObject::connect(this, &SettingsWindow::expandToggleDescription, toggles, &TogglesPanel::expandToggleDescription);

  QList<QPair<QString, QWidget *>> panels = {
    {tr("Device"), device},
    {tr("Network"), new Networking(this)},
    {tr("Toggles"), toggles},
    {tr("Branch"), new TbranchPanel(this)},
    {tr("Tenesi"), new TenesiPanel(this)},
    {tr("TenesiCar"), new TenesiCarPanel(this)},
    //{tr("Software"), new SoftwarePanel(this)},
    {tr("Community"), new CommunityPanel(this)},
  };

/*#ifdef ENABLE_MAPS
  auto map_panel = new MapPanel(this);
  panels.push_back({tr("Navigation"), map_panel});
  QObject::connect(map_panel, &MapPanel::closeSettings, this, &SettingsWindow::closeSettings);
#endif*/

  nav_btns = new QButtonGroup(this);
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setChecked(nav_btns->buttons().size() == 0);
    btn->setStyleSheet(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 60px;
        font-weight: 500;
      }
      QPushButton:checked {
        color: white;
      }
      QPushButton:pressed {
        color: #ADADAD;
      }
    )");
    btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignRight);

    const int lr_margin = name != tr("Network") ? 50 : 0;  // Network panel handles its own margins
    panel->setContentsMargins(lr_margin, 25, lr_margin, 25);

    ScrollView *panel_frame = new ScrollView(panel, this);
    panel_widget->addWidget(panel_frame);

    QObject::connect(btn, &QPushButton::clicked, [=, w = panel_frame]() {
      btn->setChecked(true);
      panel_widget->setCurrentWidget(w);
    });
  }
  sidebar_layout->setContentsMargins(50, 50, 100, 50);

  // main settings layout, sidebar + main panel
  QHBoxLayout *main_layout = new QHBoxLayout(this);

  sidebar_widget->setFixedWidth(500);
  main_layout->addWidget(sidebar_widget);
  main_layout->addWidget(panel_widget);

  setStyleSheet(R"(
    * {
      color: white;
      font-size: 50px;
    }
    SettingsWindow {
      background-color: black;
    }
  )");
}


static QStringList get_list(const char* path)
{
  QStringList stringList;
  QFile textFile(path);
  if(textFile.open(QIODevice::ReadOnly))
  {
      QTextStream textStream(&textFile);
      while (true)
      {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        else
            stringList.append(line);
      }
  }

  return stringList;
}

CommunityPanel::CommunityPanel(SettingsWindow *parent) : ListWidget(parent) {

  QString selected_car = QString::fromStdString(Params().get("SelectedCar"));

  auto changeCar = new ButtonControl(selected_car.length() ? selected_car : tr("Select your car"),
                    selected_car.length() ? tr("CHANGE") : tr("SELECT"), "");

  connect(changeCar, &ButtonControl::clicked, [=]() {
    QStringList items = get_list("/data/params/d/SupportedCars");
    QString selection = MultiOptionDialog::getSelection(tr("Select a car"), items, selected_car, this);
    if (!selection.isEmpty()) {
      Params().put("SelectedCar", selection.toStdString());
      qApp->exit(18);
      watchdog_kick(0);
    }
  });
  addItem(changeCar);

  // param, title, desc, icon
  std::vector<std::tuple<QString, QString, QString, QString>> toggle_defs{
    {
      "UseLanelines",
      tr("Use lane lines instead of e2e"),
      "",
      "../assets/offroad/icon_openpilot.png",
    },

    {
      "SccOnBus2",
      tr("SCC on BUS 2"),
      tr("If SCC is on bus 2, turn it on."),
      "../assets/offroad/icon_road.png",
    },

    {
      "CruiseStateControl",
      tr("Openpilot controls Cruise State (Experimental)"),
      tr("Openpilot controls cruise on/off, gap and set speed."),
      "../assets/offroad/icon_road.png",
    },
    {
      "IsLdwsCar",
      tr("LDWS only"),
      tr("If your car only supports LDWS, turn it on."),
      "../assets/offroad/icon_warning.png",
    },
    {
      "HapticFeedbackWhenSpeedCamera",
      tr("Haptic feedback (speed-cam alert)"),
      tr("Haptic feedback when a speed camera is detected"),
      "../assets/offroad/icon_openpilot.png",
    },
    {
      "ShowDebugMessage",
      tr("Show Debug Message"),
      "",
      "../assets/offroad/icon_shell.png",
    },
    {
     "TenesiSSHLegacy",
     tr("SSH 공용키 사용"),
     tr("SSH 공용키 사용.."),
     "../assets/offroad/icon_shell.png",
     },
  };

  for (auto &[param, title, desc, icon] : toggle_defs) {
    auto toggle = new ParamControl(param, title, desc, icon, this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
  }

  /*connect(toggles["SccOnBus2"], &ToggleControl::toggleFlipped, [=]() {
    updateToggles();
  });*/
}

void CommunityPanel::showEvent(QShowEvent *event) {
  updateToggles();
}

void CommunityPanel::updateToggles() {
  //auto op_control = toggles["CruiseStateControl"];
  //op_control->setEnabled(params.getBool("SccOnBus2"));
  //op_control->refresh();
}

SelectCar::SelectCar(QWidget* parent): QWidget(parent) {

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(20);
  main_layout->setSpacing(20);

  // Back button
  QPushButton* back = new QPushButton(tr("Back"));
  back->setObjectName("back_btn");
  back->setFixedSize(500, 100);
  connect(back, &QPushButton::clicked, [=]() { emit backPress(); });
  main_layout->addWidget(back, 0, Qt::AlignLeft);

  QListWidget* list = new QListWidget(this);
  list->setStyleSheet("QListView {padding: 40px; background-color: #393939; border-radius: 15px; height: 140px;} QListView::item{height: 100px}");
  //list->setAttribute(Qt::WA_AcceptTouchEvents, true);
  QScroller::grabGesture(list->viewport(), QScroller::LeftMouseButtonGesture);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  list->addItem(tr("[ Not selected ]"));

  QStringList items = get_list("/data/params/d/SupportedCars");
  list->addItems(items);
  list->setCurrentRow(0);

  QString selected = QString::fromStdString(Params().get("SelectedCar"));

  int index = 0;
  for(QString item : items) {
    if(selected == item) {
        list->setCurrentRow(index + 1);
        break;
    }
    index++;
  }

  QObject::connect(list, QOverload<QListWidgetItem*>::of(&QListWidget::itemClicked),
    [=](QListWidgetItem* item){

    if(list->currentRow() == 0)
        Params().remove("SelectedCar");
    else
        Params().put("SelectedCar", list->currentItem()->text().toStdString());

    emit selectedCar();
    });

  main_layout->addWidget(list);
}

//테네시 브랜치 메뉴 추가해보기
TbranchPanel::TbranchPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  // 테네시 깃풀설정등추가

  layout->addWidget(new GitHash()); // 깃 업데이트 시작
  const char* gitpull = "/data/openpilot/selfdrive/assets/sshkey/gitpull.sh ''";
  auto gitpullBtn = new ButtonControl(tr("깃 풀 업데이트"), tr("실 행"));
  layout->addWidget(gitpullBtn);
  QObject::connect(gitpullBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("브랜치의 업데이트를 다운로드하고 적용합니까? \n펭펭~ 재부팅은 수동으로...펭펭~"), tr("실 행?"), this)) {
      std::system(gitpull);
      std::system("rm /data/openpilot/prebuilt");
    }
  });

  layout->addWidget(horizontal_line());
  const char* git_reset = "/data/openpilot/selfdrive/assets/sshkey/git_reset.sh ''";
  auto gitrestBtn = new ButtonControl(tr("깃 리셋"), tr("실 행"));
  layout->addWidget(gitrestBtn);
  QObject::connect(gitrestBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("C3 에서 수정된것을 초기화 시켜요!\n 재부팅이 안되요~ 펭펭~펭펭~"), tr("실 행?"), this)) {
      std::system(git_reset);
      std::system("rm /data/openpilot/prebuilt");
    }
  });

  layout->addWidget(horizontal_line());
  const char* gitpull_cancel = "/data/openpilot/selfdrive/assets/sshkey/gitpull_cancel.sh ''";
  auto gitpull_cancelBtn = new ButtonControl(tr("깃풀 이전 단계로"), tr("실 행"));
  layout->addWidget(gitpull_cancelBtn);
  QObject::connect(gitpull_cancelBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("깃풀을 취소합니다! 펭펭~ \n 재부팅 수동으로...펭펭~"), tr("실 행?"), this)) {
      std::system(gitpull_cancel);
      std::system("rm /data/openpilot/prebuilt");
    }
  });
  layout->addWidget(horizontal_line());

  auto TenesiSSHLegacy_Btn = new ButtonControl(tr("공용 SSH키를 사용"), tr("실 행"));
  layout->addWidget(TenesiSSHLegacy_Btn);
  QObject::connect(TenesiSSHLegacy_Btn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("공용 SSH키를 사용하게 설정합니다.! 펭펭~ \n 재부팅 수동으로...펭펭~"), tr("실 행?"), this)) {
      std::system("cp -f /data/openpilot/selfdrive/assets/sshkey/GithubSshKeys_legacy /data/params/d/GithubSshKeys");
      std::system("chmod 600 /data/params/d/GithubSshKeys");
    }
  });
  layout->addWidget(new ParamControl("TenesiSSHLegacy", "SSH 공용키 사용", "SSH 공용키 사용..", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(horizontal_line());
 // C3 Panda DFU 모드 진입 후 리커버리 명령어 실행하기
  auto C3DFU_Btn = new ButtonControl(tr("C3 판다 DFU 플래싱!"), tr("실 행"));
  layout->addWidget(C3DFU_Btn);
  const char* c3dfurun = "/data/openpilot/selfdrive/assets/dtc/c3dfu.sh ''";
  QObject::connect(C3DFU_Btn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm(tr("판다가 복구가 안될때 DFU 모드 진입후 실합니다."), tr("실 행?"), this)) {
      std::system(c3dfurun);
    }
  });
 // 깃 업데이트 관련끝

} // 테네시 브랜치 관리메뉴

//테네시 메뉴 추가해보기
TenesiPanel::TenesiPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("TenesiCamera", "NDA 상시 카메라경고", "NDA 카메라 경고를 계기판과 HUD에 끄거나 켭니다.", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new CValueControl("TenesiCameraSelect", "NDA 상시 카메라경고 HUD 표시", "HUD나 계기판에 경고표시 모드 선택 1 2 3", "../assets/offroad/icon_road.png", 0, 15, 1));
  layout->addWidget(new CValueControl("TenesiBrakeSelect", "HUD에 브레이킹 작동표시", "HUD에 브레이킹 작동표시 모드 선택 1 2 3", "../assets/offroad/icon_road.png", 0, 15, 1));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("Steer_409", "조향토크 409설정", "조향토크를 409로 무조건 설정 GenesisDH 테스트용", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("Steer_SRTune", "조향비율 가변", "SR값을 속도대비 가변으로 사용하기", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new CValueControl("Steer_SRTune_v", "가변SR 비율", "가변SR 사용시 비율값.", "../assets/offroad/icon_road.png", 80, 120, 1));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("Zorrobyte", "차로인식 Zorrobyte", "차로인식 Zorrobyte 적용선택", "../assets/offroad/icon_road.png", this));
  layout->addWidget(new ParamControl("Neokiibyte", "차로인식 NeokiiByte", "차로인식 NeokiiByte 적용여부 선택", "../assets/offroad/icon_road.png", this));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("ShowErpmUI", "엔진RPM 표시", "현재엔진RPM 표시하기..", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("ShowCgearUI", "주행기어단수 보기","기어레버 위치와 기어단수를 볼수 있습니다..","../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("ShowBsdUI","측후방정보 보기", "측후방 감지기 이미지를 볼수 있습니다..", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("Sound_Start","오프시작시 음성지원 선택","오프시작시 음성 온 오프", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("Sound_Slow", "TMAP 자동감속 음성지원 선택", "네비 감속시 음성 온 오프", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("SoundAutoHold","오토홀드음성지원","오토홀드음성을 끄거나 켭니다..", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("SoundBsd","측후방 음성지원","측후방 센서 작동시 음성을 끄거나 켭니다.","../assets/offroad/icon_shell.png", this));
  layout->addWidget(new ParamControl("Sound_Mdpserr","MDPS 작동불능 음성","DH MDPS 작동 불가시 음성을 끄거나 켭니다.", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(horizontal_line());
  layout->addWidget(new ParamControl("AutoSetOpt", "크루즈 오토 셋", "파파의 오토크루즈 셋 적용", "../assets/offroad/icon_road.png", this));
  layout->addWidget(new ParamControl("T_Debug","Tmux a 저장옵션","Tmux a 내용을 화일로 저장합니다..", "../assets/offroad/icon_shell.png", this));
  layout->addWidget(horizontal_line());

  auto recorddelbtn = new ButtonControl(tr("녹화파일 전부 삭제"), tr("실행"));
    QObject::connect(recorddelbtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm(tr("저장된 녹화파일을 모두 삭제합니다. 진행하시겠습니까?"), tr("실 행?"), this)){
        std::system("rm -f /data/media/0/videos/*");
      }
    });
    layout->addWidget(recorddelbtn);

  auto realdatadelbtn = new ButtonControl(tr("주행로그-영상 전부 삭제"), tr("실행"));
    QObject::connect(realdatadelbtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm(tr("저장된 주행로그 영상을 모두 삭제합니다. 진행하시겠습니까?"), tr("실 행?"), this)){
        std::system("rm -rf /data/media/0/realdata/*");
      }
    });
    layout->addWidget(realdatadelbtn);

} // 테네시 메뉴 추가

TenesiCarPanel::TenesiCarPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  layout->addWidget(horizontal_line());
  layout->addWidget(new CValueControl("Boost_s", "스타트 악셀 부스트값 *0.01", "SCC 악셀 부스트값.", "../assets/offroad/icon_road.png", 0, 20, 1));
  layout->addWidget(new CValueControl("Creep_Speed_Start", "스타트 악셀 속도한계 km", "SCC 악셀 부스트값 적용 속도. 단위 km", "../assets/offroad/icon_road.png", 0, 30, 1));
  layout->addWidget(new CValueControl("Boost_v", "가속 악셀 부스트값 *0.01", "SCC 악셀 가속시 부스트값.", "../assets/offroad/icon_road.png", 0, 50, 1));
  layout->addWidget(new CValueControl("Creep_Speed_Scc", "가속 악셀 부스트값 속도범위 km", "SCC 악셀 가속시 부스트값 적용 속도 범위 지정. 단위 km", "../assets/offroad/icon_road.png", 0, 100, 1));
  layout->addWidget(new CValueControl("TenesiSccStopCost", "SCC 정차시도 브레이크 감도조절", "SCC 정차시도 브레이크 감도조절 단위 * 0.01", "../assets/offroad/icon_road.png", 0, 30, 1));
  layout->addWidget(new CValueControl("Tenesi_Gas_Factor", "SCC 가속감 수정", "SCC가속값을 수정한다...", "../assets/offroad/icon_road.png", -10, 50, 1));
  layout->addWidget(new CValueControl("Lane_Change", "차로변경 속도", "자동 차로 변경 적용 속도 지정.", "../assets/offroad/icon_road.png", 0, 200, 1));
  layout->addWidget(new CValueControl("Auto_engage", "자동인게이지 속도", "자동 인게이지 적용 속도 지정.", "../assets/offroad/icon_road.png", 0, 30, 1));

  //addItem(new ParamControl("AutoResumeFromGas", "엑셀크루즈ON", "엑셀을 60%이상 밟으면 크루즈를 켭니다.", "../assets/offroad/icon_road.png", this));
} // 테네시 SCC등 미세 설정값 수정

// // ajouato 코드추가 여기부터
CValueControl::CValueControl(const QString& params, const QString& title, const QString& desc, const QString& icon, int min, int max, int unit/*=1*/) : AbstractControl(title, desc, icon)
{

    m_params = params;
    m_min = min;
    m_max = max;
    m_unit = unit;

    label.setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    label.setStyleSheet("color: #e0e879");
    hlayout->addWidget(&label);

    btnminus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
    btnplus.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
    btnminus.setFixedSize(150, 100);
    btnplus.setFixedSize(150, 100);
    hlayout->addWidget(&btnminus);
    hlayout->addWidget(&btnplus);

    QObject::connect(&btnminus, &QPushButton::released, [=]() {
        auto str = QString::fromStdString(Params().get(m_params.toStdString()));
        int value = str.toInt();
        value = value - m_unit;
        if (value < m_min) {
            value = m_min;
        }
        else {
        }

        //UIScene& scene = uiState()->scene;//QUIState::ui_state.scene;
        //scene.scr.autoFocus = value;
        QString values = QString::number(value);
        Params().put(m_params.toStdString(), values.toStdString());
        refresh();
    });

    QObject::connect(&btnplus, &QPushButton::released, [=]() {
        auto str = QString::fromStdString(Params().get(m_params.toStdString()));
        int value = str.toInt();
        value = value + m_unit;
        if (value > m_max) {
            value = m_max;
        }
        else {
        }

        //UIScene& scene = uiState()->scene;//QUIState::ui_state.scene;
        //scene.scr.autoFocus = value;
        QString values = QString::number(value);
        Params().put(m_params.toStdString(), values.toStdString());
        refresh();
    });
    refresh();
}

void CValueControl::refresh()
{
    label.setText(QString::fromStdString(Params().get(m_params.toStdString())));
    btnminus.setText("－");
    btnplus.setText("＋");
} // ajouato 코드추가 여기까지
