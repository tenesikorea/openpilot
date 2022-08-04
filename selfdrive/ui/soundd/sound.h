#include <QMap>
#include <QSoundEffect>
#include <QString>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/ui.h"

const std::tuple<AudibleAlert, QString, int> sound_list[] = {
  // AudibleAlert, file name, loop count
  {AudibleAlert::ENGAGE, "engage.wav", 0},
  {AudibleAlert::DISENGAGE, "disengage.wav", 0},
  {AudibleAlert::REFUSE, "refuse.wav", 0},

  {AudibleAlert::PROMPT, "prompt.wav", 0},
  {AudibleAlert::PROMPT_REPEAT, "prompt.wav", QSoundEffect::Infinite},
  {AudibleAlert::PROMPT_DISTRACTED, "prompt_distracted.wav", QSoundEffect::Infinite},

  {AudibleAlert::WARNING_SOFT, "warning_soft.wav", QSoundEffect::Infinite},
  {AudibleAlert::WARNING_IMMEDIATE, "warning_immediate.wav", QSoundEffect::Infinite},

  {AudibleAlert::READY, "ready.wav", 0}, // 여기서부터 커스텀 음성
  {AudibleAlert::TAUTOHOLD, "autohold.wav", 0},
  {AudibleAlert::TBADBOY, "tstay.wav", 0},
  {AudibleAlert::TREADY, "tready.wav", 0},
  {AudibleAlert::TYES, "tyes.wav", 0},
  {AudibleAlert::SLOWING_DOWN_SPEED, "slowing_down_speed.wav", 0},
  //오디오재생과 car.capnp의 관계는 이름과 연관은 있으나 달라도 된다.. car.capnp의 변수명이 event.py에서 쓰이고 이곳은
  //그 이벤트 이름처럼 언더바를 넣어서  연관되게 연동한다.
};

class Sound : public QObject {
public:
  explicit Sound(QObject *parent = 0);

protected:
  void update();
  void setAlert(const Alert &alert);

  Alert current_alert = {};
  QMap<AudibleAlert, QPair<QSoundEffect *, int>> sounds;
  SubMaster sm;
  uint64_t started_frame;
};
