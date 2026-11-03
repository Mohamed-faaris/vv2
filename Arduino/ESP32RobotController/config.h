#pragma once
#include <Arduino.h>
enum class RobotMode:uint8_t{IR,ULTRASONIC,BOTH,CONSTANT};
enum class LogLevel:uint8_t{ERROR=0,STATUS=1,INFO=2,DEBUG=3,TRACE=4};
struct Config{
 RobotMode mode=RobotMode::BOTH; LogLevel logLevel=LogLevel::STATUS;
 bool irEnabled=true,ultrasonicEnabled=true,irActiveLow=true,motorEnabled=true,buzzerEnabled=true,ledEnabled=true,builtinLedEnabled=true,buzzerOnStop=true;
 uint16_t stopDistance=5,redLimit=5,yellowLimit=10,sampleInterval=100,ultrasonicTimeoutMs=30;
 uint8_t motorSpeed=200; bool wifiEnabled=true,apEnabled=true,staEnabled=true,corsEnabled=true;
 char apSsid[33]="ESP32-Robot",apPassword[65]="robot1234",staSsid[33]="",staPassword[65]="";
};
