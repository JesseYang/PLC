#include <iostream>
#include <thread>
#include "timer.h"
using namespace std;

Timer::Timer(int total_time, pf callback) {
  this->total_time = total_time;
  this->remain_time = total_time;
  this->callback = callback;
}

void Timer::refresh() {
  remain_time = total_time;
}

thread Timer::start() {
  in_use = true;
  in_pause = false;
  remain_time = total_time;
  // create new thread to run
  thread t1(&Timer::run, this);
  return t1;
}

void Timer::stop() {
  in_use = false;
  in_pause = false;
}

void Timer::pause() {
  if (in_use == false) {
    return;
  }
  in_pause = true;
}

void Timer::resume() {
  if (in_use == false) {
    return;
  }
  in_pause = false;
}

void Timer::run() {
  while (remain_time > 0) {
    cout << remain_time << endl;
    sleep(1);
    if (!in_use) {
      return;
    }
    if (!in_pause) {
      remain_time--;
    }
  }
  // call pf
  this->callback();
}
