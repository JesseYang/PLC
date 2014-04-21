#ifndef TIMER_H
#define TIMER_H

#include <thread>
using namespace std;

typedef void (*pf)();

class Timer {
  private:
    pf callback;
    bool in_use;
    bool in_pause;
    int remain_time;
    int total_time;
    void run();
  public:
    void refresh();
    thread start();
    void stop();
    void pause();
    void resume();
    Timer(int, pf);
};

#endif
