#pragma once

#include <stdint.h>
#include <vector>
#include <functional>

namespace rhythmus
{

/**
 * @brief Timer with calculating TickRate and Callback function with Tick Interval.
 */
class Timer
{
public:
  Timer();
  ~Timer();
  double GetTime();
  uint32_t GetTimeInMillisecond();
  void Start();
  void Stop();
  bool IsTimerStarted();
  void Tick();
  double GetTickRate();
  void SetEventInterval(double interval_second, bool loop);
  void RestartEvent();
  void ClearEvent();
  virtual void OnEvent();
  virtual void OnTick(double delta);

  static double GetUncachedGameTime();
  static double GetGameTime();
  static double GetGameTimeDelta();
  static uint32_t GetGameTimeDeltaInMillisecond();
  static uint32_t GetGameTimeInMillisecond();
  static void Initialize();
  static void Update();
private:
  double start_time_;
  double last_time_;
  double tick_rate_;
  double event_interval_;
  double event_next_tick_;
  bool event_loop_;
  bool timer_started_;
};

}