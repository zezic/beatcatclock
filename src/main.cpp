#include <iostream>
#include <cstdlib>
#include <cstring>
#include <aubio/aubio.h>
#include "../include/jackio.hpp"


fvec_t * tempo_out;
smpl_t is_beat = 0.;
uint_t is_silence = 0;
smpl_t silence_threshold = -90.;
uint_t mix_input = 0;

aubio_jack_t *jack_setup;
fvec_t *input_buffer;
fvec_t *output_buffer;

int buffer_size = 256;
int hop_size = 256;
int samplerate = 44100;
aubio_tempo_t *tempo = new_aubio_tempo("default", buffer_size, hop_size, samplerate);
aubio_wavetable_t *wavetable = new_aubio_wavetable(samplerate, hop_size);
float last_conf = 0.0;

void process_block(fvec_t * ibuf, fvec_t *obuf) {
  aubio_tempo_do(tempo, ibuf, tempo_out);
  if (tempo_out->data[0] > 0.0) {
    last_conf = tempo_out->data[0];
    obuf->data[0] = 1.0;
  } else {
    obuf->data[0] = 0.0;
  }
  // 0.2 is very good. Lower than 0.1 is unstable.
  std::cout << aubio_tempo_get_bpm(tempo) << " " << aubio_tempo_get_last(tempo) << " " << last_conf << " " << aubio_tempo_get_confidence(tempo) << std::endl;
  // is_beat = fvec_get_sample(tempo_out, 0);
  // if (silence_threshold != -90.) {
  //   is_silence = aubio_silence_detection(ibuf, silence_threshold);
  // }
}

void process_print (void) {
  if ( is_beat && !is_silence ) {
    std::cout << aubio_tempo_get_last(tempo) << std::endl;
  }
}

int main()
{
  jack_setup = new_aubio_jack(hop_size, 1, 1, 0, 1);
  samplerate = aubio_jack_get_samplerate(jack_setup);
  input_buffer = new_fvec(hop_size);
  output_buffer = new_fvec(hop_size);
  tempo_out = new_fvec(2);
  aubio_jack_activate(jack_setup, process_block);
  char input;
  std::cout << "\nRecording ... press <enter> to quit.\n";
  std::cin.get(input);
  aubio_jack_close(jack_setup);
  // RtAudio adc;
  // if ( adc.getDeviceCount() < 1 ) {
  //   std::cout << "\nNo audio devices found!\n";
  //   exit( 0 );
  // }
  // RtAudio::StreamParameters parameters;
  // parameters.deviceId = adc.getDefaultInputDevice();
  // parameters.nChannels = 2;
  // parameters.firstChannel = 0;
  // unsigned int sampleRate = 44100;
  // unsigned int bufferFrames = 256; // 256 sample frames
  // try {
  //   adc.openStream( NULL, &parameters, RTAUDIO_FLOAT64,
  //                   sampleRate, &bufferFrames, &record);
  //   adc.startStream();
  // }
  // catch ( RtAudioError& e ) {
  //   e.printMessage();
  //   exit( 0 );
  // }

  // char input;
  // std::cout << "\nRecording ... press <enter> to quit.\n";
  // std::cin.get( input );
  // try {
  //   // Stop the stream
  //   adc.stopStream();
  // }
  // catch (RtAudioError& e) {
  //   e.printMessage();
  // }
  // if ( adc.isStreamOpen() ) adc.closeStream();
  return 0;
}

