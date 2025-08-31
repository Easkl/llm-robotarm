// MultiStepper.pde

#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>
#include <ESP32Servo.h>

MultiStepper steppers;

Servo pen;
const int servoPin = 4;
const int MIN_US = 500;
const int MAX_US = 2400;

const int rightStepPin = 23;
const int rightDIRPin = 19;
const int leftStepPin = 21;
const int leftDIRPin = 18;

//시작 위치를 0,0으로 하고 코드를 실행한다 (스텝모터 두개가 상단에 위치하고, 헤드가 왼쪽 아래에 위치)
// 오른쪽 위가 x,y의 양의방향
int CurrentX = 0;
int CurrentY = 0;

//여러 시도 끝에 찾아낸 각 좌표의 최대값
const int XMAX = 1300;
const int YMAX = 950;

// Define some steppers and the pins the will use
AccelStepper rightStepper(AccelStepper::DRIVER, rightStepPin, rightDIRPin);
AccelStepper leftStepper(AccelStepper::DRIVER, leftStepPin, leftDIRPin);

void setup() {
  rightStepper.setMaxSpeed(500);
  leftStepper.setMaxSpeed(500); 
  steppers.addStepper(rightStepper);  
  steppers.addStepper(leftStepper);
  ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);

  pen.attach(servoPin, MIN_US, MAX_US);
}

//직선 gcode 해석
//gcode를 해석할 때 받은 값을 여기 넣는다
void Gcodeline(int Xgoal, int Ygoal) {
    //최대 최소 정의
    if (Xgoal > XMAX) Xgoal = XMAX;
    if (Xgoal < 0)    Xgoal = 0;
    if (Ygoal > YMAX) Ygoal = YMAX;
    if (Ygoal < 0)    Ygoal = 0;
    
    //상대 이동량을 현재 위치를 기반으로 구함
    int dx = Xgoal - CurrentX;
    int dy = Ygoal - CurrentY;

    //plotter 구조에 따른 step 모터 상대 이동량
    long rightMove = dx - dy;
    long leftMove  = dx + dy;

    // 이동, 동시
    long targets[2] = {
      rightStepper.currentPosition() + rightMove,
      leftStepper.currentPosition()  + leftMove
    };
    steppers.moveTo(targets);
    steppers.runSpeedToPosition();

    //현재 포지션 업데이트
    CurrentX = Xgoal;
    CurrentY = Ygoal;
}

// 보조: [0, 2π) 정규화
static inline double normAngle(double a) {
  const double twoPi = 2.0 * M_PI;
  while (a < 0)     a += twoPi;
  while (a >= twoPi) a -= twoPi;
  return a;
}

// 시계방향(CW) 원호 보간: G2 스타일
// Icenter, Jcenter는 "현재점에서 중심까지"의 오프셋(고전 G-code I, J 의미)
void GcodeCW(int GoalX, int GoalY, int Icenter, int Jcenter) {
  // 중심 좌표
  const double CenterX = static_cast<double>(CurrentX) + static_cast<double>(Icenter);
  const double CenterY = static_cast<double>(CurrentY) + static_cast<double>(Jcenter);

  // 반지름 (현재점 기준)
  const double Radius = hypot(static_cast<double>(Icenter), static_cast<double>(Jcenter));
  if (!(Radius > 0.0)) {
    // 반지름 0이면 조기 종료
    return;
  }

  // 세그먼트 길이 L = 5 (좌표 단위)
  const double L = 5.0;

  // 한 세그먼트의 중심각: cosθ = 1 - L^2/(2R^2)
  double cosTheta = 1.0 - (L * L) / (2.0 * Radius * Radius);
  if (cosTheta < -1.0) cosTheta = -1.0;
  if (cosTheta >  1.0) cosTheta =  1.0;

  double thetaOneStep = acos(cosTheta);
  if (!(thetaOneStep > 0.0) || isnan(thetaOneStep)) {
    // 매우 큰 R 등으로 수치 불안정하면 최소 각도 보장(≈2.86도)
    thetaOneStep = 0.05;
  }

  // 시작/끝 각도 (atan2(y - yc, x - xc))
  double thetaStart = atan2(static_cast<double>(CurrentY) - CenterY,
                            static_cast<double>(CurrentX) - CenterX);
  double thetaEnd   = atan2(static_cast<double>(GoalY)    - CenterY,
                            static_cast<double>(GoalX)    - CenterX);

  thetaStart = normAngle(thetaStart);
  thetaEnd   = normAngle(thetaEnd);

  // 시계방향 스윕 각 (양수, (0, 2π])
  double sweepCW = thetaStart - thetaEnd;
  if (sweepCW <= 0.0) sweepCW += 2.0 * M_PI;

  // 세그먼트 수(최소 1): ceil(스윕/세그먼트각)
  int n = (int)ceil(sweepCW / thetaOneStep);
  if (n < 1) n = 1;

  // 실제 사용할 스텝 각도(시계방향 → 음수)
  const double step = -(sweepCW / (double)n);

  // 세그먼트별로 G1로 근사 이동
  for (int i = 1; i <= n; ++i) {
    const double t = thetaStart + step * i; // CW: 각도 감소
    const double x = CenterX + Radius * cos(t);
    const double y = CenterY + Radius * sin(t);

    // 정수 좌표로 스냅 (라운드)
    const int xi = (int)lround(x);
    const int yi = (int)lround(y);

    Gcodeline(xi, yi);
  }

  // 목표점 보정(정밀 종착)
  if (CurrentX != GoalX || CurrentY != GoalY) {
    Gcodeline(GoalX, GoalY);
  }
}

