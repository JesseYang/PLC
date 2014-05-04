#include <speex/speex.h>
using namespace std;

Encoder::Encoder() {
  speex_bits_init(&this->bits);
  this->enc_state = speex_encoder_init(&speex_nb_mode);
}

void Encoder::encode() {
}
