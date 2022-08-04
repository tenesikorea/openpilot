import copy

import crcmod
from selfdrive.car.hyundai.values import CAR, CHECKSUM, FEATURES, EV_HYBRID_CAR

hyundai_checksum = crcmod.mkCrcFun(0x11D, initCrc=0xFD, rev=False, xorOut=0xdf)
# 제네시스DH,G80기준 필요한 핑거
# BO_ 593(0x251) MDPS12: 8 MDPS
# BO_ 832(0x340) LKAS11: 8 LDWS_LKAS
# BO_ 1265(0x4F1) CLU11: 4 CLU 계기판
# BO_ 1056(0x420) SCC11: 8 SCC
# BO_ 1057(0x421) SCC12: 8 SCC
# BO_ 1290(0x50A) SCC13: 8 SCC
# BO_ 905(0x389) SCC14: 8 SCC
# BO_ 1157(0x485) LFAHDA_MFC: 4 XXX LFA관련등

def create_lkas11(packer, frame, car_fingerprint, apply_steer, steer_req,
                  lkas11, sys_warning, sys_state, enabled,
                  left_lane, right_lane,
                  left_lane_depart, right_lane_depart, bus, ldws_opt, cut_steer_temp):
  values = copy.copy(lkas11) # BO_ 832(0x340) LKAS11: 8 LDWS_LKAS
  values["CF_Lkas_LdwsSysState"] = sys_state # sys_state 변수의 내용을  CF_Lkas_LdwsSysState변수에 저장을 한다...
  values["CF_Lkas_SysWarning"] = 3 if sys_warning else 0 #sys_warning의 기본값은 3이지만 경고가 있다면 0으로 대체한다..
  values["CF_Lkas_LdwsLHWarning"] = left_lane_depart # 좌측차로 인식할경우에 carcontroller.py에서 1또는 2를 넣는다..
  values["CF_Lkas_LdwsRHWarning"] = right_lane_depart
  values["CR_Lkas_StrToqReq"] = apply_steer
  values["CF_Lkas_ActToi"] = steer_req and not cut_steer_temp
  values["CF_Lkas_ToiFlt"] = cut_steer_temp  # seems to allow actuation on CR_Lkas_StrToqReq
  values["CF_Lkas_Chksum"] = 0
  if car_fingerprint == CAR.GENESIS:
    values["CF_Lkas_MsgCount"] = frame % 0x10 # LKAS신호이상 개선해보기
  else:
    values["CF_Lkas_MsgCount"] = frame % 0x10

  if car_fingerprint in FEATURES["send_lfa_mfa"]:
    values["CF_Lkas_LdwsActivemode"] = int(left_lane) + (int(right_lane) << 1) # 차로 미인식시 0이 나오고 좌측1 우측은 2 양쪽 차로 인식시 3 이다..
    values["CF_Lkas_LdwsOpt_USM"] = 2 # 차량별 LKAS LDWS등의 고유값..

    # FcwOpt_USM 5 = Orange blinking car + lanes
    # FcwOpt_USM 4 = Orange car + lanes
    # FcwOpt_USM 3 = Green blinking car + lanes
    # FcwOpt_USM 2 = Green car + lanes
    # FcwOpt_USM 1 = White car + lanes
    # FcwOpt_USM 0 = No car + lanes
    values["CF_Lkas_FcwOpt_USM"] = 2 if enabled else 1

    # SysWarning 4 = keep hands on wheel
    # SysWarning 5 = keep hands on wheel (red)
    # SysWarning 6 = keep hands on wheel (red) + beep
    # Note: the warning is hidden while the blinkers are on
    values["CF_Lkas_SysWarning"] = 4 if sys_warning else 0 # 평상시 0으로 작동 4가 나올때에는 차량에 알려준다..fcw기능같은것..

  elif car_fingerprint == CAR.GENESIS:
    # This field is actually LdwsActivemode
    # Genesis and Optima fault when forwarding while engaged
    values["CF_Lkas_LdwsActivemode"] = 2
    values["CF_Lkas_SysWarning"] = lkas11["CF_Lkas_SysWarning"]

  elif car_fingerprint == CAR.SONATA_LF_TURBO:
    values["CF_Lkas_LdwsOpt_USM"] = 2
    values["CF_Lkas_FcwOpt_USM"] = 2 if enabled else 1
    values["CF_Lkas_SysWarning"] = 4 if sys_warning else 0

  if ldws_opt:
    values["CF_Lkas_LdwsOpt_USM"] = 3

  dat = packer.make_can_msg("LKAS11", 0, values)[2]

  if car_fingerprint in CHECKSUM["crc8"]:
    # CRC Checksum as seen on 2019 Hyundai Santa Fe
    dat = dat[:6] + dat[7:8]
    checksum = hyundai_checksum(dat)
  elif car_fingerprint in CHECKSUM["6B"]:
    # Checksum of first 6 Bytes, as seen on 2018 Kia Sorento
    checksum = sum(dat[:6]) % 256
  else:
    # Checksum of first 6 Bytes and last Byte as seen on 2018 Kia Stinger
    checksum = (sum(dat[:6]) + dat[7]) % 256

  values["CF_Lkas_Chksum"] = checksum

  return packer.make_can_msg("LKAS11", bus, values)

def create_clu11(packer, bus, clu11, button, speed):
  values = copy.copy(clu11) # BO_ 1265(0x4F1) CLU11: 4 CLU 계기판
  values["CF_Clu_CruiseSwState"] = button
  values["CF_Clu_Vanz"] = speed
  values["CF_Clu_AliveCnt1"] = (values["CF_Clu_AliveCnt1"] + 1) % 0x10
  return packer.make_can_msg("CLU11", bus, values)

