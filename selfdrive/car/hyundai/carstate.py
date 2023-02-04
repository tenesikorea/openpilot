from cereal import car
from common.numpy_fast import interp
from selfdrive.car.hyundai.values import DBC, STEER_THRESHOLD, FEATURES, CAR, HYBRID_CAR, EV_HYBRID_CAR
from selfdrive.car.interfaces import CarStateBase
from opendbc.can.parser import CANParser
from opendbc.can.can_define import CANDefine
from common.conversions import Conversions as CV
from common.params import Params
import sys # dbc모니터링 저장용

GearShifter = car.CarState.GearShifter


class CarState(CarStateBase):
  def __init__(self, CP):
    super().__init__(CP)

    can_define = CANDefine(DBC[CP.carFingerprint]["pt"]) # 교주사마 현재기어 등등

    if self.CP.carFingerprint in FEATURES["use_cluster_gears"]:
      self.shifter_values = can_define.dv["CLU15"]["CF_Clu_Gear"]
    elif self.CP.carFingerprint in FEATURES["use_tcu_gears"]:
      self.shifter_values = can_define.dv["TCU12"]["CUR_GR"]
    else:  # preferred and elect gear methods use same definition
      self.shifter_values = can_define.dv["LVR12"]["CF_Lvr_Gear"]  # 교주사마 현재기어 등등

    #Auto detection for setup
    self.no_radar = CP.sccBus == -1
    self.mdps_bus = CP.mdpsBus
    self.sas_bus = CP.sasBus
    self.scc_bus = CP.sccBus
    self.has_scc13 = CP.hasScc13 or CP.carFingerprint in FEATURES["has_scc13"]
    self.has_scc14 = CP.hasScc14 or CP.carFingerprint in FEATURES["has_scc14"]
    self.has_lfa_hda = CP.hasLfaHda
    self.leftBlinker = False
    self.rightBlinker = False
    self.cruise_main_button = 0
    self.mdps_error_cnt = 0
    self.cruise_unavail_cnt = 0

    self.apply_steer = 0.

    # scc smoother
    self.acc_mode = False
    self.cruise_gap = 1
    self.brake_pressed = False
    self.gas_pressed = False
    self.standstill = False
    self.cruiseState_enabled = False
    self.cruiseState_speed = 0

    # Auto-resume Cruise Set Speed by JangPoo - 파파
    self.prev_cruiseState_speed = 0
    self.obj_valid = 0
    # Auto-resume Cruise Set Speed by JangPoo - 파파

    self.use_cluster_speed = Params().get_bool('UseClusterSpeed')
    self.long_control_enabled = Params().get_bool('LongControlEnabled')
    #self.gear_Shifter = GearShifter.unknown  # 기어레버 특수한 환경용
    self.dhmdps_err = 0 # DHMDPS 고장코드 자동 삭제용

  def update(self, cp, cp2, cp_cam):
    cp_mdps = cp2 if self.mdps_bus else cp
    cp_sas = cp2 if self.sas_bus else cp
    cp_scc = cp2 if self.scc_bus == 1 else cp_cam if self.scc_bus == 2 else cp

    self.prev_cruise_buttons = self.cruise_buttons
    self.prev_cruise_main_button = self.cruise_main_button
    self.prev_left_blinker = self.leftBlinker
    self.prev_right_blinker = self.rightBlinker

    ret = car.CarState.new_message()

    ret.doorOpen = any([cp.vl["CGW1"]["CF_Gway_DrvDrSw"], cp.vl["CGW1"]["CF_Gway_AstDrSw"],
                        cp.vl["CGW2"]["CF_Gway_RLDrSw"], cp.vl["CGW2"]["CF_Gway_RRDrSw"]])

    ret.seatbeltUnlatched = cp.vl["CGW1"]["CF_Gway_DrvSeatBeltSw"] == 0

    self.is_set_speed_in_mph = bool(cp.vl["CLU11"]["CF_Clu_SPEED_UNIT"])
    self.speed_conv_to_ms = CV.MPH_TO_MS if self.is_set_speed_in_mph else CV.KPH_TO_MS

    cluSpeed = cp.vl["CLU11"]["CF_Clu_Vanz"]
    decimal = cp.vl["CLU11"]["CF_Clu_VanzDecimal"]
    if 0. < decimal < 0.5:
      cluSpeed += decimal

    ret.cluSpeedMs = cluSpeed * self.speed_conv_to_ms

    ret.wheelSpeeds = self.get_wheel_speeds(
      cp.vl["WHL_SPD11"]["WHL_SPD_FL"],
      cp.vl["WHL_SPD11"]["WHL_SPD_FR"],
      cp.vl["WHL_SPD11"]["WHL_SPD_RL"],
      cp.vl["WHL_SPD11"]["WHL_SPD_RR"],
    )

    vEgoRawClu = cluSpeed * self.speed_conv_to_ms
    vEgoClu, aEgoClu = self.update_clu_speed_kf(vEgoRawClu)

    vEgoRawWheel = (ret.wheelSpeeds.fl + ret.wheelSpeeds.fr + ret.wheelSpeeds.rl + ret.wheelSpeeds.rr) / 4.
    vEgoRawWheel = interp(vEgoRawWheel, [0., 10.], [(vEgoRawWheel + vEgoRawClu) / 2., vEgoRawWheel])
    vEgoWheel, aEgoWheel = self.update_speed_kf(vEgoRawWheel)

    if self.use_cluster_speed:
      ret.vEgoRaw = vEgoRawClu
      ret.vEgo = vEgoClu
      ret.aEgo = aEgoClu
    else:
      ret.vEgoRaw = vEgoRawWheel
      ret.vEgo = vEgoWheel
      ret.aEgo = aEgoWheel

    ret.vCluRatio = (vEgoWheel / vEgoClu) if (vEgoClu > 3. and vEgoWheel > 3.) else 1.0
    ret.aBasis = cp.vl["TCS13"]["aBasis"]

    ret.standstill = ret.vEgoRaw < 0.01

    ret.steeringAngleDeg = cp_sas.vl["SAS11"]["SAS_Angle"] # 0x2B0 MDPS 정보관련
    ret.steeringRateDeg = cp_sas.vl["SAS11"]["SAS_Speed"] # 0x220 ESP 스마트크루즈관련등등 여러가지 정보
    ret.yawRate = cp.vl["ESP12"]["YAW_RATE"]
    ret.leftBlinker, ret.rightBlinker = self.update_blinker_from_lamp(50, cp.vl["CGW1"]["CF_Gway_TurnSigLh"],
                                                            cp.vl["CGW1"]["CF_Gway_TurnSigRh"])
    ret.steeringTorque = cp_mdps.vl["MDPS12"]["CR_Mdps_StrColTq"]
    ret.steeringTorqueEps = cp_mdps.vl["MDPS12"]["CR_Mdps_OutTq"] / 10.  # scale to Nm
    ret.steeringPressed = abs(ret.steeringTorque) > STEER_THRESHOLD

    if not ret.standstill and cp_mdps.vl["MDPS12"]["CF_Mdps_ToiUnavail"] != 0:
      self.mdps_error_cnt += 1
    else:
      self.mdps_error_cnt = 0

    ret.steerFaultTemporary = self.mdps_error_cnt > 256 # 테네시50쓰다가 줄임 220420일자

    if self.CP.enableAutoHold: # 교주님 추가한 오토홀드 메시지 UI화면에서 표시용
      ret.autoHold = cp.vl["ESP11"]["AVH_STAT"]
      self.brakeHold = (cp.vl["ESP11"]["AVH_STAT"] == 1) # 오토홀드 음성지원용 변수

    # cruise state
    ret.cruiseState.enabled = (cp_scc.vl["SCC12"]["ACCMode"] != 0) if not self.no_radar else \
                                      cp.vl["LVR12"]["CF_Lvr_CruiseSet"] != 0
    ret.cruiseState.available = (cp_scc.vl["SCC11"]["MainMode_ACC"] != 0) if not self.no_radar else \
                                      cp.vl["EMS16"]["CRUISE_LAMP_M"] != 0
    ret.cruiseState.standstill = cp_scc.vl["SCC11"]["SCCInfoDisplay"] == 4. if not self.no_radar else False

    ret.cruiseState.enabledAcc = ret.cruiseState.enabled

    if ret.cruiseState.enabled:
      ret.cruiseState.speed = cp_scc.vl["SCC11"]["VSetDis"] * self.speed_conv_to_ms if not self.no_radar else \
                                         cp.vl["LVR12"]["CF_Lvr_CruiseSet"] * self.speed_conv_to_ms
    else:
      ret.cruiseState.speed = 0
    self.cruise_main_button = cp.vl["CLU11"]["CF_Clu_CruiseSwMain"]
    self.cruise_buttons = cp.vl["CLU11"]["CF_Clu_CruiseSwState"]

    # TODO: Find brake pressure
    ret.brake = 0
    ret.brakePressed = cp.vl["TCS13"]["DriverBraking"] != 0
    ret.brakeHoldActive = cp.vl["TCS15"]["AVH_LAMP"] == 2  # 0 OFF, 1 ERROR, 2 ACTIVE, 3 READY
    ret.parkingBrake = cp.vl["TCS13"]["PBRAKE_ACT"] == 1
    #ret.parkingBrake = cp.vl["CGW1"]["CF_Gway_ParkBrakeSw"]

    # TODO: Check this
    ret.brakeLights = bool(cp.vl["TCS13"]["BrakeLight"] or ret.brakePressed)
    ret.gasPressed = cp.vl["TCS13"]["DriverOverride"] == 1
    self.currentBrake = ret.brakeLights  # 브레이크등 작동시 허드에 표시하기

    if self.CP.carFingerprint in EV_HYBRID_CAR:
      if self.CP.carFingerprint in HYBRID_CAR:
        ret.gas = cp.vl["E_EMS11"]["CR_Vcu_AccPedDep_Pos"] / 254.
      else:
        ret.gas = cp.vl["E_EMS11"]["Accel_Pedal_Pos"] / 254.

    if self.CP.hasEms:
      ret.gas = cp.vl["EMS12"]["PV_AV_CAN"] / 100.
      ret.gasPressed = bool(cp.vl["EMS16"]["CF_Ems_AclAct"])

    NOT_GEAR = [CAR.KONA_EV, CAR.NIRO_EV, CAR.IONIQ_EV_2020]
