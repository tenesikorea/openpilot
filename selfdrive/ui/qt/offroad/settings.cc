#include "selfdrive/ui/qt/offroad/settings.h"

#include <cassert>
#include <cmath>
#include <string>

#include <QDebug>

#ifndef QCOM
#include "selfdrive/ui/qt/offroad/networking.h"
#endif

#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_settings.h"
#endif

#include "selfdrive/common/params.h"
#include "selfdrive/common/util.h"
#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"
#include "selfdrive/ui/qt/widgets/toggle.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"
#include "selfdrive/ui/qt/qt_window.h"

#include <QComboBox>
#include <QAbstractItemView>
#include <QScroller>
#include <QListView>
#include <QListWidget>

TogglesPanel::TogglesPanel(SettingsWindow *parent) : ListWidget(parent) {
  // param, title, desc, icon
  std::vector<std::tuple<QString, QString, QString, QString>> toggles{
    {
      "OpenpilotEnabledToggle",
      "오픈파일럿 사용",
      "어댑티브 크루즈 컨트롤 및 차선 유지 지원을 위해 오픈파일럿 시스템을 사용하십시오. 이 기능을 사용하려면 항상 주의를 기울여야 합니다. 이 설정을 변경하는 것은 자동차의 전원이 꺼졌을 때 적용됩니다.",
      "../assets/offroad/icon_openpilot.png",
    },
    {
      "IsLdwEnabled",
      "차선이탈 경보 사용",
      "50km/h이상의 속도로 주행하는 동안 방향 지시등이 활성화되지 않은 상태에서 차량이 감지된 차선 위를 넘어갈 경우 원래 차선으로 다시 방향을 전환하도록 경고를 보냅니다.",
      "../assets/offroad/icon_warning.png",
    },
    {
      "IsRHD",
      "우핸들 운전방식 사용",
      "오픈파일럿이 좌측 교통 규칙을 준수하도록 허용하고 우측 운전석에서 운전자 모니터링을 수행하십시오.",
      "../assets/offroad/icon_openpilot_mirrored.png",
    },
    {
      "IsMetric",
      "미터법 사용",
      "mi/h 대신 km/h 단위로 속도를 표시합니다.",
      "../assets/offroad/icon_metric.png",
    },
    {
      "RecordFront",
      "운전자 영상 녹화-블박처럼",
      "운전자 모니터링 카메라에서 데이터를 업로드하고 운전자 모니터링 알고리즘을 개선하십시오.",
      "../assets/offroad/icon_monitoring.png",
    },
    {
      "EndToEndToggle",
      "\U0001f96c 차선이 없을 때 사용 버전 알파 \U0001f96c",
      "차선이 없는 곳에서 사람과 같이 운전을 하는 것을 목표함니다.",
      "../assets/offroad/icon_road.png",
    },
    {
      "DisengageOnAccelerator",
      "가속폐달 작동시 오파제어 해제",
      "When enabled, pressing the accelerator pedal will disengage openpilot.",
      "../assets/offroad/icon_disengage_on_accelerator.svg",
    },
#ifdef ENABLE_MAPS
    {
      "NavSettingTime24h",
      "24시간제로 시간 표시",
      "Use 24h format instead of am/pm",
      "../assets/offroad/icon_metric.png",
    },
#endif

  };

  Params params;

  if (params.getBool("DisableRadar_Allow")) {
    toggles.push_back({
      "DisableRadar",
      "openpilot Longitudinal Control",
      "openpilot will disable the car's radar and will take over control of gas and brakes. Warning: this disables AEB!",
      "../assets/offroad/icon_speed_limit.png",
    });
  }

  for (auto &[param, title, desc, icon] : toggles) {
    auto toggle = new ParamControl(param, title, desc, icon, this);
    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);
    //if (!locked) {
    //  connect(uiState(), &UIState::offroadTransition, toggle, &ParamControl::setEnabled);
    //}
    addItem(toggle);
  }
}

