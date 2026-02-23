//cltick.cpp
#include <stdlib.h>
#include <cstdio>
#include <alsa/asoundlib.h>


snd_pcm_t *handle;
const short MAX=32767;
const int sampleRate = 8000;
const int freq=200;
const int bufferSize=sampleRate/freq; 
short bufferTick[bufferSize]; //space for a period
short bufferSilent[bufferSize]; //space for a period
int period; //number of samples of tick + silence
int bufferIndex;
const char *deviceName="default";

void printWriteError(int err, int size) {
        if (err < 0) {
            printf("Write error: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
        }
        if (err != size) {
            printf("Write error: written %i expected %d\n", err, size);
            exit(EXIT_FAILURE);
        }
}

void printAvailError(snd_pcm_sframes_t avail){
    if(avail<0) {
      printf("Write error: %s\n", snd_strerror(avail));
    }
}

void async_callback(snd_async_handler_t *ahandler) {
    snd_pcm_sframes_t avail;
    int err;

    avail = snd_pcm_avail_update(handle);
    printAvailError(avail);
    while (avail >= bufferSize) {
	int size;
        if(bufferIndex<bufferSize * 4) { //four periods
	  size=bufferSize;
          err = snd_pcm_writei(handle, bufferTick, size);
	  bufferIndex +=bufferSize;
	} else if(bufferIndex<period-bufferSize) {
	  size=bufferSize;
          err = snd_pcm_writei(handle, bufferSilent, size);
	  bufferIndex +=bufferSize;
	} else {
	  size=period-bufferIndex;
          err = snd_pcm_writei(handle, bufferSilent, size);
	  bufferIndex=0;
	}
	printWriteError(err,size);
        avail = snd_pcm_avail_update(handle);
        printAvailError(avail);
    }
}

int async_loop(snd_pcm_t *handle) {
    snd_pcm_sframes_t avail;
    snd_async_handler_t *ahandler;
    int err;

    err = snd_async_add_pcm_handler(&ahandler, handle, async_callback, 0);
    if (err < 0) {
        printf("Unable to register async handler\n");
        exit(EXIT_FAILURE);
    }
    avail = snd_pcm_avail_update(handle);
    printAvailError(avail);
    for(int i=0; i< avail/bufferSize; i++) {
      err = snd_pcm_writei(handle, bufferSilent, bufferSize);
      printWriteError(err,bufferSize);
    }
    bufferIndex=0;
    while (1) {
        sleep(10);
    }
}

snd_pcm_t *alsaOpen(const char *deviceName, snd_pcm_stream_t mode, unsigned int rate, snd_pcm_uframes_t periodSize, uint32_t periods, int channels) {
    snd_pcm_t *handle;

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    if (snd_pcm_open (&handle, deviceName, mode, 0) < 0) {
        fprintf (stderr, "cannot open audio device %s\n", deviceName);
        exit (1);
    }
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(handle, hw_params);
    snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);

    snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0);
    snd_pcm_hw_params_set_channels(handle, hw_params, channels);
    snd_pcm_hw_params_set_periods(handle, hw_params, periods, 0);
    snd_pcm_hw_params_set_period_size(handle, hw_params, periodSize, 0);

    snd_pcm_hw_params(handle, hw_params);


    snd_pcm_sw_params_alloca(&sw_params);
    snd_pcm_sw_params_current(handle, sw_params);
    snd_pcm_sw_params_set_avail_min(handle, sw_params, periodSize);
    snd_pcm_sw_params(handle, sw_params);

    snd_pcm_prepare(handle);
    snd_pcm_start(handle);
    return handle;
}

int main(int argc, char *argv[]) {
  int bpm;

  if(argc!=2) {
    printf("Usage: cltick bpm\n");
    exit(0);
  }
  bpm=atoi(argv[1]);
  if(bpm<=0) {
    printf("Error in bpm\n");
    exit(0);
  }
  period = sampleRate*60/bpm;
  handle=alsaOpen(deviceName,SND_PCM_STREAM_PLAYBACK,sampleRate,256,16,1);
  for(int i=0;i<bufferSize/2; i++){
    bufferTick[i]=abs(random())%MAX;
    bufferSilent[i]=0;
  }
  for(int i=bufferSize/2;i<bufferSize; i++){
    bufferTick[i]=-1 * (abs(random())%MAX);
    bufferSilent[i]=0;
  }
  bufferIndex=0;
  int err = async_loop(handle);
  if (err < 0) printf("Transfer failed: %s\n", snd_strerror(err));


  snd_pcm_drain(handle);
  snd_pcm_close(handle);
}

