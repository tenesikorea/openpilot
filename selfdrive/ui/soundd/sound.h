#pragma once

#include <tuple>

#include <QMap>
#include <QSoundEffect>
#include <QString>

#include "system/hardware/hw.h"
#include "selfdrive/ui/ui.h"


const float MAX_VOLUME = 1.0;

const std::tuple<AudibleAlert, QString, int, float> sound_list[] = {
  // AudibleAlert, file name, loop count
  {AudibleAlert::ENGAGE, "engage.wav", 0, MAX_VOLUME},
  {AudibleAlert::DISENGAGE, "disengage.wav", 0, MAX_VOLUME},
  {AudibleAlert::REFUSE, "refuse.wav", 0, MAX_VOLUME},

  {AudibleAlert::PROMPT, "prompt.wav", 0, MAX_VOLUME},
  {AudibleAlert::PROMPT_REPEAT, "prompt.wav", QSoundEffect::Infinite, MAX_VOLUME},
  {AudibleAlert::PROMPT_DISTRACTED, "prompt_distracted.wav", QSoundEffect::Infinite, MAX_VOLUME},

  {AudibleAlert::WARNING_SOFT, "warning_soft.wav", QSoundEffect::Infinite, MAX_VOLUME},
  {AudibleAlert::WARNING_IMMEDIATE, "warning_immediate.wav", QSoundEffect::Infinite, MAX_VOLUME},

  {AudibleAlert::SLOWING_DOWN_SPEED, "slowing_down_speed.wav", 0, MAX_VOLUME},
  {AudibleAlert::CRUISE_ON, "cruise_on.wav", 0, MAX_VOLUME},
  {AudibleAlert::CRUISE_OFF, "cruise_off.wav", 0, MAX_VOLUME},
  {AudibleAlert::READY, "ready.wav", 0, MAX_VOLUME}, // 여기서부터 커스텀 음성
  {AudibleAlert::TAUTOHOLD, "autohold.wav", 0, MAX_VOLUME},
  {AudibleAlert::TBADBOY, "tstay.wav", 0, MAX_VOLUME},
  {AudibleAlert::TREADY, "tready.wav", 0, MAX_VOLUME},
  {AudibleAlert::TYES, "tyes.wav", 0, MAX_VOLUME},
  //오디오재생과 car.capnp의 관계는 이름과 연관은 있으나 달라도 된다.. car.capnp의 변수명이 event.py에서 쓰이고 이곳은
  //그 이벤트 이름처럼 언더바를 넣어서  연관되게 연동한다.
};

class Sound : public QObject {
public:
  explicit Sound(QObject *parent = 0);

protected:
  void update();
  void setAlert(const Alert &alert);

  SubMaster sm;
  Alert current_alert = {};
  QMap<AudibleAlert, QPair<QSoundEffect *, int>> sounds;
  int current_volume = -1;
};