DevicePanel::DevicePanel(SettingsWindow *parent) : ListWidget(parent) {
  setSpacing(50);
  addItem(new LabelControl("동글 아이디", getDongleId().value_or("N/A")));
  addItem(new LabelControl("동글 시리얼", params.get("기계 시리얼").c_str()));

  QHBoxLayout *reset_layout = new QHBoxLayout();
  reset_layout->setSpacing(30);

  QPushButton *restart_openpilot_btn = new QPushButton("소프트 재부팅");
  restart_openpilot_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #7300a4;");
  reset_layout->addWidget(restart_openpilot_btn);
  QObject::connect(restart_openpilot_btn, &QPushButton::released, [=]() {
    emit closeSettings();
    QTimer::singleShot(1000, []() {
      Params().putBool("SoftRestartTriggered", true);
    });
  });

  // 고장코드 삭제 메뉴 dtc_btn MDPS DTC
  QPushButton *dtc_btn = new QPushButton("MDPS DTC");
  dtc_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #E22C2C;");
  reset_layout->addWidget(dtc_btn);
  const char* dtcrun = "/data/openpilot/selfdrive/assets/dtc/dtc.sh ''";
  QObject::connect(dtc_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm(tr("제네시스DH MDPS 고장코드 삭제! \n 약 10초후 무조건 재부팅합니다!! "), this)) {
      std::system(dtcrun);
      std::system("touch /data/openpilot/prebuilt");
      if (Hardware::TICI())
        std::system("sudo reboot");
      else
        std::system("reboot");

    }
  });

  // reset calibration button
  QPushButton *reset_calib_btn = new QPushButton("학습값초기화");
  reset_calib_btn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #6B7EFF;");
  reset_layout->addWidget(reset_calib_btn);
  QObject::connect(reset_calib_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("명령 승인하면 리셋되고 재부팅합니다. \n 해유?", this)) {
      Params().remove("CalibrationParams");
      Params().remove("LiveParameters");
      emit closeSettings();
      QTimer::singleShot(1000, []() {
        Params().putBool("SoftRestartTriggered", true);
      });
    }
  });

  addItem(reset_layout);

  // offroad-only buttons
  addItem(new OpenpilotView());
  auto dcamBtn = new ButtonControl("운전자 감시", "미리보기",
                                   "이쁜 아가씨가 도로에 있어도 한눈 팔지 말기!");
  connect(dcamBtn, &ButtonControl::clicked, [=]() { emit showDriverView(); });
  addItem(dcamBtn);

  auto resetCalibBtn = new ButtonControl("캘리브레이션 리셋", "리 셋", " ");
  connect(resetCalibBtn, &ButtonControl::showDescription, this, &DevicePanel::updateCalibDescription);
  connect(resetCalibBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm("AO상태 즉 핸들 캘리상태를 리셋하겠나유?", this)) {
      params.remove("CalibrationParams");
    }
  });
  addItem(resetCalibBtn);

  if (!params.getBool("Passive")) {
    auto retrainingBtn = new ButtonControl("사용법 다시 보기", "보기", "다양한 기본 조작법을 배웁니다..");
    connect(retrainingBtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm("펭수를 보아라~~~~?", this)) {
        emit reviewTrainingGuide();
      }
    });
    addItem(retrainingBtn);
  }

  if (Hardware::TICI()) {
    auto regulatoryBtn = new ButtonControl("주의 사항", " 보기 ", "");
    connect(regulatoryBtn, &ButtonControl::clicked, [=]() {
      const std::string txt = util::read_file("../assets/offroad/fcc.html");
      RichTextDialog::alert(QString::fromStdString(txt), this);
    });
    addItem(regulatoryBtn);
  }

  /*QObject::connect(uiState(), &UIState::offroadTransition, [=](bool offroad) {
    for (auto btn : findChildren<ButtonControl *>()) {
      btn->setEnabled(offroad);
    }
  });*/

  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(30);

  QPushButton *reboot_btn = new QPushButton("Reboot");
  reboot_btn->setObjectName("reboot_btn");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::clicked, this, &DevicePanel::reboot);

  QPushButton *rebuild_btn = new QPushButton("Rebuild");
  rebuild_btn->setObjectName("rebuild_btn");
  power_layout->addWidget(rebuild_btn);
  QObject::connect(rebuild_btn, &QPushButton::clicked, [=]() {

    if (ConfirmationDialog::confirm("Are you sure you want to rebuild?", this)) {
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

  QPushButton *poweroff_btn = new QPushButton("Power Off");
  poweroff_btn->setObjectName("poweroff_btn");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::clicked, this, &DevicePanel::poweroff);

  if (Hardware::TICI()) {
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
      "오픈파일럿은 좌우로 4° 위아래로 5° 를 보정합니다."
      " 그 이상의 경우 보정이 필요합니다. 마운트고체나 재학습 하세요!";
  std::string calib_bytes = Params().get("CalibrationParams");
  if (!calib_bytes.empty()) {
    try {
      AlignedBuffer aligned_buf;
      capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
      auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
      if (calib.getCalStatus() != 0) {
        double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
        double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
        desc += QString(" 이온장착상태는 %1° %2 and %3° %4.")
                    .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? "아래로" : "위로",
                         QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? "좌측으로" : "우측으로");
      }
    } catch (kj::Exception) {
      qInfo() << "invalid CalibrationParams";
    }
  }
  qobject_cast<ButtonControl *>(sender())->setDescription(desc);
}

void DevicePanel::reboot() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm("장치를 정말로 재부팅 할겨?", this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        Params().putBool("DoReboot", true);
      }
    }
  } else {
    ConfirmationDialog::alert("인게이지 해제후에 시도하세요!", this);
  }
}

