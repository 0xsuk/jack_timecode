#include <jack/jack.h>
#include <math.h>
#include <stdio.h>

// https://mixxx.org/news/2021-11-21-dvs-internals-pt1/

jack_port_t *output_port_left; //left means left channel (the left slide of groove) of vinyl record
jack_port_t *output_port_right; //right is used to tell whether record is playing forward or backward
jack_client_t *client;

int sample_rate = 0;
float phase_left = 0.0;
float phase_right = M_PI / 2.0; // 1/4 cycle ahead

int process(jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *out_left =
    (jack_default_audio_sample_t *)jack_port_get_buffer(output_port_left, nframes);
  jack_default_audio_sample_t *out_right =
    (jack_default_audio_sample_t *)jack_port_get_buffer(output_port_right, nframes);
  
  for (jack_nframes_t i = 0; i < nframes; i++) {
    out_left[i] = sin(phase_left);
    out_right[i] = sin(phase_right);
    phase_left += 2.0 * M_PI * 1000.0 / (float)sample_rate;
    phase_right += 2.0 * M_PI * 1000.0 / (float)sample_rate; //serato DJ timecode is 1000hz
    // 440hz means 440 mountains per sec.
    // phase goes from (0 to 2pi, imagine a circle) 440 times per sec.
    // for each sampling, phase should rotate 440/sample_rate circle
    // full cycle of circle is 2Pi.
    // for each sampling=each frame, phase should rotate 2pi*440 / sample_rate radian
    if (phase_left >= 2.0 * M_PI) phase_left -= 2.0 * M_PI;
    if (phase_right >= 2.0 * M_PI) phase_right -= 2.0 * M_PI;
  }

  return 0;
}

int sample_rate_cb(jack_nframes_t rate, void *arg) {
  sample_rate = rate;
  return 0;
}

int main(int argc, char *argv[]) {
  client = jack_client_open("simple_jack_client", JackNullOption, NULL);
  if (client == NULL) {
    fprintf(stderr, "Could not connect to JACK server.\n");
    return 1;
  }

  jack_set_process_callback(client, process, 0);
  jack_set_sample_rate_callback(client, sample_rate_cb, 0);

  sample_rate = jack_get_sample_rate(client);

  output_port_left = jack_port_register(client, "output_left", JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsOutput, 0);
  output_port_right = jack_port_register(client, "output_right", JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsOutput, 0);
  
  if (jack_activate(client)) {
    fprintf(stderr, "Cannot activate client.\n");
    return 1;
  }

  printf("Client activated. Press Enter to stop...\n");
  getchar();

  jack_client_close(client);
  return 0;
}