# 테네시 현재기어단수 적용
    if not self.car_fingerprint in NOT_GEAR : # 현재 기어 단수를 표시하기 위한 작업
      ret.currentGear = cp.vl["LVR11"]["CF_Lvr_GearInf"] # 핑거 870 내용교체함 CF_Lvr_CGear
      ret.currentErpm = cp.vl["EMS_366"]["ENG_RPM"]
      if self.car_fingerprint in [CAR.GENESIS_G70] :
        ret.currentGear = cp.vl["LVR11"]["G70_Gear"]

    # TODO: refactor gear parsing in function
    # Gear Selection via Cluster - For those Kia/Hyundai which are not fully discovered, we can use the Cluster Indicator for Gear Selection,
    # as this seems to be standard over all cars, but is not the preferred method.
    if self.CP.carFingerprint in FEATURES["use_cluster_gears"]:
      #disp = cp.vl["CLU15"] # 0x52A 계기판 정보에서 기어 레버정보가 나올때
      #print(gear_disp)
      gear = cp.vl["CLU15"]["CF_Clu_Gear"]
    elif self.CP.carFingerprint in FEATURES["use_tcu_gears"]:
      #gear_disp = cp.vl["TCU12"]
      #print(gear_disp)
      gear = cp.vl["TCU12"]["CUR_GR"]
    elif self.CP.carFingerprint in FEATURES["use_elect_gears"]:
      #gear_disp = cp.vl["ELECT_GEAR"]
      #print(gear_disp)
      gear = cp.vl["ELECT_GEAR"]["Elect_Gear_Shifter"]
    else:
      #gear_disp = cp.vl["LVR12"] # 0x367 871계기판과 TCU 정보교환데이터가 있을때
      #print(gear_disp)
      gear = cp.vl["LVR12"]["CF_Lvr_Gear"]

    ret.gearShifter = self.parse_gear_shifter(self.shifter_values.get(gear))

    if self.CP.carFingerprint in FEATURES["use_fca"]:
      ret.stockAeb = cp.vl["FCA11"]["FCA_CmdAct"] != 0 # 0xFCA
      ret.stockFcw = cp.vl["FCA11"]["CF_VSM_Warn"] == 2
    else:
      ret.stockAeb = cp.vl["SCC12"]["AEB_CmdAct"] != 0
      ret.stockFcw = cp.vl["SCC12"]["CF_VSM_Warn"] == 2

    # Blind Spot Detection and Lane Change Assist signals
    if self.CP.enableBsm: # 0x58B 후측방 관련 정보들
      ret.leftBlindspot = cp.vl["LCA11"]["CF_Lca_IndLeft"] != 0
      ret.rightBlindspot = cp.vl["LCA11"]["CF_Lca_IndRight"] != 0
    else:
      ret.leftBlindspot = False
      ret.rightBlindspot = False

    # save the entire LKAS11, CLU11, SCC12 and MDPS12
    self.lkas11 = cp_cam.vl["LKAS11"] # 0x340 후측방 관련 정보들
    self.clu11 = cp.vl["CLU11"]
    self.scc11 = cp_scc.vl["SCC11"]
    self.scc12 = cp_scc.vl["SCC12"]
    self.mdps12 = cp_mdps.vl["MDPS12"]
    self.lfahda_mfc = cp_cam.vl["LFAHDA_MFC"]
    self.steer_state = cp_mdps.vl["MDPS12"]["CF_Mdps_ToiActive"] #0 NOT ACTIVE, 1 ACTIVE
    self.cruise_unavail_cnt += 1 if cp.vl["TCS13"]["CF_VSM_Avail"] != 1 and cp.vl["TCS13"]["ACCEnable"] != 0 else -self.cruise_unavail_cnt
    self.cruise_unavail = self.cruise_unavail_cnt > 100

    self.lead_distance = cp_scc.vl["SCC11"]["ACC_ObjDist"] if not self.no_radar else 0
    if self.has_scc13:
      self.scc13 = cp_scc.vl["SCC13"] # SCC 전방레이더 정보
    if self.has_scc14:
      self.scc14 = cp_scc.vl["SCC14"]

    # scc smoother
    driver_override = cp.vl["TCS13"]["DriverOverride"]
    self.acc_mode = cp_scc.vl["SCC12"]["ACCMode"] != 0
    self.cruise_gap = cp_scc.vl["SCC11"]["TauGapSet"] if not self.no_radar else 1
    self.gas_pressed = ret.gasPressed or driver_override == 1
    self.brake_pressed = ret.brakePressed or driver_override == 2
    self.standstill = ret.standstill or ret.cruiseState.standstill
    self.cruiseState_enabled = ret.cruiseState.enabled
    self.cruiseState_speed = ret.cruiseState.speed
    ret.cruiseGap = self.cruise_gap

    tpms_unit = cp.vl["TPMS11"]["UNIT"] * 0.725 if int(cp.vl["TPMS11"]["UNIT"]) > 0 else 1.
    ret.tpms.fl = tpms_unit * cp.vl["TPMS11"]["PRESSURE_FL"]
    ret.tpms.fr = tpms_unit * cp.vl["TPMS11"]["PRESSURE_FR"]
    ret.tpms.rl = tpms_unit * cp.vl["TPMS11"]["PRESSURE_RL"]
    ret.tpms.rr = tpms_unit * cp.vl["TPMS11"]["PRESSURE_RR"]

    # Auto-resume Cruise Set Speed by JangPoo - 파파
    self.prev_cruiseState_speed = self.cruiseState_speed if self.cruiseState_speed else self.prev_cruiseState_speed
    self.obj_valid = cp_scc.vl["SCC11"]['ObjValid']

    if self.prev_cruise_buttons == 4: #cancle
      self.prev_cruiseState_speed = 0 # 여기까지....
    # Auto-resume Cruise Set Speed by JangPoo - 파파

    if Params().get_bool('T_Debug'): # 테네시 음성관련
      sys.stdout = open('/data/media/0/tenesilog.txt', 'a') #  화일에 저장시 필요

    #can_lkas11 = cp_cam.vl["LKAS11"]  # MDPS12 데이터를 본다..
    #print(can_lkas11)
    can_mdps11 = cp_mdps.vl["MDPS11"]  # MDPS12 데이터를 본다..
    print(can_mdps11)
    can_mdps12 = cp_mdps.vl["MDPS12"]  # MDPS12 데이터를 본다..
    print(can_mdps12)
    #can_sas11 = cp_sas.vl["SAS11"] # 0x2B0 MDPS 정보관련
    #print(can_sas11)
    #can_navi11 = cp_sas.vl["NAVI"] # 0x2B0 MDPS 정보관련
    #print(can_navi11)
    #can_scc11 = cp_sas.vl["SCC11"] # 0x2B0 MDPS 정보관련
    #print(can_scc11)

    return ret

  def update_hda2(self, cp, cp_cam):
    ret = car.CarState.new_message()

    ret.gas = cp.vl["ACCELERATOR"]["ACCELERATOR_PEDAL"] / 255.
    ret.gasPressed = ret.gas > 1e-3
    ret.brakePressed = cp.vl["BRAKE"]["BRAKE_PRESSED"] == 1

    ret.doorOpen = cp.vl["DOORS_SEATBELTS"]["DRIVER_DOOR_OPEN"] == 1
    ret.seatbeltUnlatched = cp.vl["DOORS_SEATBELTS"]["DRIVER_SEATBELT_LATCHED"] == 0

    gear = cp.vl["ACCELERATOR"]["GEAR"]
    ret.gearShifter = self.parse_gear_shifter(self.shifter_values.get(gear))

    # TODO: figure out positions
    ret.wheelSpeeds = self.get_wheel_speeds(
      cp.vl["WHEEL_SPEEDS"]["WHEEL_SPEED_1"],
      cp.vl["WHEEL_SPEEDS"]["WHEEL_SPEED_2"],
      cp.vl["WHEEL_SPEEDS"]["WHEEL_SPEED_3"],
      cp.vl["WHEEL_SPEEDS"]["WHEEL_SPEED_4"],
    )
    ret.vEgoRaw = (ret.wheelSpeeds.fl + ret.wheelSpeeds.fr + ret.wheelSpeeds.rl + ret.wheelSpeeds.rr) / 4.
    ret.vEgo, ret.aEgo = self.update_speed_kf(ret.vEgoRaw)
    ret.standstill = ret.vEgoRaw < 0.1

    ret.steeringRateDeg = cp.vl["STEERING_SENSORS"]["STEERING_RATE"]
    ret.steeringAngleDeg = cp.vl["STEERING_SENSORS"]["STEERING_ANGLE"] * -1
    ret.steeringTorque = cp.vl["MDPS"]["STEERING_COL_TORQUE"]
    ret.steeringTorqueEps = cp.vl["MDPS"]["STEERING_OUT_TORQUE"]
    ret.steeringPressed = abs(ret.steeringTorque) > self.params.STEER_THRESHOLD

    ret.leftBlinker, ret.rightBlinker = self.update_blinker_from_lamp(50, cp.vl["BLINKERS"]["LEFT_LAMP"],
                                                                      cp.vl["BLINKERS"]["RIGHT_LAMP"])

    ret.cruiseState.available = True
    ret.cruiseState.enabled = cp.vl["SCC1"]["CRUISE_ACTIVE"] == 1
    ret.cruiseState.standstill = cp.vl["CRUISE_INFO"]["CRUISE_STANDSTILL"] == 1

    speed_factor = CV.MPH_TO_MS if cp.vl["CLUSTER_INFO"]["DISTANCE_UNIT"] == 1 else CV.KPH_TO_MS
    ret.cruiseState.speed = cp.vl["CRUISE_INFO"]["SET_SPEED"] * speed_factor

    self.cruise_buttons.extend(cp.vl_all["CRUISE_BUTTONS"]["CRUISE_BUTTONS"])
    self.main_buttons.extend(cp.vl_all["CRUISE_BUTTONS"]["ADAPTIVE_CRUISE_MAIN_BTN"])
    self.buttons_counter = cp.vl["CRUISE_BUTTONS"]["COUNTER"]

    self.cam_0x2a4 = copy.copy(cp_cam.vl["CAM_0x2a4"])
    return ret

  @staticmethod
  def get_can_parser(CP):

    signals = [
      # sig_name, sig_address
      ("WHL_SPD_FL", "WHL_SPD11"), # 0x386  ABS모듈에서 읽는다..
      ("WHL_SPD_FR", "WHL_SPD11"),
      ("WHL_SPD_RL", "WHL_SPD11"),
      ("WHL_SPD_RR", "WHL_SPD11"),

      ("YAW_RATE", "ESP12"), # 0x220 롱컨트롤 악셀값등등-G센서 가속도값? 경사로 하향주행시 속도 유지장치

      ("CF_Gway_DrvSeatBeltInd", "CGW4"),

      ("CF_Gway_DrvSeatBeltSw", "CGW1"), # 0x541 BCM에서 나오는 신호를 체크 안전벨트 체크
      ("CF_Gway_DrvDrSw", "CGW1"),       # Driver Door
      ("CF_Gway_AstDrSw", "CGW1"),       # Passenger door
      ("CF_Gway_RLDrSw", "CGW2"),        # Rear reft door 0x553 2열 도어 신호
      ("CF_Gway_RRDrSw", "CGW2"),        # Rear right door
      ("CF_Gway_TurnSigLh", "CGW1"),
      ("CF_Gway_TurnSigRh", "CGW1"),
      ("CF_Gway_ParkBrakeSw", "CGW1"),   # Parking Brake

      ("CYL_PRES", "ESP12"), # 0x220 롱컨트롤 악셀값등등 사용자 브레이킹 압력?

      ("CF_Clu_CruiseSwState", "CLU11"), # 0x4f1 계기판정보 보기
      ("CF_Clu_CruiseSwMain", "CLU11"),
      ("CF_Clu_SldMainSW", "CLU11"),
      ("CF_Clu_ParityBit1", "CLU11"),
      ("CF_Clu_VanzDecimal" , "CLU11"),
      ("CF_Clu_Vanz", "CLU11"),
      ("CF_Clu_SPEED_UNIT", "CLU11"),
      ("CF_Clu_DetentOut", "CLU11"),
      ("CF_Clu_RheostatLevel", "CLU11"),
      ("CF_Clu_CluInfo", "CLU11"),
      ("CF_Clu_AmpInfo", "CLU11"),
      ("CF_Clu_AliveCnt1", "CLU11"),

      ("ACCEnable", "TCS13"), #0x394 브레이크등이 들어오는 신호등등
      ("BrakeLight", "TCS13"),
      ("aBasis", "TCS13"),
      ("DriverBraking", "TCS13"),
      ("PBRAKE_ACT", "TCS13"),
      ("DriverOverride", "TCS13"),
      ("CF_VSM_Avail", "TCS13"),

      ("ESC_Off_Step", "TCS15"), #0x507
      ("AVH_LAMP", "TCS15"),

      ("CF_Lvr_GearInf", "LVR11"), # 테네시 추가 0x368 기어레버위치 확인용 (0 = N or P, 1-8 = Fwd, 14 = Rev) # 테네시 추가 0x368 tcu주행기어단수
      ("AVH_STAT", "ESP11"),  # 테네시  추가 0x47F 오토홀드작동 확인데이터용등 - 교주님도 추가함
      ("G70_Gear", "LVR11"),  # 테네시 추가 0x368 기어레버위치 확인용 (0 = N or P, 1-8 = Fwd, 14 = Rev) # 테네시 추가 0x368 tcu주행기어단수
      ("ENG_RPM", "EMS_366"), # 테네시 추가 엔진RPM 모니터링

      ("MainMode_ACC", "SCC11"), #0x420 SCC11 전방레이더
      ("SCCInfoDisplay", "SCC11"),
      ("AliveCounterACC", "SCC11"),
      ("VSetDis", "SCC11"), # 최소 크루즈작동속도 30에서 교체해봄
      ("ObjValid", "SCC11"),
      ("DriverAlertDisplay", "SCC11"),
      ("TauGapSet", "SCC11"),
      ("ACC_ObjStatus", "SCC11"),
      ("ACC_ObjLatPos", "SCC11"),
      ("ACC_ObjDist", "SCC11"), #TK211X value is 204.6
      ("ACC_ObjRelSpd", "SCC11"),
      ("Navi_SCC_Curve_Status", "SCC11"),
      ("Navi_SCC_Curve_Act", "SCC11"),
      ("Navi_SCC_Camera_Act", "SCC11"),
      ("Navi_SCC_Camera_Status", "SCC11"),

      ("ACCMode", "SCC12"), #0x421 SCC12 전방레이더
      ("CF_VSM_Prefill", "SCC12"),
      ("CF_VSM_DecCmdAct", "SCC12"),
      ("CF_VSM_HBACmd", "SCC12"),
      ("CF_VSM_Warn", "SCC12"),
      ("CF_VSM_Stat", "SCC12"),
      ("CF_VSM_BeltCmd", "SCC12"),
      ("ACCFailInfo", "SCC12"),
      ("StopReq", "SCC12"),
      ("CR_VSM_DecCmd", "SCC12"),
      ("aReqRaw", "SCC12"), #aReqMax
      ("TakeOverReq", "SCC12"),
      ("PreFill", "SCC12"),
      ("aReqValue", "SCC12"), #aReqMin
      ("CF_VSM_ConfMode", "SCC12"),
      ("AEB_Failinfo", "SCC12"),
      ("AEB_Status", "SCC12"),
      ("AEB_CmdAct", "SCC12"),
      ("AEB_StopReq", "SCC12"),
      ("CR_VSM_Alive", "SCC12"),
      ("CR_VSM_ChkSum", "SCC12"),

      ("SCCDrvModeRValue", "SCC13"), #0x50A
      ("SCC_Equip", "SCC13"),
      ("AebDrvSetStatus", "SCC13"),

      ("JerkUpperLimit", "SCC14"), #0x389
      ("JerkLowerLimit", "SCC14"),
      ("SCCMode2", "SCC14"),
      ("ComfortBandUpper", "SCC14"),
      ("ComfortBandLower", "SCC14"),

      ("UNIT", "TPMS11"),
      ("PRESSURE_FL", "TPMS11"),
      ("PRESSURE_FR", "TPMS11"),
      ("PRESSURE_RL", "TPMS11"),
      ("PRESSURE_RR", "TPMS11"),
    ]

    checks = [
      # address, frequency DBC정의된 것에 의한 작동
      ("TCS13", 50),  # 0d916 0x394
      ("TCS15", 10),  # 0d1287 0x507
      ("CLU11", 50),  # 0d1265 0x4F1
      ("ESP12", 100),  # 0d544 0x220
      ("CGW1", 10),  # 0d1345 0x541
      ("CGW2", 5), # 0d1363 0x553
      ("CGW4", 5),  # 0d1369 0x559
      ("WHL_SPD11", 50),  # 0d902 0x389
    ]

    if CP.sccBus == 0 and CP.pcmCruise:
      checks += [
        ("SCC11", 50),
        ("SCC12", 50),
      ]
    if CP.mdpsBus == 0: # 저속개조가 아닌 상태라면..
      signals += [
        ("CR_Mdps_StrColTq", "MDPS12"), #0x251 mdps관련신호 보내기
        ("CF_Mdps_Def", "MDPS12"),
        ("CF_Mdps_ToiActive", "MDPS12"),
        ("CF_Mdps_ToiUnavail", "MDPS12"),
        ("CF_Mdps_ToiFlt", "MDPS12"),
        ("CF_Mdps_MsgCount2", "MDPS12"),
        ("CF_Mdps_Chksum2", "MDPS12"),
        ("CF_Mdps_SErr", "MDPS12"),
        ("CR_Mdps_StrTq", "MDPS12"),
        ("CF_Mdps_FailStat", "MDPS12"),
        ("CR_Mdps_OutTq", "MDPS12")
      ]
      checks += [
        ("MDPS12", 50) # GDS 장비의 점검에서 점검시간에서 작동시 40ms시간으로 검사한다를 참조함..
      ]
    if CP.sasBus == 0:
      signals += [
        ("SAS_Angle", "SAS11"), #0x2B0 자동주차기능
        ("SAS_Speed", "SAS11"),
      ]
      checks += [
        ("SAS11", 100)
      ]
    if CP.sccBus == -1: # SCC가 없는차라면...
      signals += [
        ("CRUISE_LAMP_M", "EMS16"), #0x260 의 크루즈정보를 이용한다..
        ("CF_Lvr_CruiseSet", "LVR12"), #0x367의 TCU정보이용
    ]
    if CP.carFingerprint in FEATURES["use_cluster_gears"]: # 계기판의 기어정보로 인식하는 차량
      signals += [
        ("CF_Clu_Gear", "CLU15"),
      ]
    elif CP.carFingerprint in FEATURES["use_tcu_gears"]: # TCU의 기어레버정보등을 이용하는 경우
      signals += [
        ("CUR_GR", "TCU12"), #0x112 TCU의 기어정보를 얻는다..
      ]
    elif CP.carFingerprint in FEATURES["use_elect_gears"]: # 전자식기어 차량정보를 이용하는 경우
      signals += [
        ("Elect_Gear_Shifter", "ELECT_GEAR"), #0x372 기어레버모듈 정보
    ]
    else:
      signals += [
        ("CF_Lvr_Gear","LVR12"), # 기어레버의 정의가 없는 차량은 LVR12정보를 사용한다..
      ]

    if CP.carFingerprint in EV_HYBRID_CAR:
      if CP.carFingerprint in HYBRID_CAR:
        signals += [
          ("CR_Vcu_AccPedDep_Pos", "E_EMS11")
        ]
      else:
        signals += [
          ("Accel_Pedal_Pos", "E_EMS11"),
        ]
      checks += [
        ("E_EMS11", 50),
      ]

    else:
      signals += [
        ("PV_AV_CAN", "EMS12"), #0x329에서 브레이크스위치정보등을 읽을수 잇다..무슨정보지?
        ("CF_Ems_AclAct", "EMS16"), #0x52A에서 scc에서 나오는 정보를 얻는다..
      ]
      checks += [
        ("EMS12", 100),
        ("EMS16", 100),
      ]

    if CP.carFingerprint in FEATURES["use_fca"]:
      signals += [
        ("FCA_CmdAct", "FCA11"), #0x38D FCA11정보를 얻는데
        ("CF_VSM_Warn", "FCA11"),
      ]

      if not CP.openpilotLongitudinalControl:
        checks += [("FCA11", 50)]

    if CP.carFingerprint in [CAR.SANTA_FE, CAR.SANTA_FE_2022]:
      checks.remove(("TCS13", 50))

    if CP.enableBsm:
      signals += [
        ("CF_Lca_IndLeft", "LCA11"), #0x58B LCA정보...측후방같은 정보
        ("CF_Lca_IndRight", "LCA11"),
      ]
      checks += [("LCA11", 50)]

    if CP.enableAutoHold:
      signals += [
        ("AVH_STAT", "ESP11"),  # 교주님 오토홀드 기본 추가
        ("LDM_STAT", "ESP11"),  # HDA정보등
      ]
      checks += [("ESP11", 50)]

    return CANParser(DBC[CP.carFingerprint]["pt"], signals, checks, 0, enforce_checks=False)

  @staticmethod
  def get_can2_parser(CP):
    signals = []
    checks = []
    if CP.mdpsBus == 1: # MDPS가 저속개조 상태라면
      signals += [
        ("CR_Mdps_StrColTq", "MDPS12"), #0x251의 정보르를 읽는다..
        ("CF_Mdps_Def", "MDPS12"),
        ("CF_Mdps_ToiActive", "MDPS12"),
        ("CF_Mdps_ToiUnavail", "MDPS12"),
        ("CF_Mdps_ToiFlt", "MDPS12"),
        ("CF_Mdps_MsgCount2", "MDPS12"),
        ("CF_Mdps_Chksum2", "MDPS12"),
        ("CF_Mdps_SErr", "MDPS12"),
        ("CR_Mdps_StrTq", "MDPS12"),
        ("CF_Mdps_FailStat", "MDPS12"),
        ("CR_Mdps_OutTq", "MDPS12")
      ]
      checks += [
        ("MDPS12", 50)
      ]

    if CP.sasBus == 1:
      signals += [
        ("SAS_Angle", "SAS11"),
        ("SAS_Speed", "SAS11"),
      ]
      checks += [
        ("SAS11", 100)
      ]
    if CP.sccBus == 1: # 버스가 1인 경우...판다종류에 따라?
      signals += [
        ("MainMode_ACC", "SCC11"),
        ("SCCInfoDisplay", "SCC11"),
        ("AliveCounterACC", "SCC11"),
        ("VSetDis", "SCC11"),
        ("ObjValid", "SCC11"),
        ("DriverAlertDisplay", "SCC11"),
        ("TauGapSet", "SCC11"),
        ("ACC_ObjStatus", "SCC11"),
        ("ACC_ObjLatPos", "SCC11"),
        ("ACC_ObjDist", "SCC11"),
        ("ACC_ObjRelSpd", "SCC11"),
        ("Navi_SCC_Curve_Status", "SCC11"),
        ("Navi_SCC_Curve_Act", "SCC11"),
        ("Navi_SCC_Camera_Act", "SCC11"),
        ("Navi_SCC_Camera_Status", "SCC11"),


        ("ACCMode", "SCC12"),
        ("CF_VSM_Prefill", "SCC12"),
        ("CF_VSM_DecCmdAct", "SCC12"),
        ("CF_VSM_HBACmd", "SCC12"),
        ("CF_VSM_Warn", "SCC12"),
        ("CF_VSM_Stat", "SCC12"),
        ("CF_VSM_BeltCmd", "SCC12"),
        ("ACCFailInfo", "SCC12"),
        ("StopReq", "SCC12"),
        ("CR_VSM_DecCmd", "SCC12"),
        ("aReqRaw", "SCC12"), #aReqMax
        ("TakeOverReq", "SCC12"),
        ("PreFill", "SCC12"),
        ("aReqValue", "SCC12"), #aReqMin
        ("CF_VSM_ConfMode", "SCC12"),
        ("AEB_Failinfo", "SCC12"),
        ("AEB_Status", "SCC12"),
        ("AEB_CmdAct", "SCC12"),
        ("AEB_StopReq", "SCC12"),
        ("CR_VSM_Alive", "SCC12"),
        ("CR_VSM_ChkSum", "SCC12"),

        ("SCCDrvModeRValue", "SCC13"),
        ("SCC_Equip", "SCC13"),
        ("AebDrvSetStatus", "SCC13"),

        ("JerkUpperLimit", "SCC14"),
        ("JerkLowerLimit", "SCC14"),
        ("SCCMode2", "SCC14"),
        ("ComfortBandUpper", "SCC14"),
        ("ComfortBandLower", "SCC14"),

      ]
      checks += [
        ("SCC11", 50),
        ("SCC12", 50),
      ]
    return CANParser(DBC[CP.carFingerprint]["pt"], signals, checks, 1, enforce_checks=False)

  @staticmethod
  def get_cam_can_parser(CP):

    signals = [
      # sig_name, sig_address, default
      ("CF_Lkas_LdwsActivemode", "LKAS11"),
      ("CF_Lkas_LdwsSysState", "LKAS11"),
      ("CF_Lkas_SysWarning", "LKAS11"),
      ("CF_Lkas_LdwsLHWarning", "LKAS11"),
      ("CF_Lkas_LdwsRHWarning", "LKAS11"),
      ("CF_Lkas_HbaLamp", "LKAS11"),
      ("CF_Lkas_FcwBasReq", "LKAS11"),
      ("CF_Lkas_ToiFlt", "LKAS11"),
      ("CF_Lkas_HbaSysState", "LKAS11"),
      ("CF_Lkas_FcwOpt", "LKAS11"),
      ("CF_Lkas_HbaOpt", "LKAS11"),
      ("CF_Lkas_FcwSysState", "LKAS11"),
      ("CF_Lkas_FcwCollisionWarning", "LKAS11"),
      ("CF_Lkas_MsgCount", "LKAS11"),
      ("CF_Lkas_FusionState", "LKAS11"),
      ("CF_Lkas_FcwOpt_USM", "LKAS11"),
      ("CF_Lkas_LdwsOpt_USM", "LKAS11"),
    ]

    checks = [
      ("LKAS11", 100)
    ]
    if CP.sccBus == 2: # 풀컨트롤 개조상태
      signals += [
        ("MainMode_ACC", "SCC11"),
        ("SCCInfoDisplay", "SCC11"),
        ("AliveCounterACC", "SCC11"),
        ("VSetDis", "SCC11"),
        ("ObjValid", "SCC11"),
        ("DriverAlertDisplay", "SCC11"),
        ("TauGapSet", "SCC11"),
        ("ACC_ObjStatus", "SCC11"),
        ("ACC_ObjLatPos", "SCC11"),
        ("ACC_ObjDist", "SCC11"),
        ("ACC_ObjRelSpd", "SCC11"),
        ("Navi_SCC_Curve_Status", "SCC11"),
        ("Navi_SCC_Curve_Act", "SCC11"),
        ("Navi_SCC_Camera_Act", "SCC11"),
        ("Navi_SCC_Camera_Status", "SCC11"),

        ("ACCMode", "SCC12"),
        ("CF_VSM_Prefill", "SCC12"),
        ("CF_VSM_DecCmdAct", "SCC12"),
        ("CF_VSM_HBACmd", "SCC12"),
        ("CF_VSM_Warn", "SCC12"),
        ("CF_VSM_Stat", "SCC12"),
        ("CF_VSM_BeltCmd", "SCC12"),
        ("ACCFailInfo", "SCC12"),
        ("StopReq", "SCC12"),
        ("CR_VSM_DecCmd", "SCC12"),
        ("aReqRaw", "SCC12"), #aReqMax
        ("TakeOverReq", "SCC12"),
        ("PreFill", "SCC12"),
        ("aReqValue", "SCC12"), #aReqMin
        ("CF_VSM_ConfMode", "SCC12"),
        ("AEB_Failinfo", "SCC12"),
        ("AEB_Status", "SCC12"),
        ("AEB_CmdAct", "SCC12"),
        ("AEB_StopReq", "SCC12"),
        ("CR_VSM_Alive", "SCC12"),
        ("CR_VSM_ChkSum", "SCC12"),

        ("SCCDrvModeRValue", "SCC13"),
        ("SCC_Equip", "SCC13"),
        ("AebDrvSetStatus", "SCC13"),

        ("JerkUpperLimit", "SCC14"),
        ("JerkLowerLimit", "SCC14"),
        ("SCCMode2", "SCC14"),
        ("ComfortBandUpper", "SCC14"),
        ("ComfortBandLower", "SCC14"),
      ]
      checks += [
        ("SCC11", 50),
        ("SCC12", 50),
      ]

      if CP.hasLfaHda:
        signals += [
          ("HDA_USM", "LFAHDA_MFC"),
          ("HDA_Active", "LFAHDA_MFC"),
          ("HDA_Icon_State", "LFAHDA_MFC"),
          ("HDA_LdwSysState", "LFAHDA_MFC"),
          ("HDA_Icon_Wheel", "LFAHDA_MFC"),
        ]
        checks += [("LFAHDA_MFC", 20)]

    return CANParser(DBC[CP.carFingerprint]["pt"], signals, checks, 2, enforce_checks=False)