void DevicePanel::poweroff() {
  if (!uiState()->engaged()) {
    if (ConfirmationDialog::confirm("장치를 정말로 끌려구? 할겨??", this)) {
      // Check engaged again in case it changed while the dialog was open
      if (!uiState()->engaged()) {
        Params().putBool("DoShutdown", true);
      }
    }
  } else {
    ConfirmationDialog::alert("인게이지 해제후에 시도하세요!", this);
  }
}

SoftwarePanel::SoftwarePanel(QWidget* parent) : ListWidget(parent) {
  gitBranchLbl = new LabelControl("깃 브랜치");
  gitCommitLbl = new LabelControl("깃 커 밋");
  osVersionLbl = new LabelControl("오파 운용버젼");
  versionLbl = new LabelControl("버젼", "", QString::fromStdString(params.get("ReleaseNotes")).trimmed());
  lastUpdateLbl = new LabelControl("마지막업데이트체크", "", "나는 지난밤에 니가 한짓을 알고 있찌롱! 펭수~");
  updateBtn = new ButtonControl("업데이트를 확인", "");
  connect(updateBtn, &ButtonControl::clicked, [=]() {
    if (params.getBool("IsOffroad")) {
      fs_watch->addPath(QString::fromStdString(params.getParamPath("LastUpdateTime")));
      fs_watch->addPath(QString::fromStdString(params.getParamPath("UpdateFailedCount")));
      updateBtn->setText("펭수를 보아라~");
      updateBtn->setEnabled(false);
    }
    std::system("pkill -1 -f selfdrive.updated");
  });


  auto uninstallBtn = new ButtonControl("오픈파일럿 삭제 " + getBrand(), "UNINSTALL");
  connect(uninstallBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm("정말로 오픈파일럿을 삭제할겨? 펭펭??", this)) {
      params.putBool("DoUninstall", true);
    }
  });
  connect(uiState(), &UIState::offroadTransition, uninstallBtn, &QPushButton::setEnabled);

  QWidget *widgets[] = {versionLbl, lastUpdateLbl, updateBtn, gitBranchLbl, gitCommitLbl, osVersionLbl, uninstallBtn};
  for (QWidget* w : widgets) {
    addItem(w);
  }

  fs_watch = new QFileSystemWatcher(this);
  QObject::connect(fs_watch, &QFileSystemWatcher::fileChanged, [=](const QString path) {
    if (path.contains("UpdateFailedCount") && std::atoi(params.get("UpdateFailedCount").c_str()) > 0) {
      lastUpdateLbl->setText("브랜치 업데이트 실패");
      updateBtn->setText("확인중 펭수를 보아라~");
      updateBtn->setEnabled(true);
    } else if (path.contains("LastUpdateTime")) {
      updateLabels();
    }
  });
}

void SoftwarePanel::showEvent(QShowEvent *event) {
  updateLabels();
}

void SoftwarePanel::updateLabels() {
  QString lastUpdate = "";
  auto tm = params.get("LastUpdateTime");
  if (!tm.empty()) {
    lastUpdate = timeAgo(QDateTime::fromString(QString::fromStdString(tm + "Z"), Qt::ISODate));
  }

  versionLbl->setText(getBrandVersion());
  lastUpdateLbl->setText(lastUpdate);
  updateBtn->setText("확인중");
  updateBtn->setEnabled(true);
  gitBranchLbl->setText(QString::fromStdString(params.get("GitBranch")));
  gitCommitLbl->setText(QString::fromStdString(params.get("GitCommit")).left(24));
  osVersionLbl->setText(QString::fromStdString(Hardware::get_os_version()).trimmed());
}

C2NetworkPanel::C2NetworkPanel(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(50, 0, 50, 0);

  ListWidget *list = new ListWidget();
  list->setSpacing(30);
  // wifi + tethering buttons
#ifdef QCOM
  auto wifiBtn = new ButtonControl("네트워크 설정", " 열기");
  QObject::connect(wifiBtn, &ButtonControl::clicked, [=]() { HardwareEon::launch_wifi(); });
  list->addItem(wifiBtn);

  auto tetheringBtn = new ButtonControl("Tethering Settings", "OPEN");
  QObject::connect(tetheringBtn, &ButtonControl::clicked, [=]() { HardwareEon::launch_tethering(); });
  list->addItem(tetheringBtn);
#endif
  ipaddress = new LabelControl("IP Address", "");
  list->addItem(ipaddress);

  // SSH key management
  list->addItem(new SshToggle());
  list->addItem(new SshControl());
  layout->addWidget(list);
  layout->addStretch(1);
}

