#!/usr/bin/env python3
import datetime
import os
import signal
import subprocess
import sys
import traceback
from typing import List, Tuple, Union

import cereal.messaging as messaging
import selfdrive.sentry as sentry
from common.basedir import BASEDIR
from common.params import Params, ParamKeyType
from common.text_window import TextWindow
from selfdrive.boardd.set_time import set_time
from system.hardware import HARDWARE, PC
from selfdrive.manager.helpers import unblock_stdout, write_onroad_params
from selfdrive.manager.process import ensure_running
from selfdrive.manager.process_config import managed_processes
from selfdrive.athena.registration import register, UNREGISTERED_DONGLE_ID
from system.swaglog import cloudlog, add_file_handler
from system.version import is_dirty, get_commit, get_version, get_origin, get_short_branch, \
                           get_normalized_origin, terms_version, training_version, \
                           is_tested_branch, is_release_branch



def manager_init() -> None:
  # update system time from panda
  set_time(cloudlog)

  # save boot log
  #subprocess.call("./bootlog", cwd=os.path.join(BASEDIR, "system/loggerd"))

  params = Params()
  params.clear_all(ParamKeyType.CLEAR_ON_MANAGER_START)

  default_params: List[Tuple[str, Union[str, bytes]]] = [
    ("CompletedTrainingVersion", "0.2.0"),
    ("DisengageOnAccelerator", "0"),
    ("GsmMetered", "1"),
    ("HasAcceptedTerms", "0"),
    ("LanguageSetting", "main_ko"),
    ("OpenpilotEnabledToggle", "1"),
    ("IsMetric", "1"),

    ("TenesiSSHLegacy", "1"),  # SSH 공용키 사용 옵션
    ("SshEnabled", "1"),  # SSH 공용키 사용 옵션
#    ("KeepSteeringTurnSignals", "1"),  # 방향지시등에도 상시조향 작동
    ("HapticFeedbackWhenSpeedCamera", "1"),  # NDA 카메라앞 과속시 허드 계기판 경고
#    ("DisableOpFcw", "1"),  # 오파에서 만드는 FCW경고 옵션
#    ("ShowDebugUI", "0"),
#    ("NewRadarInterface", "0"),
    ("AutoSetOpt", "1"),  # Auto-resume Cruise Set Speed by JangPoo - 파파
    ("ShowTpmsUI", "1"),  # TPMS 보기 안보기
    ("ShowCgearUI", "1"),  # 기어단수 보기 안보기
    ("SoundAutoHold", "1"),  # 오토홀드음성 듣기 안든기
    ("SoundBsd", "1"),  # BSD 음성 듣기 안든기
    ("PutPrebuiltOn", "0"),  # 프리빌트 온오프
    ("TenesiCamera", "1"),  # NDA작동시 티맵 카메라 정보에 따른 선택사항
    ("TenesiCameraSelect", "1"),  # NDA작동시 티맵 카메라 정보에 따른 허드표시 선택사항
    ("ShowBsdUI", "1"),  # bsd 보기옵션
    ("ShowBrakeUI", "1"),  # 브레이크 보기옵션
    ("ShowErpmUI", "1"),  # 엔진Rpm 보기옵션
    ("T_Debug", "0"),  # tmux a 저장용 옵션
    ("Zorrobyte", "1"),  # 차로인식 옵션
    ("Neokiibyte", "1"),  # 차로인식 옵션
    ("Steer_90D", "0"),  # 90도 이상시 에러나는 경우
    ("Steer_SRTune", "0"),  # 가변SR비율사용
    ("Sound_Slow", "1"),  # TAMP자동감속 음성 듣기 안든기
    ("Sound_Start", "1"),  # 펭수 시작음성 듣기 안든기
    ("Steer_409", "1"), #펭수 시작음성 듣기 안든기
    ("Steer_SRTune_v", "92"),  # 가변 조향비율 초기값 수정
    ("Lane_Change", "70"),  # DH기준 기본 차속변경속도 70으로 수정
    ("Auto_engage", "8"),  # 자동인게이지 기본속도
    ("Boost_s", "5"),  # 악셀 부스트 적용 영역 지정- 정지후 적용키로
    ("Creep_Speed_Start", "8"),  # 악셀 부스트 적용 영역 지정- 정지후 적용키로
    ("Boost_v", "30"),  #  악셀 부스트 적용 영역 지정- SCC 속도범위지정
    ("Creep_Speed_Scc", "70"),  #  악셀 부스트 적용 영역 지정- SCC 속도범위지정
    ("Sound_Mdpserr", "1"),  # mdps에러시 음성듣기
    ("TenesiBrakeSelect", "3"),  # 브레이킹작동시 허드에 표시하기
    ("TenesiSccStopCost", "30"),  # vEgoStopping 기본값 0.3를 변경하기..
    ("Tenesi_Gas_Factor", "10"),  # Scc스무서 상태에서 가속이 더디어서 셋팅
    ("SccOnBus2", "1"),  # SCC 2번 개조상태 기본적용
    ("CruiseStateControl", "1"),  # SCC 2번 개조상태 기본적용

  ]
  if not PC:
    default_params.append(("LastUpdateTime", datetime.datetime.utcnow().isoformat().encode('utf8')))

  if params.get_bool("RecordFrontLock"):
    params.put_bool("RecordFront", True)

  # set unset params
  for k, v in default_params:
    if params.get(k) is None:
      params.put(k, v)

  # is this dashcam?
  if os.getenv("PASSIVE") is not None:
    params.put_bool("Passive", bool(int(os.getenv("PASSIVE", "0"))))

  if params.get("Passive") is None:
    raise Exception("Passive must be set to continue")

  # Create folders needed for msgq
  try:
    os.mkdir("/dev/shm")
  except FileExistsError:
    pass
  except PermissionError:
    print("WARNING: failed to make /dev/shm")

  # set version params
  params.put("Version", get_version())
  params.put("TermsVersion", terms_version)
  params.put("TrainingVersion", training_version)
  params.put("GitCommit", get_commit(default=""))
  params.put("GitBranch", get_short_branch(default=""))
  params.put("GitRemote", get_origin(default=""))
  params.put_bool("IsTestedBranch", is_tested_branch())
  params.put_bool("IsReleaseBranch", is_release_branch())

  # set dongle id
  reg_res = register(show_spinner=True)
  if reg_res:
    dongle_id = reg_res
  else:
    serial = params.get("HardwareSerial")
    raise Exception(f"Registration failed for device {serial}")
  os.environ['DONGLE_ID'] = dongle_id  # Needed for swaglog

  if not is_dirty():
    os.environ['CLEAN'] = '1'

  # init logging
  sentry.init(sentry.SentryProject.SELFDRIVE)
  cloudlog.bind_global(dongle_id=dongle_id,
                       version=get_version(),
                       origin=get_normalized_origin(),
                       branch=get_short_branch(),
                       commit=get_commit(),
                       dirty=is_dirty(),
                       device=HARDWARE.get_device_type())


