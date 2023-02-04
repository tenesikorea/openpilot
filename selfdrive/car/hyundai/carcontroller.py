from random import randint
import time # 테네시 추가
from cereal import car
from common.realtime import DT_CTRL
from common.numpy_fast import clip, interp
from selfdrive.car import apply_std_steer_torque_limits
from selfdrive.car.hyundai.hyundaican import create_lkas11, create_clu11, \
  create_scc11, create_scc12, create_scc13, create_scc14, \
  create_mdps12, create_lfahda_mfc, create_hda_mfc
from selfdrive.car.hyundai.scc_smoother import SccSmoother
from selfdrive.car.hyundai.values import Buttons, CAR, FEATURES, CarControllerParams
from opendbc.can.packer import CANPacker
from common.conversions import Conversions as CV
from common.params import Params
from selfdrive.controls.lib.longcontrol import LongCtrlState
from selfdrive.road_speed_limiter import road_speed_limiter_get_active

VisualAlert = car.CarControl.HUDControl.VisualAlert
min_set_speed = 30 * CV.KPH_TO_MS

def process_hud_alert(enabled, fingerprint, hud_control):

  sys_warning = (hud_control.visualAlert in (VisualAlert.steerRequired, VisualAlert.ldw))

  # initialize to no line visible - 차로인식 관련 허드상에서 표시
  sys_state = 1 # 기본 수치는 1이다, 좌측차선인식 그리고 우측차선인식 그런데 실질 0x값에서는 0x4가 되고 그래서 0x2와 더해서 lkas대기는 0x6이 된다..
  if hud_control.leftLaneVisible and hud_control.rightLaneVisible or sys_warning:  # HUD alert only display when LKAS status is active
    sys_state = 3 if enabled or sys_warning else 4 # 3이면 0x0c이고 양쪽차로인식상태 + 0x02 0x0E이 나온다.LKAS작동기준,제네시스 DH기준 4는 0x12
  elif hud_control.leftLaneVisible:
    sys_state = 5 # 좌측차로인식 5이면 0x14 + 0x02 0x16이 나온다. 제네시스 DH기준
  elif hud_control.rightLaneVisible:
    sys_state = 6 # 우측차로인식 6이면 0x18 + 0x02 0x1A이 나온다. 제네시스 DH기준

  # initialize to no warnings
  left_lane_warning = 0
  right_lane_warning = 0
  if hud_control.leftLaneDepart:
    left_lane_warning = 1
  if hud_control.rightLaneDepart:
    right_lane_warning = 1

  return sys_warning, sys_state, left_lane_warning, right_lane_warning


