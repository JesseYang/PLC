#ifndef ENCODER_H
#define ENCODER_H

#include <speex/speex.h>
using namespace std;

class Encoder {
  private:
    SpeexBits bits;
    void *enc_state;
  public:
    void encode();
};

#endif