def create_lfahda_mfc(packer, enabled, active):
  values = {
    "LFA_Icon_State": 2 if enabled else 0,
    "HDA_Active": 1 if active > 0 else 0,
    "HDA_Icon_State": 2 if active > 0 else 0,
    # "HDA_VSetReq": 0,
  }

  # VAL_ 1157 LFA_Icon_State 0 "no_wheel" 1 "white_wheel" 2 "green_wheel" 3 "green_wheel_blink";
  # VAL_ 1157 LFA_SysWarning 0 "no_message" 1 "switching_to_hda" 2 "switching_to_scc" 3 "lfa_error" 4 "check_hda" 5 "keep_hands_on_wheel_orange" 6 "keep_hands_on_wheel_red";
  # VAL_ 1157 HDA_Icon_State 0 "no_hda" 1 "white_hda" 2 "green_hda";
  # VAL_ 1157 HDA_SysWarning 0 "no_message" 1 "driving_convenience_systems_cancelled" 2 "highway_drive_assist_system_cancelled";

  return packer.make_can_msg("LFAHDA_MFC", 0, values)

def create_hda_mfc(packer, active, CS, left_lane, right_lane):
  values = copy.copy(CS.lfahda_mfc)

  ldwSysState = 0
  if left_lane:
    ldwSysState += 1
  if right_lane:
    ldwSysState += 2

  values["HDA_LdwSysState"] = ldwSysState
  values["HDA_USM"] = 2
  values["HDA_VSetReq"] = 100
  values["HDA_Icon_Wheel"] = 1 if active > 1 and CS.out.cruiseState.enabledAcc else 0
  values["HDA_Icon_State"] = 2 if active > 1 else 0
  values["HDA_Chime"] = 1 if active > 1 else 0
  # 파파  HDA 개인화 적용
  return packer.make_can_msg("LFAHDA_MFC", 0, values)

def create_mdps12(packer, frame, mdps12):
  values = copy.copy(mdps12) # BO_ 593(0x251) MDPS12: 8 MDPS
  values["CF_Mdps_ToiActive"] = 0
  values["CF_Mdps_ToiUnavail"] = 1
  values["CF_Mdps_MsgCount2"] = frame % 0x100 # 0x100을 50으로 수정해봄
  values["CF_Mdps_Chksum2"] = 0

  dat = packer.make_can_msg("MDPS12", 2, values)[2]
  checksum = sum(dat) % 256
  values["CF_Mdps_Chksum2"] = checksum

  return packer.make_can_msg("MDPS12", 2, values)

def create_scc11(packer, frame, enabled, set_speed, lead_visible, scc_live, scc11, active_cam, stock_cam):
  values = copy.copy(scc11) # BO_ 1056(0x420) SCC11: 8 SCC
  values["AliveCounterACC"] = frame // 2 % 0x10

  if not stock_cam:
    values["Navi_SCC_Camera_Act"] = 2 if active_cam else 0
    values["Navi_SCC_Camera_Status"] = 2 if active_cam else 0

  if not scc_live:
    values["MainMode_ACC"] = 1
    values["VSetDis"] = set_speed
    values["ObjValid"] = 1 if enabled else 0
#  values["ACC_ObjStatus"] = lead_visible

  return packer.make_can_msg("SCC11", 0, values)

def create_scc12(packer, apply_accel, enabled, cnt, scc_live, scc12, gaspressed, brakepressed,
                 standstill, car_fingerprint):
  values = copy.copy(scc12)

  if car_fingerprint in EV_HYBRID_CAR:
    # from xps-genesis
    if enabled and not brakepressed:
      values["ACCMode"] = 2 if gaspressed and (apply_accel > -0.2) else 1
      if apply_accel < 0.0 and standstill:
        values["StopReq"] = 1
      values["aReqRaw"] = apply_accel
      values["aReqValue"] = apply_accel
    else:
      values["ACCMode"] = 0
      values["aReqRaw"] = 0
      values["aReqValue"] = 0

    if not scc_live:
      values["CR_VSM_Alive"] = cnt

  else:
    values["aReqRaw"] = apply_accel if enabled else 0  # aReqMax
    values["aReqValue"] = apply_accel if enabled else 0  # aReqMin
    values["CR_VSM_Alive"] = cnt
    if not scc_live:
      values["ACCMode"] = 1 if enabled else 0  # 2 if gas padel pressed

  values["CR_VSM_ChkSum"] = 0
  dat = packer.make_can_msg("SCC12", 0, values)[2]
  values["CR_VSM_ChkSum"] = 16 - sum([sum(divmod(i, 16)) for i in dat]) % 16

  return packer.make_can_msg("SCC12", 0, values)

def create_scc13(packer, scc13):
  values = copy.copy(scc13) # BO_ 1290(0x50A) SCC13: 8 SCC
  return packer.make_can_msg("SCC13", 0, values)

def create_scc14(packer, enabled, e_vgo, standstill, accel, gaspressed, objgap, scc14):
  values = copy.copy(scc14)  # BO_ 905(0x389) SCC14: 8 SCC

  # from xps-genesis
  if enabled:
    values["ACCMode"] = 2 if gaspressed and (accel > -0.2) else 1
    values["ObjGap"] = objgap
    if standstill:
      values["JerkUpperLimit"] = 0.5
      values["JerkLowerLimit"] = 10.
      values["ComfortBandUpper"] = 0.
      values["ComfortBandLower"] = 0.
      if e_vgo > 0.27:
        values["ComfortBandUpper"] = 2.
        values["ComfortBandLower"] = 0.
    else:
      values["JerkUpperLimit"] = 50.
      values["JerkLowerLimit"] = 50.
      values["ComfortBandUpper"] = 50.
      values["ComfortBandLower"] = 50.

  return packer.make_can_msg("SCC14", 0, values)