void C2NetworkPanel::showEvent(QShowEvent *event) {
  ipaddress->setText(getIPAddress());
}

QString C2NetworkPanel::getIPAddress() {
  std::string result = util::check_output("ifconfig wlan0");
  if (result.empty()) return "";

  const std::string inetaddrr = "inet addr:";
  std::string::size_type begin = result.find(inetaddrr);
  if (begin == std::string::npos) return "";

  begin += inetaddrr.length();
  std::string::size_type end = result.find(' ', begin);
  if (end == std::string::npos) return "";

  return result.substr(begin, end - begin).c_str();
}

QWidget *network_panel(QWidget *parent) {
#ifdef QCOM
  return new C2NetworkPanel(parent);
#else
  return new Networking(parent);
#endif
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
} // 교주사마 코드

void SettingsWindow::showEvent(QShowEvent *event) {
  panel_widget->setCurrentIndex(0);
  nav_btns->buttons()[0]->setChecked(true);
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
  QPushButton *close_btn = new QPushButton("← 닫기");
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

  QList<QPair<QString, QWidget *>> panels = {
    {"장 치", device},
    {"WIFI-연결", network_panel(this)},
    {"토글메뉴", new TogglesPanel(this)},
    {"브랜치관리", new TbranchPanel(this)},
    {"테네시설정", new TenesiPanel(this)},
    {"소프트개발자", new SoftwarePanel(this)},
    {"현기전용셋팅", new CommunityPanel(this)},
  };

/*#ifdef ENABLE_MAPS
  auto map_panel = new MapPanel(this);
  panels.push_back({tr("Navigation"), map_panel});
  QObject::connect(map_panel, &MapPanel::closeSettings, this, &SettingsWindow::closeSettings);
#endif*/

  const int padding = panels.size() > 3 ? 25 : 35;

  nav_btns = new QButtonGroup(this);
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setChecked(nav_btns->buttons().size() == 0);
    btn->setStyleSheet(QString(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 60px;
        font-weight: 500;
        padding-top: %1px;
        padding-bottom: %1px;
      }
      QPushButton:checked {
        color: white;
      }
      QPushButton:pressed {
        color: #ADADAD;
      }
    )").arg(padding));

    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignRight);

    const int lr_margin = name != "Network" ? 50 : 0;  // Network panel handles its own margins
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

void SettingsWindow::hideEvent(QHideEvent *event) {
#ifdef QCOM
  HardwareEon::close_activities();
#endif
}

/////////////////////////////////////////////////////////////////////////
//opkr

OpenpilotView::OpenpilotView() : AbstractControl("Driving Camera", "Preview the open pilot driving screen.", "") {

  // setup widget
  hlayout->addStretch(1);

  btn.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");

  btn.setFixedSize(250, 100);
  hlayout->addWidget(&btn);

  QObject::connect(&btn, &QPushButton::clicked, [=]() {
    bool stat = params.getBool("IsOpenpilotViewEnabled");
    if (stat) {
      params.putBool("IsOpenpilotViewEnabled", false);
    } else {
      params.putBool("IsOpenpilotViewEnabled", true);
    }
    refresh();
  });
  refresh();
}

void OpenpilotView::refresh() {
  bool param = params.getBool("IsOpenpilotViewEnabled");
  QString car_param = QString::fromStdString(params.get("CarParams"));
  if (param) {
    btn.setText("UNVIEW");
  } else {
    btn.setText("PREVIEW");
  }
  if (car_param.length()) {
    btn.setEnabled(false);
  } else {
    btn.setEnabled(true);
  }
}



ChargingMin::ChargingMin() : AbstractControl("BAT MinCharging Value", "Sets the minimum battery charge value.", "../assets/offroad/icon_shell.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
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
  btnminus.setText("－");
  btnplus.setText("＋");
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("OpkrBatteryChargingMin"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 10) {
      value = 10;
    }
    QString values = QString::number(value);
    params.put("OpkrBatteryChargingMin", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("OpkrBatteryChargingMin"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 90) {
      value = 90;
    }
    QString values = QString::number(value);
    params.put("OpkrBatteryChargingMin", values.toStdString());
    refresh();
  });
  refresh();
}

void ChargingMin::refresh() {
  label.setText(QString::fromStdString(params.get("OpkrBatteryChargingMin")));
}

