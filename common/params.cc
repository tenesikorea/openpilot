#include "common/params.h"

#include <dirent.h>
#include <sys/file.h>

#include <algorithm>
#include <csignal>
#include <unordered_map>

#include "common/swaglog.h"
#include "common/util.h"
#include "system/hardware/hw.h"

namespace {

volatile sig_atomic_t params_do_exit = 0;
void params_sig_handler(int signal) {
  params_do_exit = 1;
}

int fsync_dir(const std::string &path) {
  int result = -1;
  int fd = HANDLE_EINTR(open(path.c_str(), O_RDONLY, 0755));
  if (fd >= 0) {
    result = fsync(fd);
    close(fd);
  }
  return result;
}

bool create_params_path(const std::string &param_path, const std::string &key_path) {
  // Make sure params path exists
  if (!util::file_exists(param_path) && !util::create_directories(param_path, 0775)) {
    return false;
  }

  // See if the symlink exists, otherwise create it
  if (!util::file_exists(key_path)) {
    // 1) Create temp folder
    // 2) Symlink it to temp link
    // 3) Move symlink to <params>/d

    std::string tmp_path = param_path + "/.tmp_XXXXXX";
    // this should be OK since mkdtemp just replaces characters in place
    char *tmp_dir = mkdtemp((char *)tmp_path.c_str());
    if (tmp_dir == NULL) {
      return false;
    }

    std::string link_path = std::string(tmp_dir) + ".link";
    if (symlink(tmp_dir, link_path.c_str()) != 0) {
      return false;
    }

    // don't return false if it has been created by other
    if (rename(link_path.c_str(), key_path.c_str()) != 0 && errno != EEXIST) {
      return false;
    }
  }

  return true;
}

std::string ensure_params_path(const std::string &prefix, const std::string &path = {}) {
  std::string params_path = path.empty() ? Path::params() : path;
  if (!create_params_path(params_path, params_path + prefix)) {
    throw std::runtime_error(util::string_format("Failed to ensure params path, errno=%d", errno));
  }
  return params_path;
}

class FileLock {
public:
  FileLock(const std::string &fn) {
    fd_ = HANDLE_EINTR(open(fn.c_str(), O_CREAT, 0775));
    if (fd_ < 0 || HANDLE_EINTR(flock(fd_, LOCK_EX)) < 0) {
      LOGE("Failed to lock file %s, errno=%d", fn.c_str(), errno);
    }
  }
  ~FileLock() { close(fd_); }

private:
  int fd_ = -1;
};

std::unordered_map<std::string, uint32_t> keys = {
    {"AccessToken", CLEAR_ON_MANAGER_START | DONT_LOG},
    {"AssistNowToken", PERSISTENT},
    {"AthenadPid", PERSISTENT},
    {"AthenadUploadQueue", PERSISTENT},
    {"CalibrationParams", PERSISTENT},
    {"CameraDebugExpGain", CLEAR_ON_MANAGER_START},
    {"CameraDebugExpTime", CLEAR_ON_MANAGER_START},
    {"CarBatteryCapacity", PERSISTENT},
    {"CarParams", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"CarParamsCache", CLEAR_ON_MANAGER_START},
    {"CarParamsPersistent", PERSISTENT},
    {"CarVin", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"CompletedTrainingVersion", PERSISTENT},
    {"ControlsReady", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"CurrentBootlog", PERSISTENT},
    {"CurrentRoute", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"DisableLogging", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"DisablePowerDown", PERSISTENT},
    {"ExperimentalMode", PERSISTENT},
    {"ExperimentalModeConfirmed", PERSISTENT},
    {"ExperimentalLongitudinalEnabled", PERSISTENT},
    {"DisableUpdates", PERSISTENT},
    {"DisengageOnAccelerator", PERSISTENT},
    {"DongleId", PERSISTENT},
    {"DoReboot", CLEAR_ON_MANAGER_START},
    {"DoShutdown", CLEAR_ON_MANAGER_START},
    {"DoUninstall", CLEAR_ON_MANAGER_START},
    {"FirmwareQueryDone", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"ForcePowerDown", CLEAR_ON_MANAGER_START},
    {"GitBranch", PERSISTENT},
    {"GitCommit", PERSISTENT},
    {"GitDiff", PERSISTENT},
    {"GithubSshKeys", PERSISTENT},
    {"GithubUsername", PERSISTENT},
    {"GitRemote", PERSISTENT},
    {"GsmApn", PERSISTENT},
    {"GsmMetered", PERSISTENT},
    {"GsmRoaming", PERSISTENT},
    {"HardwareSerial", PERSISTENT},
    {"HasAcceptedTerms", PERSISTENT},
    {"IMEI", PERSISTENT},
    {"InstallDate", PERSISTENT},
    {"IsDriverViewEnabled", CLEAR_ON_MANAGER_START},
    {"IsEngaged", PERSISTENT},
    {"IsLdwEnabled", PERSISTENT},
    {"IsMetric", PERSISTENT},
    {"IsOffroad", CLEAR_ON_MANAGER_START},
    {"IsOnroad", PERSISTENT},
    {"IsRhdDetected", PERSISTENT},
    {"IsTakingSnapshot", CLEAR_ON_MANAGER_START},
    {"IsTestedBranch", CLEAR_ON_MANAGER_START},
    {"IsReleaseBranch", CLEAR_ON_MANAGER_START},
    {"IsUpdateAvailable", CLEAR_ON_MANAGER_START},
    {"JoystickDebugMode", CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION},
    {"LaikadEphemerisV3", PERSISTENT | DONT_LOG},
    {"LanguageSetting", PERSISTENT},
    {"LastAthenaPingTime", CLEAR_ON_MANAGER_START},
    {"LastGPSPosition", PERSISTENT},
    {"LastManagerExitReason", CLEAR_ON_MANAGER_START},
    {"LastPowerDropDetected", CLEAR_ON_MANAGER_START},
    {"LastSystemShutdown", CLEAR_ON_MANAGER_START},
    {"LastUpdateException", CLEAR_ON_MANAGER_START},
    {"LastUpdateTime", PERSISTENT},
    {"LiveParameters", PERSISTENT},
    {"LiveTorqueCarParams", PERSISTENT},
    {"LiveTorqueParameters", PERSISTENT | DONT_LOG},
    {"NavDestination", CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION},
    {"NavDestinationWaypoints", CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION},
    {"NavSettingTime24h", PERSISTENT},
    {"NavSettingLeftSide", PERSISTENT},
    {"NavdRender", PERSISTENT},
    {"ObdMultiplexingChanged", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"ObdMultiplexingEnabled", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"OpenpilotEnabledToggle", PERSISTENT},
    {"PandaHeartbeatLost", CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION},
    {"PandaSignatures", CLEAR_ON_MANAGER_START},
    {"Passive", PERSISTENT},
    {"PrimeType", PERSISTENT},
    {"RecordFront", PERSISTENT},
    {"RecordFrontLock", PERSISTENT},  // for the internal fleet
    {"ReplayControlsState", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"ShouldDoUpdate", CLEAR_ON_MANAGER_START},
    {"SnoozeUpdate", CLEAR_ON_MANAGER_START | CLEAR_ON_OFFROAD_TRANSITION},
    {"SshEnabled", PERSISTENT},
    {"SubscriberInfo", PERSISTENT},
    {"TermsVersion", PERSISTENT},
    {"Timezone", PERSISTENT},
    {"TrainingVersion", PERSISTENT},
    {"UbloxAvailable", PERSISTENT},
    {"UpdateAvailable", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"UpdateFailedCount", CLEAR_ON_MANAGER_START},
    {"UpdaterState", CLEAR_ON_MANAGER_START},
    {"UpdaterFetchAvailable", CLEAR_ON_MANAGER_START},
    {"UpdaterTargetBranch", CLEAR_ON_MANAGER_START},
    {"UpdaterAvailableBranches", CLEAR_ON_MANAGER_START},
    {"UpdaterCurrentDescription", CLEAR_ON_MANAGER_START},
    {"UpdaterCurrentReleaseNotes", CLEAR_ON_MANAGER_START},
    {"UpdaterNewDescription", CLEAR_ON_MANAGER_START},
    {"UpdaterNewReleaseNotes", CLEAR_ON_MANAGER_START},
    {"Version", PERSISTENT},
    {"VisionRadarToggle", PERSISTENT},
    {"WideCameraOnly", PERSISTENT},
    {"ApiCache_Device", PERSISTENT},
    {"ApiCache_DriveStats", PERSISTENT},
    {"ApiCache_NavDestinations", PERSISTENT},
    {"ApiCache_Owner", PERSISTENT},
    {"Offroad_BadNvme", CLEAR_ON_MANAGER_START},
    {"Offroad_CarUnrecognized", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"Offroad_ConnectivityNeeded", CLEAR_ON_MANAGER_START},
    {"Offroad_ConnectivityNeededPrompt", CLEAR_ON_MANAGER_START},
    {"Offroad_InvalidTime", CLEAR_ON_MANAGER_START},
    {"Offroad_IsTakingSnapshot", CLEAR_ON_MANAGER_START},
    {"Offroad_NeosUpdate", CLEAR_ON_MANAGER_START},
    {"Offroad_NoFirmware", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"Offroad_StorageMissing", CLEAR_ON_MANAGER_START},
    {"Offroad_TemperatureTooHigh", CLEAR_ON_MANAGER_START},
    {"Offroad_UnofficialHardware", CLEAR_ON_MANAGER_START},
    {"Offroad_UpdateFailed", CLEAR_ON_MANAGER_START},
    {"Offroad_Recalibration", CLEAR_ON_MANAGER_START | CLEAR_ON_ONROAD_TRANSITION},
    {"MapboxToken", PERSISTENT},

    // Community
    {"SupportedCars", PERSISTENT},
    {"SelectedCar", PERSISTENT},
    {"UseLanelines", PERSISTENT},
    {"SccOnBus2", PERSISTENT},
    {"CruiseStateControl", PERSISTENT},
    {"SccGapAdjust", PERSISTENT},

    {"IsLdwsCar", PERSISTENT},
    {"HapticFeedbackWhenSpeedCamera", PERSISTENT},
    {"ShowDebugMessage", PERSISTENT},

    {"SoftRestartTriggered", CLEAR_ON_MANAGER_START},
// 테네시추가작업
    {"TenesiSSHLegacy", PERSISTENT},// eonley.ppk 공용키 사용하기 위해서 추가
    {"TenesiCamera", PERSISTENT}, // NDA작동시 티맵 카메라 정보에 따른 선택사항
    {"TenesiCameraSelect", PERSISTENT}, // NDA작동시 티맵 카메라 정보에 따른 허드표시 선택
    {"Boost_s", PERSISTENT}, // 정지후 출발시 악셀 부스트값
    {"Creep_Speed_Start", PERSISTENT}, // 악셀 부스트 적용 영역 지정- 정지후 적용키로
    {"Boost_v", PERSISTENT}, // 주행시 가속 부트스값
    {"Creep_Speed_Scc", PERSISTENT}, // 악셀 부스트 적용 영역 지정- SCC 속도범위지정
    {"Auto_engage", PERSISTENT}, // 자동인게이지 활성화 속도
    {"AutoSetOpt", PERSISTENT}, // 장푸 오토 크루즈셋 활용하기 위한 셋딩
    {"ShowTpmsUI", PERSISTENT}, // 타이어 공기압
    {"ShowCgearUI", PERSISTENT}, // 기어단수
    {"SoundAutoHold", PERSISTENT}, // 오토홀드 음성 온 오프
    {"SoundBsd", PERSISTENT}, // BSD 음성 온 오프
    {"PutPrebuiltOn", PERSISTENT}, //프리빌트 온오프
    {"ShowBsdUI", PERSISTENT},// bsd 보기옵션
    {"ShowBrakeUI", PERSISTENT},// 기본정보 보기옵션
    {"ShowErpmUI", PERSISTENT},// 엔진 RPM보기
    {"T_Debug", PERSISTENT},// tmux a 저장용 옵션
    {"Zorrobyte", PERSISTENT},// 조로바이트 쪽 코드 선택
    {"Neokiibyte", PERSISTENT},// 교주 쪽 코드 선택
    {"Steer_90D", PERSISTENT},//90도 조향 옵션화
    {"Steer_SRTune", PERSISTENT},//속도대응 고정SR값 사용하기
    {"Sound_Slow", PERSISTENT}, // TAMP자동감속 음성 온 오프
    {"Sound_Start", PERSISTENT}, // 펭수 시작음성 듣기 안든기
    {"Steer_409", PERSISTENT}, // 제네시스 조향 토크 409 선택 온셥
    {"Boost_s", PERSISTENT}, // 정지후 출발시 악셀 부스트값
    {"Boost_v", PERSISTENT}, // 주행시 가속 부트스값
    {"Steer_SRTune_v", PERSISTENT}, // Steer_SRTune 선택시 가변SR의 비율값
    {"Lane_Change", PERSISTENT}, // 차로 변경 가능한 속도 설정
    {"Auto_engage", PERSISTENT}, // 자동인게이지 활성화 속도
    {"Creep_Speed_Start", PERSISTENT}, // 악셀 부스트 적용 영역 지정- 정지후 적용키로
    {"Creep_Speed_Scc", PERSISTENT}, // 악셀 부스트 적용 영역 지정- SCC 속도범위지정
    {"Sound_Mdpserr", PERSISTENT}, // mdps에러시 음성듣기
    {"TenesiBrakeSelect", PERSISTENT}, // NDA작동시 티맵 카메라 정보에 따른 허드표시 선택
    {"TenesiSccStopCost", PERSISTENT}, // vEgoStopping 기본값 0.3를 변경하기..
    {"Tenesi_Gas_Factor", PERSISTENT}, // 차로 변경 가능한 속도 설정

};

} // namespace