void GcodeCCW(int GoalX, int GoalY, int Icenter, int Jcenter) {
  // 중심 좌표
  const double CenterX = static_cast<double>(CurrentX) + static_cast<double>(Icenter);
  const double CenterY = static_cast<double>(CurrentY) + static_cast<double>(Jcenter);

  // 반지름 (현재점 기준)
  const double Radius = hypot(static_cast<double>(Icenter), static_cast<double>(Jcenter));
  if (!(Radius > 0.0)) {
    // 반지름 0이면 조기 종료
    return;
  }

  // 세그먼트 길이 L = 5 (좌표 단위)
  const double L = 5.0;

  // 한 세그먼트의 중심각: cosθ = 1 - L^2/(2R^2)
  double cosTheta = 1.0 - (L * L) / (2.0 * Radius * Radius);
  if (cosTheta < -1.0) cosTheta = -1.0;
  if (cosTheta >  1.0) cosTheta =  1.0;

  double thetaOneStep = acos(cosTheta);
  if (!(thetaOneStep > 0.0) || isnan(thetaOneStep)) {
    // 매우 큰 R 등으로 수치 불안정하면 최소 각도 보장(≈2.86도)
    thetaOneStep = 0.05;
  }

  // 시작/끝 각도 (atan2(y - yc, x - xc))
  double thetaStart = atan2(static_cast<double>(CurrentY) - CenterY,
                            static_cast<double>(CurrentX) - CenterX);
  double thetaEnd   = atan2(static_cast<double>(GoalY)    - CenterY,
                            static_cast<double>(GoalX)    - CenterX);

  thetaStart = normAngle(thetaStart);
  thetaEnd   = normAngle(thetaEnd);

  // 시계방향 스윕 각 (양수, (0, 2π])
  double sweepCW = thetaStart - thetaEnd;
  if (sweepCW <= 0.0) sweepCW += 2.0 * M_PI;

  // 세그먼트 수(최소 1): ceil(스윕/세그먼트각)
  int n = (int)ceil(sweepCW / thetaOneStep);
  if (n < 1) n = 1;

  // 실제 사용할 스텝 각도(시계방향 → 음수)
  const double step = (sweepCW / (double)n);

  // 세그먼트별로 G1로 근사 이동
  for (int i = 1; i <= n; ++i) {
    const double t = thetaStart + step * i; // CW: 각도 감소
    const double x = CenterX + Radius * cos(t);
    const double y = CenterY + Radius * sin(t);

    // 정수 좌표로 스냅 (라운드)
    const int xi = (int)lround(x);
    const int yi = (int)lround(y);

    Gcodeline(xi, yi);
  }

  // 목표점 보정(정밀 종착)
  if (CurrentX != GoalX || CurrentY != GoalY) {
    Gcodeline(GoalX, GoalY);
  }
}

int penStatus = 0;

void Pen(int signal) {
  if (signal == 1) {
  pen.write(90);
  delay(200);
  penStatus = 1;
  }
  if (signal == 0) {
  pen.write(0);
  delay(200);
  penStatus = 0;
  }
}

// 위의 기본적인 명령어에 따른 Gcode 변환

void G0(int X,int Y) {
  Pen(0);
  delay(200);
  Gcodeline(5*X,5*Y);
  delay(200);
}

void G1(int X,int Y) {
  if (penStatus == 0) {
  Pen(1);
  delay(200);
  Gcodeline(5*X,5*Y);
  delay(200);
  }
  else if (penStatus == 1) {
  Gcodeline(5*X,5*Y);
  delay(200);
  }
}

