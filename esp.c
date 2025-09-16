// MultiStepper.pde

#include <AccelStepper.h>
#include <MultiStepper.h>
#include <math.h>
#include <ESP32Servo.h>
#include <stdlib.h>
#include <string.h>

MultiStepper steppers;
Servo pen;
const int servoPin = 4;
const int MIN_US = 500;
const int MAX_US = 2400;
const int rightStepPin = 23;
const int rightDIRPin = 19;
const int leftStepPin = 21;
const int leftDIRPin = 18;
int CurrentX = 0;
int CurrentY = 0;
const int XMAX = 1300;
const int YMAX = 950;
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

void Gcodeline(int Xgoal, int Ygoal) {
  if (Xgoal > XMAX) Xgoal = XMAX;
  if (Xgoal < 0)    Xgoal = 0;
  if (Ygoal > YMAX) Ygoal = YMAX;
  if (Ygoal < 0)    Ygoal = 0;
  int dx = Xgoal - CurrentX;
  int dy = Ygoal - CurrentY;
  long rightMove = dx - dy;
  long leftMove  = dx + dy;
  long targets[2] = {
    rightStepper.currentPosition() + rightMove,
    leftStepper.currentPosition()  + leftMove
  };
  steppers.moveTo(targets);
  steppers.runSpeedToPosition();
  CurrentX = Xgoal;
  CurrentY = Ygoal;
}

static inline double normAngle(double a) {
  const double twoPi = 2.0 * M_PI;
  while (a < 0)     a += twoPi;
  while (a >= twoPi) a -= twoPi;
  return a;
}

void GcodeCW(int GoalX, int GoalY, int Icenter, int Jcenter) {
  const double CenterX = static_cast<double>(CurrentX) + static_cast<double>(Icenter);
  const double CenterY = static_cast<double>(CurrentY) + static_cast<double>(Jcenter);
  const double Radius = hypot(static_cast<double>(Icenter), static_cast<double>(Jcenter));
  if (!(Radius > 0.0)) return;
  const double L = 5.0;
  double cosTheta = 1.0 - (L * L) / (2.0 * Radius * Radius);
  if (cosTheta < -1.0) cosTheta = -1.0;
  if (cosTheta >  1.0) cosTheta =  1.0;
  double thetaOneStep = acos(cosTheta);
  if (!(thetaOneStep > 0.0) || isnan(thetaOneStep)) thetaOneStep = 0.05;
  double thetaStart = atan2(static_cast<double>(CurrentY) - CenterY, static_cast<double>(CurrentX) - CenterX);
  double thetaEnd   = atan2(static_cast<double>(GoalY)    - CenterY, static_cast<double>(GoalX)    - CenterX);
  thetaStart = normAngle(thetaStart);
  thetaEnd   = normAngle(thetaEnd);
  double sweepCW = thetaStart - thetaEnd;
  if (sweepCW <= 0.0) sweepCW += 2.0 * M_PI;
  int n = (int)ceil(sweepCW / thetaOneStep);
  if (n < 1) n = 1;
  const double step = -(sweepCW / (double)n);
  for (int i = 1; i <= n; ++i) {
    const double t = thetaStart + step * i;
    const double x = CenterX + Radius * cos(t);
    const double y = CenterY + Radius * sin(t);
    const int xi = (int)lround(x);
    const int yi = (int)lround(y);
    Gcodeline(xi, yi);
  }
  if (CurrentX != GoalX || CurrentY != GoalY) {
    Gcodeline(GoalX, GoalY);
  }
}

void GcodeCCW(int GoalX, int GoalY, int Icenter, int Jcenter) {
  const double CenterX = static_cast<double>(CurrentX) + static_cast<double>(Icenter);
  const double CenterY = static_cast<double>(CurrentY) + static_cast<double>(Jcenter);
  const double Radius = hypot(static_cast<double>(Icenter), static_cast<double>(Jcenter));
  if (!(Radius > 0.0)) return;
  const double L = 5.0;
  double cosTheta = 1.0 - (L * L) / (2.0 * Radius * Radius);
  if (cosTheta < -1.0) cosTheta = -1.0;
  if (cosTheta >  1.0) cosTheta =  1.0;
  double thetaOneStep = acos(cosTheta);
  if (!(thetaOneStep > 0.0) || isnan(thetaOneStep)) thetaOneStep = 0.05;
  double thetaStart = atan2(static_cast<double>(CurrentY) - CenterY, static_cast<double>(CurrentX) - CenterX);
  double thetaEnd   = atan2(static_cast<double>(GoalY)    - CenterY, static_cast<double>(GoalX)    - CenterX);
  thetaStart = normAngle(thetaStart);
  thetaEnd   = normAngle(thetaEnd);
  double sweepCW = thetaStart - thetaEnd;
  if (sweepCW <= 0.0) sweepCW += 2.0 * M_PI;
  int n = (int)ceil(sweepCW / thetaOneStep);
  if (n < 1) n = 1;
  const double step = (sweepCW / (double)n);
  for (int i = 1; i <= n; ++i) {
    const double t = thetaStart + step * i;
    const double x = CenterX + Radius * cos(t);
    const double y = CenterY + Radius * sin(t);
    const int xi = (int)lround(x);
    const int yi = (int)lround(y);
    Gcodeline(xi, yi);
  }
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

void loop() {
  String buffer = Serial.readString();
  // 아무것도 안 받으면 continue로 루프 처음으로
  if (buffer.length() == 0) continue;
  char *line = strtok((char*)buffer.c_str(), "\n");
  while (line != NULL) {
    if (strcmp(line, "done") == 0) {
      break;
    }
    if (strncmp(line, "G0", 2) == 0) {
      int X = 0, Y = 0;
      sscanf(line, "G0 X%d Y%d", &X, &Y);
      G0(X, Y);
    } else if (strncmp(line, "G1", 2) == 0) {
      int X = 0, Y = 0;
      sscanf(line, "G1 X%d Y%d", &X, &Y);
      G1(X, Y);
    } else if (strncmp(line, "G2", 2) == 0) {
      int X = 0, Y = 0, I = 0, J = 0;
      sscanf(line, "G2 X%d Y%d I%d J%d", &X, &Y, &I, &J);
      G2(X, Y, I, J);
    } else if (strncmp(line, "G3", 2) == 0) {
      int X = 0, Y = 0, I = 0, J = 0;
      sscanf(line, "G3 X%d Y%d I%d J%d", &X, &Y, &I, &J);
      G3(X, Y, I, J);
    }
    line = strtok(NULL, "\n");
  }
  Serial.println("ok");
}