#include <iostream>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <ctime>
#include <pthread.h>
#include <ratio>
#include <aubio/aubio.h>
#include <zmq.hpp>
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
float last_beat = 0.0;
float last_bpm = 120.0;
float last_confidence = 0.0;
float phase = 0.5;
int direction = 1;
float speed = 0.0;

std::chrono::high_resolution_clock::time_point last_time;
bool last_time_initialized = false;

void process_block(fvec_t * ibuf, fvec_t *obuf) {
  aubio_tempo_do(tempo, ibuf, tempo_out);
  is_beat = fvec_get_sample(tempo_out, 0.0);
  if (is_beat > 0.0) {
    last_beat = is_beat;
    obuf->data[0] = 1.0;
  } else {
    obuf->data[0] = 0.0;
  }
  float new_bpm = aubio_tempo_get_bpm(tempo);
  if (new_bpm != last_bpm || is_beat) {
    last_confidence = aubio_tempo_get_confidence(tempo);
    // std::cout << new_bpm << " " << last_beat << " " << last_confidence << std::endl;
    last_bpm = new_bpm;
  }

  // Oscillator

  // Time delta
  std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
  if (!last_time_initialized) {
    last_time = now;
    last_time_initialized = true;
  }
  std::chrono::duration<double> time_delta = now - last_time;
  float seconds_delta = time_delta.count();
  last_time = now;

  // Phase
  if (last_confidence == 0.0) {
    speed = 120.0 / 60.0;
  } else {
    speed = last_bpm / 60.0;
  }

  speed *= 0.5;

  if ((phase > 0.45 && phase < 0.55) && is_beat) {
    phase = 0.5;
  } else {
    if (direction == 1) {
      phase += seconds_delta * speed;
      if (phase >= 1.0) {
        phase = 1.0;
        direction = -1;
      }
    } else {
      phase -= seconds_delta * speed;
      if (phase <= 0.0) {
        phase = 0.0;
        direction = 1;
      }
    }
  }

  // 0.2 is very good. Lower than 0.1 is unstable.
  // if (silence_threshold != -90.) {
  //   is_silence = aubio_silence_detection(ibuf, silence_threshold);
  // }
}

void process_print (void) {
  if ( is_beat && !is_silence ) {
    std::cout << aubio_tempo_get_last(tempo) << std::endl;
  }
}

zmq::context_t ctx = zmq::context_t(1);
zmq::socket_t skt = zmq::socket_t(ctx, ZMQ_REP);

void *ServerThread(void *threadid) {
  skt.bind("tcp://*:5555");

  while (true) {
    zmq::message_t request;

    //  Wait for next request from client
    skt.recv (&request);

    //  Send reply back to client
    const int reply_size = 10;
    char message[reply_size];
    std::sprintf(message, "%.3f", phase);
    zmq::message_t reply(reply_size);
    memcpy(reply.data(), message, reply_size);
    skt.send(reply);
  }

  pthread_exit(NULL);
}

int main()
{
  pthread_t thread;
  pthread_create(&thread, NULL, ServerThread, 0);
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
  skt.close();
  ctx.close();
  pthread_exit(NULL);
  return 0;
}