Params::Params(const std::string &path) {
  prefix = "/" + util::getenv("OPENPILOT_PREFIX", "d");
  params_path = ensure_params_path(prefix, path);
}

std::vector<std::string> Params::allKeys() const {
  std::vector<std::string> ret;
  for (auto &p : keys) {
    ret.push_back(p.first);
  }
  return ret;
}

bool Params::checkKey(const std::string &key) {
  return keys.find(key) != keys.end();
}

ParamKeyType Params::getKeyType(const std::string &key) {
  return static_cast<ParamKeyType>(keys[key]);
}

int Params::put(const char* key, const char* value, size_t value_size) {
  // Information about safely and atomically writing a file: https://lwn.net/Articles/457667/
  // 1) Create temp file
  // 2) Write data to temp file
  // 3) fsync() the temp file
  // 4) rename the temp file to the real name
  // 5) fsync() the containing directory
  std::string tmp_path = params_path + "/.tmp_value_XXXXXX";
  int tmp_fd = mkstemp((char*)tmp_path.c_str());
  if (tmp_fd < 0) return -1;

  int result = -1;
  do {
    // Write value to temp.
    ssize_t bytes_written = HANDLE_EINTR(write(tmp_fd, value, value_size));
    if (bytes_written < 0 || (size_t)bytes_written != value_size) {
      result = -20;
      break;
    }

    // fsync to force persist the changes.
    if ((result = fsync(tmp_fd)) < 0) break;

    FileLock file_lock(params_path + "/.lock");

    // Move temp into place.
    if ((result = rename(tmp_path.c_str(), getParamPath(key).c_str())) < 0) break;

    // fsync parent directory
    result = fsync_dir(getParamPath());
  } while (false);

  close(tmp_fd);
  ::unlink(tmp_path.c_str());
  return result;
}