def manager_prepare() -> None:
  for p in managed_processes.values():
    p.prepare()


def manager_cleanup() -> None:
  # send signals to kill all procs
  for p in managed_processes.values():
    p.stop(block=False)

  # ensure all are killed
  for p in managed_processes.values():
    p.stop(block=True)

  cloudlog.info("everything is dead")


def manager_thread() -> None:
  cloudlog.bind(daemon="manager")
  cloudlog.info("manager start")
  cloudlog.info({"environ": os.environ})

  params = Params()

  ignore: List[str] = []
  if params.get("DongleId", encoding='utf8') in (None, UNREGISTERED_DONGLE_ID):
    ignore += ["manage_athenad", "uploader"]
  if os.getenv("NOBOARD") is not None:
    ignore.append("pandad")
  ignore += [x for x in os.getenv("BLOCK", "").split(",") if len(x) > 0]

  sm = messaging.SubMaster(['deviceState', 'carParams'], poll=['deviceState'])
  pm = messaging.PubMaster(['managerState'])

  write_onroad_params(False, params)
  ensure_running(managed_processes.values(), False, params=params, CP=sm['carParams'], not_run=ignore)

  started_prev = False

  while True:
    sm.update()

    started = sm['deviceState'].started

    if started and not started_prev:
      params.clear_all(ParamKeyType.CLEAR_ON_ONROAD_TRANSITION)
    elif not started and started_prev:
      params.clear_all(ParamKeyType.CLEAR_ON_OFFROAD_TRANSITION)

    # update onroad params, which drives boardd's safety setter thread
    if started != started_prev:
      write_onroad_params(started, params)

    started_prev = started

    ensure_running(managed_processes.values(), started, params=params, CP=sm['carParams'], not_run=ignore)

    running = ' '.join("%s%s\u001b[0m" % ("\u001b[32m" if p.proc.is_alive() else "\u001b[31m", p.name)
                       for p in managed_processes.values() if p.proc)
    print(running)
    cloudlog.debug(running)

    # send managerState
    msg = messaging.new_message('managerState')
    msg.managerState.processes = [p.get_process_state_msg() for p in managed_processes.values()]
    pm.send('managerState', msg)

    # Exit main loop when uninstall/shutdown/reboot is needed
    shutdown = False
    for param in ("DoUninstall", "DoShutdown", "DoReboot"):
      if params.get_bool(param):
        shutdown = True
        params.put("LastManagerExitReason", f"{param} {datetime.datetime.now()}")
        cloudlog.warning(f"Shutting down manager - {param} set")

    if shutdown:
      break


def main() -> None:
  prepare_only = os.getenv("PREPAREONLY") is not None

  manager_init()

  # Start UI early so prepare can happen in the background
  if not prepare_only:
    managed_processes['ui'].start()

  manager_prepare()

  if prepare_only:
    return

  # SystemExit on sigterm
  signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(1))

  try:
    manager_thread()
  except Exception:
    traceback.print_exc()
    sentry.capture_exception()
  finally:
    manager_cleanup()

  params = Params()
  if params.get_bool("DoUninstall"):
    cloudlog.warning("uninstalling")
    HARDWARE.uninstall()
  elif params.get_bool("DoReboot"):
    cloudlog.warning("reboot")
    HARDWARE.reboot()
  elif params.get_bool("DoShutdown"):
    cloudlog.warning("shutdown")
    HARDWARE.shutdown()


if __name__ == "__main__":
  unblock_stdout()

  try:
    main()
  except Exception:
    add_file_handler(cloudlog)
    cloudlog.exception("Manager failed to start")

    try:
      managed_processes['ui'].stop()
    except Exception:
      pass

    # Show last 3 lines of traceback
    error = traceback.format_exc(-3)
    error = "Manager failed to start\n\n" + error
    with TextWindow(error) as t:
      t.wait_for_exit()

    raise

  # manual exit because we are forked
  sys.exit(0)
