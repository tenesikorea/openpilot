# TenesiOP

테네시 오픈 파일럿 도움말 - 브랜치의 포크는 NeoKii 교주사마님의 기본 포크를 기반으로 합니다..

23년 02월 04일 업데이트 내용 일부 설명...
[![](http://drive.google.com/uc?export=view&id=1hL0i6yiO2y5guzUY43wJ7vcj3V_1NFIk)](#)
- Prebuilt 파일생성 - 이건 모든 셋팅이 완료되고 빠른 부팅을 할때에 사용됩니다..주 기능은 빌드작업을 생략 개념이고 일부 강제 빌드는 무조건 빌드합니다..
  만약에 깃풀을 할때에 이부분이 켜져있고 깃풀을 하고 재부팅시에는 블랙스크린이 나올수 있으니 필수로 해제하세요!
  그리고 절차상 프리빌트해제및 깃리셋후에 깃풀을 추천합니다.. 재부팅이 정상일때에 다시 해제해서 적용이 가능하고
  만약에 블랙스크린등으로 부팅이 문제가 있으면 Ntune으로 프리빌트 삭제후에 부팅하면 됩니다..
  -(github.com/openpilotusers 에서 힌트를 얻어서 작성)
  
- GPS 오프  - 이건 GPS장치오류로 자동종료가 안되는 경우에 사용합니다.. 대표적으로 화이트 판다 사용자의 경우에 자동종료가 안됩니다..
   필수로 쓰셔야 합니다..
  -(github.com/openpilotusers 에서 힌트를 얻어서 작성)
  
- GitPull 관련 명령어를 내장...
  - 되도록 쓰지마시고.. 제 브랜치를 꾸준히 사용한다면 커밋(로컬/리모트)항목중 리모트가 파란색일때 또는 임의로 가능합니다..
    문제는 테스트중일때 꼬이면 EON폰도 꼬입니다.. 문의하고 하시길 추천합니다..주의 할점은 일부 반영이 안될수 있는데
    깃리셋후 깃풀을 추천합니다..특히나 종료시간을 임의 설정하면 반영이 안되기도 합니다..
  -(github.com/openpilotusers 에서 힌트를 얻어서 작성)
   
Git Pull 명령어 작업시에 SSL 에러 표시가 나오면서 업데이트가 안될때! 
  -> SSH 터미널에 접속해서 export GIT_SSL_NO_VERIFY=0 실행한다..

SSH터미널에서 VI,VIM명령어로 화일 내용을 수정하는 명령.. 
주요 명령 : 
  현재 편집 화일에서 나가기 => :q (키보드의 클론을 누르고 영어 q를 누른다 : 이걸 누른후에는 명령어 입력한다는 의미) 
  현재 편집 화일 저장하기 => :w (:wq 라면 저장하고 나가기가 된다)
  현재 편집 화일 저장안하고 나가기 => :qi 편집실수등을 보완..

  현재 커서위치에서 키보드의 i를 누르면 => 하단부에 insert가 나오면서 추가 모드가 된다.. ESC 누르면 취소 
  현재 커서위치에서 키보드의 r를 누르면 => 하단부에 아무것도 안나오고 입려되는 문자를 입력해서 바뀐다.. 
  현재 커서위치에서 키보드의 dd를 누르면 => 해당줄을 모두 삭제한다.

키보드 u를 누르면 입력해서 수정한것을 취소해서 원래되로 되돌린다...

운전 종료하고 이온폰이 자동으로 꺼지는 시간 설정하기.. 
  터미널 접속후
  
  vim /data/openpilot/selfdrive/shutdownd.py
  60초 * 1.00 은 60초로 1분후 종료.. 수정후 :wq

이온 화면을 강제로 어둡게 하거나 밝게 하기.. 
  터미널 접속후
  
  vim /data/openpilot/selfdrive/hardware/eon/hardware.h 
  
  //percent = 15;
  주석처리를 할경우에 (자동모드로 변경할경우) //를 넣어준다..  
  수정하고 :wq 
  
  percent = 15; // 배터리리스-다이오드방식에서 화면 밝기 적게 해보기

100퍼센트 밝기중에서 15프로 밝기로 어둡게 설정된 상태.. 수정하고 :wq

-------------------210705일자로 화이트 판다는 선택사항으로 적용-----------------------------------
화이트 판다의 경우 자동 종료가 안될때에..
vim /data/openpilot/selfdrive/boardd/boardd.cc 
화일을 열어서 제일 하단부에
	  //if(panda->is_pigeon)  // 화이트(비둘기 판다) 용
	      //threads.push_back(std::thread(pigeon_thread));  // 화이트(비둘기 판다) 용
주석처리를 하세요... 

상기 모든 적용사양을 재부팅을 해야 적용됩니다.. 음성 사운드 관련 설정등도 재부팅해야 적용됩니다..

--------------------------------------------------------------------------------------------
- TWRP 및 이온접속 - 공부 자료 동영상 - 초창기 eon.ppk를 활용해서 접속 가능한 동영상 
   https://drive.google.com/file/d/12uGwVXZkMyyDWaF0heV_ouDb8PyqBedU/view?usp=sharing
   https://drive.google.com/file/d/1Lp_Y5vGeG-R4KqMHeQrinaax4r-VBguS/view?usp=sharing
   https://drive.google.com/file/d/143vkDqCB99fjvmqJm3OjxE9PLiB1ksC-/view?usp=sharing
   https://drive.google.com/file/d/1TeVzSJra-ZySX7Ea70GMH6ER9NndQ_ca/view?usp=sharing
-------------------------------------------------------------------------------------------- 
- 안드로이드 폰에서 엔튠과 터보클라이언트 관련해서 등록 하는법등 동영상
https://drive.google.com/file/d/1UmV6d-_Uqim-lml2WIHSZReWLaPGk8mn/view?usp=sharing
------21년 6월 10일 초보와 고수도 봐야 하는 오픈파일럿 셋팅 관련된 내용 저장-------

다운로드 링크 
- 테네시 파일럿 전용 SSH접속시 필요한 ppk 화일 다운로드 경로 - 암호로 다운로드 가능함..
http://naver.me/xv0Ig0d5 
- Ntune 앱설치 다운로드.. 
https://drive.google.com/file/d/1-EPga-ukGl9p4SsTMCKfJs6zrZpZ6dqC/view?usp=sharing 
- 터보클라이언트로 SSH터미널이 아닌 상태에서 접속해서 수정이 가능한 앱으로 추천!  
https://play.google.com/store/apps/details?id=turbo.client 
- ssh 터미널앱으로 vim으로 편집가능한 앱이며 되도록 필수로 설치를 요하는 앱.. 특히나 ssl에러시에 필수다..
https://play.google.com/store/apps/details?id=com.sonelli.juicessh 
- 퍼티 64비트 PC용
https://drive.google.com/file/d/1slJ-aa5CLzwbFc3fPXk0nypgFKxUWUQr/view?usp=sharing
- 퍼티 32비트 PC용
https://drive.google.com/file/d/1yCZpOe12nrI2uHG9pY6S5pm87yv9RwpC/view?usp=sharing
- winscp PC용
https://drive.google.com/file/d/1rYHd_vI5KnaFQgQjweTHYYHRxqZXwbFK/view?usp=sharing


**이 브랜치는 콤마3 전용입니다.
콤마2/이온 은 hkg_0814_c2 브랜치를 이용하세요.**

**This branch is comma3 only.
For comma2/eon, use hkg_0814_c2.**


**This is based on xx979xx for Hyundai, Kia, Genesis.**

https://github.com/xx979xx/openpilot

It is open source and inherits MIT license.  By installing this software you accept all responsibility for anything that might occur while you use it.  All contributors to this fork are not liable.  <b>Use at your own risk.</b>


------------------------------------------------------


![](https://i.imgur.com/b0ZyIx5.jpg)

Table of Contents
=======================

* [What is openpilot?](#what-is-openpilot)
* [Running in a car](#running-in-a-car)
* [Running on PC](#running-on-pc)
* [Community and Contributing](#community-and-contributing)
* [User Data and comma Account](#user-data-and-comma-account)
* [Safety and Testing](#safety-and-testing)
* [Directory Structure](#directory-structure)
* [Licensing](#licensing)

---

What is openpilot?
------

[openpilot](http://github.com/commaai/openpilot) is an open source driver assistance system. Currently, openpilot performs the functions of Adaptive Cruise Control (ACC), Automated Lane Centering (ALC), Forward Collision Warning (FCW) and Lane Departure Warning (LDW) for a growing variety of [supported car makes, models and model years](docs/CARS.md). In addition, while openpilot is engaged, a camera based Driver Monitoring (DM) feature alerts distracted and asleep drivers. See more about [the vehicle integration](docs/INTEGRATION.md) and [limitations](docs/LIMITATIONS.md).

<table>
  <tr>
    <td><a href="https://youtu.be/NmBfgOanCyk" title="Video By Greer Viau"><img src="https://i.imgur.com/1w8c6d2.jpg"></a></td>
    <td><a href="https://youtu.be/VHKyqZ7t8Gw" title="Video By Logan LeGrand"><img src="https://i.imgur.com/LnBucik.jpg"></a></td>
    <td><a href="https://youtu.be/VxiR4iyBruo" title="Video By Charlie Kim"><img src="https://i.imgur.com/4Qoy48c.jpg"></a></td>
    <td><a href="https://youtu.be/-IkImTe1NYE" title="Video By Aragon"><img src="https://i.imgur.com/04VNzPf.jpg"></a></td>
  </tr>
  <tr>
    <td><a href="https://youtu.be/iIUICQkdwFQ" title="Video By Logan LeGrand"><img src="https://i.imgur.com/b1LHQTy.jpg"></a></td>
    <td><a href="https://youtu.be/XOsa0FsVIsg" title="Video By PinoyDrives"><img src="https://i.imgur.com/6FG0Bd8.jpg"></a></td>
    <td><a href="https://youtu.be/bCwcJ98R_Xw" title="Video By JS"><img src="https://i.imgur.com/zO18CbW.jpg"></a></td>
    <td><a href="https://youtu.be/BQ0tF3MTyyc" title="Video By Tsai-Fi"><img src="https://i.imgur.com/eZzelq3.jpg"></a></td>
  </tr>
</table>


Running on a dedicated device in a car
------

To use openpilot in a car, you need four things
* A supported device to run this software: a [comma three](https://comma.ai/shop/products/three).
* This software. The setup procedure of the comma three allows the user to enter a url for custom software.
The url, openpilot.comma.ai will install the release version of openpilot. To install openpilot master, you can use installer.comma.ai/commaai/master, and replacing commaai with another github username can install a fork.
* One of [the 150+ supported cars](docs/CARS.md). We support Honda, Toyota, Hyundai, Nissan, Kia, Chrysler, Lexus, Acura, Audi, VW, and more. If your car is not supported, but has adaptive cruise control and lane keeping assist, it's likely able to run openpilot.
* A [car harness](https://comma.ai/shop/products/car-harness) to connect to your car.

We have detailed instructions for [how to mount the device in a car](https://comma.ai/setup).

Running on PC
------

All of openpilot's services can run as normal on a PC, even without special hardware or a car. To develop or experiment with openpilot you can run openpilot on recorded or simulated data.

With openpilot's tools you can plot logs, replay drives and watch the full-res camera streams. See [the tools README](tools/README.md) for more information.

You can also run openpilot in simulation [with the CARLA simulator](tools/sim/README.md). This allows openpilot to drive around a virtual car on your Ubuntu machine. The whole setup should only take a few minutes, but does require a decent GPU.

A PC running openpilot can also control your vehicle if it is connected to a [a webcam](https://github.com/commaai/openpilot/tree/master/tools/webcam), a [black panda](https://comma.ai/shop/products/panda), and [a harness](https://comma.ai/shop/products/car-harness).

Community and Contributing
------

openpilot is developed by [comma](https://comma.ai/) and by users like you. We welcome both pull requests and issues on [GitHub](http://github.com/commaai/openpilot). Bug fixes and new car ports are encouraged. Check out [the contributing docs](docs/CONTRIBUTING.md).

Documentation related to openpilot development can be found on [docs.comma.ai](https://docs.comma.ai). Information about running openpilot (e.g. FAQ, fingerprinting, troubleshooting, custom forks, community hardware) should go on the [wiki](https://github.com/commaai/openpilot/wiki).

You can add support for your car by following guides we have written for [Brand](https://blog.comma.ai/how-to-write-a-car-port-for-openpilot/) and [Model](https://blog.comma.ai/openpilot-port-guide-for-toyota-models/) ports. Generally, a car with adaptive cruise control and lane keep assist is a good candidate. [Join our Discord](https://discord.comma.ai) to discuss car ports: most car makes have a dedicated channel.

Want to get paid to work on openpilot? [comma is hiring](https://comma.ai/jobs/).

And [follow us on Twitter](https://twitter.com/comma_ai).

User Data and comma Account
------

By default, openpilot uploads the driving data to our servers. You can also access your data through [comma connect](https://connect.comma.ai/). We use your data to train better models and improve openpilot for everyone.

openpilot is open source software: the user is free to disable data collection if they wish to do so.

openpilot logs the road facing cameras, CAN, GPS, IMU, magnetometer, thermal sensors, crashes, and operating system logs.
The driver facing camera is only logged if you explicitly opt-in in settings. The microphone is not recorded.

By using openpilot, you agree to [our Privacy Policy](https://comma.ai/privacy). You understand that use of this software or its related services will generate certain types of user data, which may be logged and stored at the sole discretion of comma. By accepting this agreement, you grant an irrevocable, perpetual, worldwide right to comma for the use of this data.

Safety and Testing
----

* openpilot observes ISO26262 guidelines, see [SAFETY.md](docs/SAFETY.md) for more details.
* openpilot has software in the loop [tests](.github/workflows/selfdrive_tests.yaml) that run on every commit.
* The code enforcing the safety model lives in panda and is written in C, see [code rigor](https://github.com/commaai/panda#code-rigor) for more details.
* panda has software in the loop [safety tests](https://github.com/commaai/panda/tree/master/tests/safety).
* Internally, we have a hardware in the loop Jenkins test suite that builds and unit tests the various processes.
* panda has additional hardware in the loop [tests](https://github.com/commaai/panda/blob/master/Jenkinsfile).
* We run the latest openpilot in a testing closet containing 10 comma devices continuously replaying routes.

Directory Structure
------
    .
    ├── cereal              # The messaging spec and libs used for all logs
    ├── common              # Library like functionality we've developed here
    ├── docs                # Documentation
    ├── opendbc             # Files showing how to interpret data from cars
    ├── panda               # Code used to communicate on CAN
    ├── third_party         # External libraries
    ├── pyextra             # Extra python packages
    └── system              # Generic services
        ├── logcatd         # systemd journal as a service
        └── proclogd        # Logs information from /proc
    └── selfdrive           # Code needed to drive the car
        ├── assets          # Fonts, images, and sounds for UI
        ├── athena          # Allows communication with the app
        ├── boardd          # Daemon to talk to the board
        ├── camerad         # Driver to capture images from the camera sensors
        ├── car             # Car specific code to read states and control actuators
        ├── common          # Shared C/C++ code for the daemons
        ├── controls        # Planning and controls
        ├── debug           # Tools to help you debug and do car ports
        ├── locationd       # Precise localization and vehicle parameter estimation
        ├── loggerd         # Logger and uploader of car data
        ├── modeld          # Driving and monitoring model runners
        ├── proclogd        # Logs information from proc
        ├── sensord         # IMU interface code
        ├── navd            # Turn-by-turn navigation
        ├── test            # Unit tests, system tests, and a car simulator
        └── ui              # The UI

Licensing
------

openpilot is released under the MIT license. Some parts of the software are released under other licenses as specified.

Any user of this software shall indemnify and hold harmless Comma.ai, Inc. and its directors, officers, employees, agents, stockholders, affiliates, subcontractors and customers from and against all allegations, claims, actions, suits, demands, damages, liabilities, obligations, losses, settlements, judgments, costs and expenses (including without limitation attorneys’ fees and costs) which arise out of, relate to or result from any use of this software by user.

**THIS IS ALPHA QUALITY SOFTWARE FOR RESEARCH PURPOSES ONLY. THIS IS NOT A PRODUCT.
YOU ARE RESPONSIBLE FOR COMPLYING WITH LOCAL LAWS AND REGULATIONS.
NO WARRANTY EXPRESSED OR IMPLIED.**

---

<img src="https://d1qb2nb5cznatu.cloudfront.net/startups/i/1061157-bc7e9bf3b246ece7322e6ffe653f6af8-medium_jpg.jpg?buster=1458363130" width="75"></img> <img src="https://cdn-images-1.medium.com/max/1600/1*C87EjxGeMPrkTuVRVWVg4w.png" width="225"></img>

[![openpilot tests](https://github.com/commaai/openpilot/workflows/openpilot%20tests/badge.svg?event=push)](https://github.com/commaai/openpilot/actions)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/commaai/openpilot.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/commaai/openpilot/alerts/)
[![Language grade: Python](https://img.shields.io/lgtm/grade/python/g/commaai/openpilot.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/commaai/openpilot/context:python)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/commaai/openpilot.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/commaai/openpilot/context:cpp)
[![codecov](https://codecov.io/gh/commaai/openpilot/branch/master/graph/badge.svg)](https://codecov.io/gh/commaai/openpilot)