void G2(int X,int Y,int I,int J) {
  if (penStatus == 0) {
  Pen(1);
  delay(200);
  GcodeCW(5*X, 5*Y, 5*I, 5*J);
  delay(200);
  }
  else if (penStatus == 1) {
  GcodeCW(5*X, 5*Y, 5*I, 5*J);
  delay(200);
  }
}

void G3(int X,int Y,int I,int J) {
  if (penStatus == 0) {
  Pen(1);
  delay(200);
  GcodeCCW(5*X, 5*Y, 5*I, 5*J);
  delay(200);
  }
  else if (penStatus == 1) {
  GcodeCCW(5*X, 5*Y, 5*I, 5*J);
  delay(200);
  }
}
//



void loop()
{

//시작할때는 펜 up
Pen(0);

// 여기서부터 testbed----------------------------------
  G0(10, 30);
  G1(287, 30);

  G0(20, 50);
  G1(140, 50);
  G1(140, 170);
  G1(20, 170);
  G1(20, 50);

  G0(20, 50);
  G1(10, 30);

  G0(140, 50);
  G1(150, 30);

  G0(15, 40);
  G1(145, 40);

  G0(30, 70);
  G1(50, 130);
  G1(70, 70);

  G0(80, 70);
  G1(100, 140);
  G1(120, 70);

  G0(128, 150);
  G2(128, 150, -8, 0);

  G0(25, 80);
  G1(60, 75);
  G1(80, 65);
  G1(120, 60);
  G1(135, 55);

  G0(141, 130);
  G2(141, 130, -1, 0);

  G0(235, 160);
  G2(235, 160, -15, 0);

  G0(216, 164);
  G2(216, 164, -2, 0);

  G0(228, 164);
  G2(228, 164, -2, 0);

  G0(214, 154);
  G1(226, 154);

  G0(220, 175);
  G1(220, 188);

  G0(222, 190);
  G2(222, 190, -2, 0);

  G0(205, 120);
  G1(235, 120);
  G1(235, 150);
  G1(205, 150);
  G1(205, 120);

  G0(210, 125);
  G1(230, 125);
  G1(230, 145);
  G1(210, 145);
  G1(210, 125);

  G0(217, 140);
  G2(217, 140, -2, 0);

  G0(227, 140);
  G2(227, 140, -2, 0);

  G0(208, 140);
  G2(208, 140, -3, 0);

  G0(238, 140);
  G2(238, 140, -3, 0);

  G0(205, 140);
  G1(175, 135);

  G0(178, 135);
  G2(178, 135, -3, 0);

  G0(175, 135);
  G1(150, 130);

  G0(153, 130);
  G2(153, 130, -3, 0);

  G0(150, 130);
  G1(140, 130);

  G0(140, 130);
  G1(138, 132);
  G1(138, 128);
  G1(140, 130);

  G0(235, 140);
  G1(250, 125);

  G0(253, 125);
  G2(253, 125, -3, 0);

  G0(250, 125);
  G1(235, 110);

  G0(238, 110);
  G2(238, 110, -3, 0);

  G0(215, 120);
  G1(210, 100);

  G0(213, 100);
  G2(213, 100, -3, 0);

  G0(210, 100);
  G1(215, 80);

  G0(215, 80);
  G1(225, 80);

  G0(225, 120);
  G1(230, 100);

  G0(233, 100);
  G2(233, 100, -3, 0);

  G0(230, 100);
  G1(225, 80);

  G0(225, 80);
  G1(235, 80);

  G0(200, 90);
  G1(240, 90);
  G1(240, 80);
  G1(200, 80);
  G1(200, 90);

  G0(205, 80);
  G1(205, 30);

  G0(235, 80);
  G1(235, 30);

  G0(220, 80);
  G1(215, 30);

  G0(252, 30);
  G1(265, 110);
  G1(278, 30);

  G0(262, 30);
  G1(275, 90);
  G1(287, 30);

  G0(280, 180);
  G2(280, 180, -10, 0);

  G0(255, 30);
  G1(255, 70);

  G0(263, 80);
  G2(263, 80, -8, 0);

  G0(263, 170);
  G2(263, 170, -3, 0);

  G0(273, 168);
  G2(273, 168, -3, 0);

  G0(250, 160);
  G1(254, 164);
  G1(258, 160);

  G0(240, 170);
  G1(244, 174);
  G1(248, 170);
//-------------------------------------------------------


  //마지막에 원점으로 돌아와서 펜 down (점검을 위해)
  G0(0,0);
  Pen(1);
  while(1);

}