class CarController:
  def __init__(self, dbc_name, CP, VM):
    self.car_fingerprint = CP.carFingerprint
    self.params = CarControllerParams(CP)
    self.packer = CANPacker(dbc_name)
    self.frame = 0

    self.apply_steer_last = 0
    self.accel = 0
    self.lkas11_cnt = 0
    self.scc12_cnt = -1

    self.resume_cnt = 0
    self.last_lead_distance = 0
    self.resume_wait_timer = 0

    self.turning_signal_timer = 0
    self.longcontrol = CP.openpilotLongitudinalControl
    self.scc_live = not CP.radarOffCan

    self.turning_indicator_alert = False

    param = Params()

    self.mad_mode_enabled = param.get_bool('MadModeEnabled')
    self.ldws_opt = param.get_bool('IsLdwsCar')
    self.stock_navi_decel_enabled = param.get_bool('StockNaviDecelEnabled')
    self.keep_steering_turn_signals = param.get_bool('KeepSteeringTurnSignals')
    self.haptic_feedback_speed_camera = param.get_bool('HapticFeedbackWhenSpeedCamera')

    self.tenesi_camera = Params().get_bool('TenesiCamera')  # NDA작동시 티맵 카메라 정보에 따른 선택사항
    self.lane_blink_on = False # NDA가 카메라 인식후 차로를 깜빡이게 하기

    self.scc_smoother = SccSmoother()
    self.last_blinker_frame = 0 # NDA가 카메라 인식후 차로를 깜빡이게 하기
    self.prev_active_cam = False
    self.active_cam_timer = 0
    self.last_active_cam_frame = 0

    self.angle_limit_counter = 0
    self.cut_steer_frames = 0
    self.cut_steer = False

    self.steer_fault_max_angle = CP.steerFaultMaxAngle
    self.steer_fault_max_frames = CP.steerFaultMaxFrames

  def update(self, CC, CS, controls):
    actuators = CC.actuators
    hud_control = CC.hudControl
    pcm_cancel_cmd = CC.cruiseControl.cancel

    # Steering Torque
    new_steer = int(round(actuators.steer * self.params.STEER_MAX))
    apply_steer = apply_std_steer_torque_limits(new_steer, self.apply_steer_last, CS.out.steeringTorque, self.params)

    # disable when temp fault is active, or below LKA minimum speed
    lkas_active = CC.latActive

    # Disable steering while turning blinker on and speed below 60 kph
    if CS.out.leftBlinker or CS.out.rightBlinker:
      self.turning_signal_timer = 0.5 / DT_CTRL  # Disable for 0.5 Seconds after blinker turned off
    if self.turning_indicator_alert: # set and clear by interface
      lkas_active = 0
    if self.turning_signal_timer > 0:
      self.turning_signal_timer -= 1

    if not lkas_active:
      apply_steer = 0

    self.apply_steer_last = apply_steer

    sys_warning, sys_state, left_lane_warning, right_lane_warning = process_hud_alert(CC.enabled, self.car_fingerprint, hud_control)

    if self.haptic_feedback_speed_camera:
      if self.prev_active_cam != self.scc_smoother.active_cam:
        self.prev_active_cam = self.scc_smoother.active_cam
        if self.scc_smoother.active_cam:
          if (self.frame - self.last_active_cam_frame) * DT_CTRL > 10.0:
            self.active_cam_timer = int(1.5 / DT_CTRL)
            self.last_active_cam_frame = self.frame

      if self.active_cam_timer > 0:
        self.active_cam_timer -= 1
        left_lane_warning = right_lane_warning = 1

    clu11_speed = CS.clu11["CF_Clu_Vanz"]
    enabled_speed = 38 if CS.is_set_speed_in_mph else 60
    if clu11_speed > enabled_speed or not lkas_active:
      enabled_speed = clu11_speed

    if self.frame == 0:  # initialize counts from last received count signals
      self.lkas11_cnt = CS.lkas11["CF_Lkas_MsgCount"]

    self.lkas11_cnt = (self.lkas11_cnt + 1) % 0x10

    cut_steer_temp = False

    if self.steer_fault_max_angle > 0:
      if lkas_active and abs(CS.out.steeringAngleDeg) > self.steer_fault_max_angle:
        self.angle_limit_counter += 1
      else:
        self.angle_limit_counter = 0

      # stop requesting torque to avoid 90 degree fault and hold torque with induced temporary fault
      # two cycles avoids race conditions every few minutes
      if self.angle_limit_counter > self.steer_fault_max_frames:
        self.cut_steer = True
      elif self.cut_steer_frames > 1:
        self.cut_steer_frames = 0
        self.cut_steer = False

      if self.cut_steer:
        cut_steer_temp = True
        self.angle_limit_counter = 0
        self.cut_steer_frames += 1

    hud_lane_warning = int(Params().get("TenesiCameraSelect", encoding="utf8"))  # 기본은 1 2 3 까지 선택적 표시
    if self.tenesi_camera:
      if self.scc_smoother.active_cam: # NDA가 카메라 인식후 차로를 깜빡이게 하기
        if self.frame % 50 == 0:
          self.lane_blink_on = not self.lane_blink_on
        left_lane_warning = right_lane_warning = hud_lane_warning # 1을 넣으면 핸들진동 기능과 함께 깜빡임이 된다.. 2는 차로 소리가 나온다.(계기판 동시) 3은 허드에서만 표시가 나온다..
      else:
        self.lane_blink_on = False # NDA가 카메라 인식후 차로를 깜빡이게 하기

    hud_brake_warning = int(Params().get("TenesiBrakeSelect", encoding="utf8")) # 기본은 1 2 3 까지 선택적 표시
    if CS.currentBrake and CS.out.vEgo > 3 and not tmap_camera: # 티맵카메라작동시 작동안함..
      sys_state = 4 # 핸들모양이 허드에 안뜨게 함..
      left_lane_warning = right_lane_warning = hud_brake_warning # 차로 색상및 계기판 진동표시나 소리표시등을 선택적으로 표시가능

    can_sends = []
    can_sends.append(create_lkas11(self.packer, self.frame, self.car_fingerprint, apply_steer, lkas_active,
                                   CS.lkas11, sys_warning, sys_state, CC.enabled, hud_control.leftLaneVisible, hud_control.rightLaneVisible,
                                   left_lane_warning, right_lane_warning, 0, self.ldws_opt, cut_steer_temp))

    if CS.mdps_bus or CS.scc_bus == 1:  # send lkas11 bus 1 if mdps or scc is on bus 1
      can_sends.append(create_lkas11(self.packer, self.frame, self.car_fingerprint, apply_steer, lkas_active,
                                     CS.lkas11, sys_warning, sys_state, CC.enabled, hud_control.leftLaneVisible, hud_control.rightLaneVisible,
                                     left_lane_warning, right_lane_warning, 1, self.ldws_opt, cut_steer_temp))

    if self.frame % 2 and CS.mdps_bus: # send clu11 to mdps if it is not on bus 0
      can_sends.append(create_clu11(self.packer, CS.mdps_bus, CS.clu11, Buttons.NONE, enabled_speed))

    if pcm_cancel_cmd and (self.longcontrol and not self.mad_mode_enabled):
      can_sends.append(create_clu11(self.packer, CS.scc_bus, CS.clu11, Buttons.CANCEL, clu11_speed))

    if CS.mdps_bus or self.car_fingerprint in FEATURES["send_mdps12"]:  # send mdps12 to LKAS to prevent LKAS error
      can_sends.append(create_mdps12(self.packer, self.frame, CS.mdps12))

    self.update_auto_resume(CC, CS, clu11_speed, can_sends)
    self.update_scc(CC, CS, actuators, controls, hud_control, can_sends)

    # 20 Hz LFA MFA message
    if self.frame % 5 == 0:
      activated_hda = road_speed_limiter_get_active()
      # activated_hda: 0 - off, 1 - main road, 2 - highway
      if self.car_fingerprint in FEATURES["send_lfa_mfa"]:
        can_sends.append(create_lfahda_mfc(self.packer, CC.enabled, activated_hda))
      elif CS.has_lfa_hda:
        can_sends.append(create_hda_mfc(self.packer, activated_hda, CS, hud_control.leftLaneVisible, hud_control.rightLaneVisible))

    new_actuators = actuators.copy()
    new_actuators.steer = apply_steer / self.params.STEER_MAX
    new_actuators.accel = self.accel

    self.frame += 1
    return new_actuators, can_sends

  def update_auto_resume(self, CC, CS, clu11_speed, can_sends):
    # fix auto resume - by neokii
    if CS.out.cruiseState.standstill and not CS.out.gasPressed:
      if self.last_lead_distance == 0:
        self.last_lead_distance = CS.lead_distance
        self.resume_cnt = 0
        self.resume_wait_timer = 0

      elif self.scc_smoother.is_active(self.frame):
        pass

      elif self.resume_wait_timer > 0:
        self.resume_wait_timer -= 1

      elif abs(CS.lead_distance - self.last_lead_distance) > 0.1:
        can_sends.append(create_clu11(self.packer, CS.scc_bus, CS.clu11, Buttons.RES_ACCEL, clu11_speed))
        self.resume_cnt += 1

        if self.resume_cnt >= randint(6, 8):
          self.resume_cnt = 0
          self.resume_wait_timer = randint(30, 36)

    elif self.last_lead_distance != 0:
      self.last_lead_distance = 0

  def update_scc(self, CC, CS, actuators, controls, hud_control, can_sends):

    # scc smoother
    self.scc_smoother.update(CC.enabled, can_sends, self.packer, CC, CS, self.frame, controls)

    # send scc to car if longcontrol enabled and SCC not on bus 0 or ont live
    if self.longcontrol and CS.cruiseState_enabled and (CS.scc_bus or not self.scc_live):

      if self.frame % 2 == 0:

        set_speed = hud_control.setSpeed
        if not (min_set_speed < set_speed < 255 * CV.KPH_TO_MS):
          set_speed = min_set_speed
        set_speed *= CV.MS_TO_MPH if CS.is_set_speed_in_mph else CV.MS_TO_KPH

        stopping = controls.LoC.long_control_state == LongCtrlState.stopping
        apply_accel = self.scc_smoother.get_apply_accel(CS, controls.sm, actuators.accel, stopping)
        apply_accel = clip(apply_accel if CC.longActive else 0,
                           CarControllerParams.ACCEL_MIN, CarControllerParams.ACCEL_MAX)

        self.accel = apply_accel

        controls.apply_accel = apply_accel
        aReqValue = CS.scc12["aReqValue"]
        controls.aReqValue = aReqValue

        if aReqValue < controls.aReqValueMin:
          controls.aReqValueMin = controls.aReqValue

        if aReqValue > controls.aReqValueMax:
          controls.aReqValueMax = controls.aReqValue

        if self.stock_navi_decel_enabled:
          controls.sccStockCamAct = CS.scc11["Navi_SCC_Camera_Act"]
          controls.sccStockCamStatus = CS.scc11["Navi_SCC_Camera_Status"]
          apply_accel, stock_cam = self.scc_smoother.get_stock_cam_accel(apply_accel, aReqValue, CS.scc11)
        else:
          controls.sccStockCamAct = 0
          controls.sccStockCamStatus = 0
          stock_cam = False

        if self.scc12_cnt < 0:
          self.scc12_cnt = CS.scc12["CR_VSM_Alive"] if not CS.no_radar else 0

        self.scc12_cnt += 1
        self.scc12_cnt %= 0xF

        can_sends.append(create_scc12(self.packer, apply_accel, CC.enabled, self.scc12_cnt, self.scc_live, CS.scc12,
                                      CS.out.gasPressed, CS.out.brakePressed, CS.out.cruiseState.standstill,
                                      self.car_fingerprint))

        can_sends.append(create_scc11(self.packer, self.frame, CC.enabled, set_speed, hud_control.leadVisible, self.scc_live, CS.scc11,
                       self.scc_smoother.active_cam, stock_cam))

        if self.frame % 20 == 0 and CS.has_scc13:
          can_sends.append(create_scc13(self.packer, CS.scc13))

        if CS.has_scc14:
          acc_standstill = stopping if CS.out.vEgo < 2. else False

          lead = self.scc_smoother.get_lead(controls.sm)

          if lead is not None:
            d = lead.dRel
            obj_gap = 1 if d < 25 else 2 if d < 40 else 3 if d < 60 else 4 if d < 80 else 5
          else:
            obj_gap = 0

          can_sends.append(
            create_scc14(self.packer, CC.enabled, CS.out.vEgo, acc_standstill, apply_accel, CS.out.gasPressed,
                         obj_gap, CS.scc14))
    else:
      self.scc12_cnt = -1