ChargingMax::ChargingMax() : AbstractControl("BAT MaxCharging Value", "Sets the maximum battery charge value.", "../assets/offroad/icon_shell.png") {

  label.setAlignment(Qt::AlignVCenter|Qt::AlignRight);
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
  btnminus.setText("－");
  btnplus.setText("＋");
  hlayout->addWidget(&btnminus);
  hlayout->addWidget(&btnplus);

  QObject::connect(&btnminus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("OpkrBatteryChargingMax"));
    int value = str.toInt();
    value = value - 1;
    if (value <= 10) {
      value = 10;
    }
    QString values = QString::number(value);
    params.put("OpkrBatteryChargingMax", values.toStdString());
    refresh();
  });
  
  QObject::connect(&btnplus, &QPushButton::clicked, [=]() {
    auto str = QString::fromStdString(params.get("OpkrBatteryChargingMax"));
    int value = str.toInt();
    value = value + 1;
    if (value >= 90) {
      value = 90;
    }
    QString values = QString::number(value);
    params.put("OpkrBatteryChargingMax", values.toStdString());
    refresh();
  });
  refresh();
}

void ChargingMax::refresh() {
  label.setText(QString::fromStdString(params.get("OpkrBatteryChargingMax")));
}


/////////////////////////////////////////////////////////////////////////