int Params::remove(const std::string &key) {
  FileLock file_lock(params_path + "/.lock");
  int result = unlink(getParamPath(key).c_str());
  if (result != 0) {
    return result;
  }
  return fsync_dir(getParamPath());
}

std::string Params::get(const std::string &key, bool block) {
  if (!block) {
    return util::read_file(getParamPath(key));
  } else {
    // blocking read until successful
    params_do_exit = 0;
    void (*prev_handler_sigint)(int) = std::signal(SIGINT, params_sig_handler);
    void (*prev_handler_sigterm)(int) = std::signal(SIGTERM, params_sig_handler);

    std::string value;
    while (!params_do_exit) {
      if (value = util::read_file(getParamPath(key)); !value.empty()) {
        break;
      }
      util::sleep_for(100);  // 0.1 s
    }

    std::signal(SIGINT, prev_handler_sigint);
    std::signal(SIGTERM, prev_handler_sigterm);
    return value;
  }
}

std::map<std::string, std::string> Params::readAll() {
  FileLock file_lock(params_path + "/.lock");
  return util::read_files_in_dir(getParamPath());
}

void Params::clearAll(ParamKeyType key_type) {
  FileLock file_lock(params_path + "/.lock");

  // 1) delete params of key_type
  // 2) delete files that are not defined in the keys.
  if (DIR *d = opendir(getParamPath().c_str())) {
    struct dirent *de = NULL;
    while ((de = readdir(d))) {
      if (de->d_type != DT_DIR) {
        auto it = keys.find(de->d_name);
        if (it == keys.end() || (it->second & key_type)) {
          unlink(getParamPath(de->d_name).c_str());
        }
      }
    }
    closedir(d);
  }

  fsync_dir(getParamPath());
}