CommunityPanel::CommunityPanel(QWidget* parent) : QWidget(parent) {

  main_layout = new QStackedLayout(this);

  homeScreen = new QWidget(this);
  QVBoxLayout* vlayout = new QVBoxLayout(homeScreen);
  vlayout->setContentsMargins(0, 20, 0, 20);

  QString selected = QString::fromStdString(Params().get("SelectedCar"));

  QPushButton* selectCarBtn = new QPushButton(selected.length() ? selected : "강제인식할 차량 선택 ");
  selectCarBtn->setObjectName("selectCarBtn");
  //selectCarBtn->setStyleSheet("margin-right: 30px;");
  //selectCarBtn->setFixedSize(350, 100);
  connect(selectCarBtn, &QPushButton::clicked, [=]() { main_layout->setCurrentWidget(selectCar); });

  homeWidget = new QWidget(this);
  QVBoxLayout* toggleLayout = new QVBoxLayout(homeWidget);
  homeWidget->setObjectName("homeWidget");

  ScrollView *scroller = new ScrollView(homeWidget, this);
  scroller->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  main_layout->addWidget(homeScreen);

  selectCar = new SelectCar(this);
  connect(selectCar, &SelectCar::backPress, [=]() { main_layout->setCurrentWidget(homeScreen); });
  connect(selectCar, &SelectCar::selectedCar, [=]() {

     QString selected = QString::fromStdString(Params().get("SelectedCar"));
     selectCarBtn->setText(selected.length() ? selected : "강제인식할 차량 선택 ");
     main_layout->setCurrentWidget(homeScreen);
  });
  main_layout->addWidget(selectCar);


  QString lateral_control = QString::fromStdString(Params().get("LateralControl"));
  if(lateral_control.length() == 0)
    lateral_control = "TORQUE";

  QPushButton* lateralControlBtn = new QPushButton(lateral_control);
  lateralControlBtn->setObjectName("lateralControlBtn");
  //lateralControlBtn->setStyleSheet("margin-right: 30px;");
  //lateralControlBtn->setFixedSize(350, 100);
  connect(lateralControlBtn, &QPushButton::clicked, [=]() { main_layout->setCurrentWidget(lateralControl); });


  lateralControl = new LateralControl(this);
  connect(lateralControl, &LateralControl::backPress, [=]() { main_layout->setCurrentWidget(homeScreen); });
  connect(lateralControl, &LateralControl::selected, [=]() {

     QString lateral_control = QString::fromStdString(Params().get("LateralControl"));
     if(lateral_control.length() == 0)
       lateral_control = "TORQUE";
     lateralControlBtn->setText(lateral_control);
     main_layout->setCurrentWidget(homeScreen);
  });
  main_layout->addWidget(lateralControl);

  QHBoxLayout* layoutBtn = new QHBoxLayout(homeWidget);

  layoutBtn->addWidget(lateralControlBtn);
  layoutBtn->addSpacing(10);
  layoutBtn->addWidget(selectCarBtn);

  vlayout->addSpacing(10);
  vlayout->addLayout(layoutBtn, 0);
  vlayout->addSpacing(10);
  vlayout->addWidget(scroller, 1);

  QPalette pal = palette();
  pal.setColor(QPalette::Background, QColor(0x29, 0x29, 0x29));
  setAutoFillBackground(true);
  setPalette(pal);

  setStyleSheet(R"(
    #back_btn, #selectCarBtn, #lateralControlBtn {
      font-size: 50px;
      margin: 0px;
      padding: 20px;
      border-width: 0;
      border-radius: 30px;
      color: #dddddd;
      background-color: #444444;
    }
  )");

  QList<ParamControl*> toggles;

  toggles.append(new ParamControl("UseClusterSpeed",
                                            "계기판 속도계 속도를 사용",
                                            "휠스피드 센서 속도를 사용시 오프.",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("LongControlEnabled",
                                            "롱컨트롤 사용",
                                            "N 롱컨트롤 기능 사용. 오픈파일럿이 속도를 조절합니다. 주의 하시길 바랍니다.",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("MadModeEnabled",
                                            "매드모드 사용",
                                            "HKG 매드모드 사용. 가감속의 사용 하지 않아도 핸들 조향을 사용합니다.",
                                            "../assets/offroad/icon_openpilot.png",
                                            this));

  toggles.append(new ParamControl("IsLdwsCar",
                                            "LDWS 옵션해야 되는차",
                                            "LDWS 옵션을 작동시켜야 작동되는 K7 차량같은차",
                                            "../assets/offroad/icon_openpilot.png",
                                            this));

  toggles.append(new ParamControl("LaneChangeEnabled",
                                            "차로 변경 옵션 사용",
                                            "차로 변경 자동 옵션 사용.",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("AutoLaneChangeEnabled",
                                            "차로변경의 수동/자동 옵션",
                                            "자동 차로 변경. 사용에 주의 하십시오",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("SccSmootherSlowOnCurves",
                                            "커브 감속 사용",
                                            "SCC 설정 시 곡률에 따른 속도 감속 기능을 사용",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("SccSmootherSyncGasPressed",
                                            "크루즈 속도의 동기화",
                                            "크루즈 속도를 설정 후 엑셀로 인해 설정 속도보다 가속 속도가 높아지면 그 속도에 크루즈 설정 속도가 동기화 됩니다.",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("StockNaviDecelEnabled",
                                            "차량네비 감속 기능 사용",
                                            "풀컨트롤 개조 차량에서는 옵션을 선택적 사용가능합니다.",
                                            "../assets/offroad/icon_road.png",
                                            this));

  toggles.append(new ParamControl("KeepSteeringTurnSignals",
                                            "깜빡이 작동시 상시조향 유지기능",
                                            "",
                                            "../assets/offroad/icon_openpilot.png",
                                            this));
  toggles.append(new ParamControl("HapticFeedbackWhenSpeedCamera",
                                            "NDA 카메라 과속시 핸들진동 선택",
                                            "NDA 카메라 과속시 핸들진동 선택",
                                            "../assets/offroad/icon_openpilot.png",
                                            this));

  /*toggles.append(new ParamControl("NewRadarInterface",
                                            "Use new radar interface",
                                            "",
                                            "../assets/offroad/icon_road.png",
                                            this));*/

  toggles.append(new ParamControl("DisableOpFcw",
                                            "오파 추돌경고 옵션",
                                            "오픈파일럿이 만들어주는 추돌 경고를 사용여부를 선택..",
                                            "../assets/offroad/icon_shell.png",
                                            this));

  toggles.append(new ParamControl("ShowDebugUI",
                                            "디버그 내용 보기",
                                            "가감속 등 디버그 내용을 화면에 띄웁니다.",
                                            "../assets/offroad/icon_shell.png",
                                            this));

  /*toggles.append(new ParamControl("CustomLeadMark",
                                            "레이더표시를 개조",
                                            "레이더 삼각형 이미지를 커스텀 이미지로 개조합니다.",
                                            "../assets/offroad/icon_road.png",
                                            this));*/

  for(ParamControl *toggle : toggles) {
    if(main_layout->count() != 0) {
      toggleLayout->addWidget(horizontal_line());
    }
    toggleLayout->addWidget(toggle);
  }
  
  toggleLayout->addWidget(horizontal_line());
  toggleLayout->addWidget(new ChargingMin());
  toggleLayout->addWidget(horizontal_line());
  toggleLayout->addWidget(new ChargingMax());
}

SelectCar::SelectCar(QWidget* parent): QWidget(parent) {

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(20);
  main_layout->setSpacing(20);

  // Back button
  QPushButton* back = new QPushButton("닫기");
  back->setObjectName("back_btn");
  back->setFixedSize(500, 100);
  connect(back, &QPushButton::clicked, [=]() { emit backPress(); });
  main_layout->addWidget(back, 0, Qt::AlignLeft);

  QListWidget* list = new QListWidget(this);
  list->setStyleSheet("QListView {padding: 40px; background-color: #393939; border-radius: 15px; height: 140px;} QListView::item{height: 100px}");
  //list->setAttribute(Qt::WA_AcceptTouchEvents, true);
  QScroller::grabGesture(list->viewport(), QScroller::LeftMouseButtonGesture);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  list->addItem("[ 강제 인식 안함 ]");

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

LateralControl::LateralControl(QWidget* parent): QWidget(parent) {

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->setMargin(20);
  main_layout->setSpacing(20);

  // Back button
  QPushButton* back = new QPushButton("Back");
  back->setObjectName("back_btn");
  back->setFixedSize(500, 100);
  connect(back, &QPushButton::clicked, [=]() { emit backPress(); });
  main_layout->addWidget(back, 0, Qt::AlignLeft);

  QListWidget* list = new QListWidget(this);
  list->setStyleSheet("QListView {padding: 40px; background-color: #393939; border-radius: 15px; height: 140px;} QListView::item{height: 100px}");
  //list->setAttribute(Qt::WA_AcceptTouchEvents, true);
  QScroller::grabGesture(list->viewport(), QScroller::LeftMouseButtonGesture);
  list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  QStringList items = {"TORQUE", "LQR", "INDI"};
  list->addItems(items);
  list->setCurrentRow(0);

  QString selectedControl = QString::fromStdString(Params().get("LateralControl"));

  int index = 0;
  for(QString item : items) {
    if(selectedControl == item) {
        list->setCurrentRow(index);
        break;
    }
    index++;
  }

  QObject::connect(list, QOverload<QListWidgetItem*>::of(&QListWidget::itemClicked),
    [=](QListWidgetItem* item){

    Params().put("LateralControl", list->currentItem()->text().toStdString());
    emit selected();

    QTimer::singleShot(1000, []() {
        Params().putBool("SoftRestartTriggered", true);
      });

    });

  main_layout->addWidget(list);
}

//테네시 브랜치 메뉴 추가해보기
TbranchPanel::TbranchPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  // 테네시 깃풀설정등추가

  layout->addWidget(new GitHash()); // 깃 업데이트 시작
  const char* gitpull = "/data/openpilot/gitpull.sh ''";
  auto gitpullBtn = new ButtonControl("깃 풀 업데이트", "실 행");
  layout->addWidget(gitpullBtn);
  QObject::connect(gitpullBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm("브랜치의 업데이트를 다운로드하고 적용합니까? \n펭펭~ 재부팅은 수동으로...펭펭~", this)) {
      std::system(gitpull);
      std::system("rm /data/openpilot/prebuilt");
    }
  });

  layout->addWidget(horizontal_line());
  const char* git_reset = "/data/openpilot/git_reset.sh ''";
  auto gitrestBtn = new ButtonControl("깃 리셋", "실 행");
  layout->addWidget(gitrestBtn);
  QObject::connect(gitrestBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm("C2 에서 수정된것을 초기화 시켜요!\n 재부팅이 안되요~ 펭펭~펭펭~", this)) {
      std::system(git_reset);
      std::system("rm /data/openpilot/prebuilt");
    }
  });

  layout->addWidget(horizontal_line());
  const char* gitpull_cancel = "/data/openpilot/gitpull_cancel.sh ''";
  auto gitpull_cancelBtn = new ButtonControl("깃풀 이전 단계로", "실 행");
  layout->addWidget(gitpull_cancelBtn);
  QObject::connect(gitpull_cancelBtn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm("깃풀을 취소합니다! 펭펭~ \n 재부팅 수동으로...펭펭~", this)) {
      std::system(gitpull_cancel);
      std::system("rm /data/openpilot/prebuilt");
    }
  });
  layout->addWidget(horizontal_line());

  auto TenesiSSHLegacy_Btn = new ButtonControl("공용 SSH키를 사용", "실 행");
  layout->addWidget(TenesiSSHLegacy_Btn);
  QObject::connect(TenesiSSHLegacy_Btn, &ButtonControl::clicked, [=]() {
    if (ConfirmationDialog::confirm("공용 SSH키를 사용하게 설정합니다.! 펭펭~ \n 재부팅 수동으로...펭펭~", this)) {
      std::system("cp -f /data/openpilot/selfdrive/assets/sshkey/GithubSshKeys_legacy /data/params/d/GithubSshKeys");
      std::system("chmod 600 /data/params/d/GithubSshKeys");
    }
  });
 // 깃 업데이트 관련끝

} // 테네시 브랜치 관리메뉴

//테네시 메뉴 추가해보기
TenesiPanel::TenesiPanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  if (!Hardware::TICI()) { // 001 시작
    layout->addWidget(horizontal_line()); // 구분선 구간
    QHBoxLayout *recovery_layout = new QHBoxLayout(); //새로운 버튼 추가를 위한 레이아웃 변수 git_layout
    recovery_layout->setSpacing(12);
    QPushButton *twrpBtn = new QPushButton("Twrp 설치"); // 순정리커버리에서 Twrp설치를 한다..
    twrpBtn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #23481E;");
    recovery_layout->addWidget(twrpBtn);
    QObject::connect(twrpBtn, &QPushButton::released, [=]() {
      if (ConfirmationDialog::confirm("펭펭~ twrp설치를 합니다!\n.펭펭~", this)) {
        std::system("/data/openpilot/twrp.sh ''");
      }
    });

    QPushButton *recoveryBtn = new QPushButton("리커버리로 재부팅");
    recoveryBtn->setStyleSheet("height: 120px;border-radius: 15px;background-color: #E22C2C;");
    recovery_layout->addWidget(recoveryBtn);
    QObject::connect(recoveryBtn, &QPushButton::released, [=]() {
      if (ConfirmationDialog::confirm("펭펭~ 리커버리로 재부팅 합니다!\n.펭펭~", this)) {
        std::system("reboot recovery");
      }
    });
    layout->addLayout(recovery_layout); // 기본에 추가한다..
  } //001 끝

  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("Steer_409",
                                            "조향토크 409설정",
                                            "조향토크를 409로 무조건 설정 GenesisDH 테스트용",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("Steer_SRTune",
                                            "조향비율 가변",
                                            "SR값을 속도대비 가변으로 사용하기",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("Steer_LatTune",
                                            "토크 LatAccel 가변",
                                            "토크 LatAccel을 가변으로 사용하기",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("Steer_90D",
                                            "조향각 90도 제한",
                                            "제네시스DH 조향에러시 사용권고",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("TenesiSSHLegacy",
                                            "SSH 공용키 사용",
                                            "SSH 공용키 사용..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("T_Debug",
                                            "Tmux a 저장옵션",
                                            "Tmux a 내용을 화일로 저장합니다..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("Zorrobyte",
                                            "차로인식 Zorrobyte",
                                            "차로인식 Zorrobyte 적용선택",
                                            "../assets/offroad/icon_road.png",
                                            this));
  layout->addWidget(new ParamControl("Neokiibyte",
                                            "차로인식 NeokiiByte",
                                            "차로인식 NeokiiByte 적용여부 선택",
                                            "../assets/offroad/icon_road.png",
                                            this));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("Sound_Start",
                                            "오프시작시 음성지원 선택",
                                            "오프시작시 음성 온 오프",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("Sound_Slow",
                                            "TMAP 자동감속 음성지원 선택",
                                            "네비 감속시 음성 온 오프",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("SoundAutoHold",
                                            "오토홀드음성지원",
                                            "오토홀드음성을 끄거나 켭니다..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("SoundBsd",
                                            "측후방 음성지원",
                                            "측후방 센서 작동시 음성을 끄거나 켭니다.",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(horizontal_line()); // 구분선 구간
  layout->addWidget(new ParamControl("TenesiCamera",
                                            "NDA 상시 카메라경고",
                                            "NDA 카메라 경고를 계기판과 HUD에 끄거나 켭니다.",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("AutoSetOpt",
                                            "크루즈 오토 셋",
                                            "파파의 오토크루즈 셋 적용",
                                            "../assets/offroad/icon_road.png",
                                            this));
  layout->addWidget(new ParamControl("ShowErpmUI",
                                            "엔진RPM 표시",
                                            "현재엔진RPM 표시하기..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("ShowCgearUI",
                                            "주행기어단수 보기",
                                            "기어레버 위치와 기어단수를 볼수 있습니다..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(new ParamControl("ShowBsdUI",
                                            "측후방정보 보기",
                                            "측후방 감지기 이미지를 볼수 있습니다..",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  layout->addWidget(horizontal_line());

  auto recorddelbtn = new ButtonControl("녹화파일 전부 삭제", "실행");
    QObject::connect(recorddelbtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm("저장된 녹화파일을 모두 삭제합니다. 진행하시겠습니까?", this)){
        std::system("rm -f /data/media/0/videos/*");
      }
    });
    layout->addWidget(recorddelbtn);

  auto realdatadelbtn = new ButtonControl("주행로그-영상 전부 삭제", "실행");
    QObject::connect(realdatadelbtn, &ButtonControl::clicked, [=]() {
      if (ConfirmationDialog::confirm("저장된 주행로그 영상을 모두 삭제합니다. 진행하시겠습니까?", this)){
        std::system("rm -rf /data/media/0/realdata/*");
      }
    });
    layout->addWidget(realdatadelbtn);

} // 테네시 메뉴 추가