#ifndef TALENTENEDHACK_LADSPA_H
#define TALENTENEDHACK_LADSPA_H

#include "pitch_detector.h"
#include "fft.h"
#include <lv2.h>

#define ZAMTUNER_URI "http://zamaudio.com/lv2/zamtuner"

#define AT_AUDIO_IN 0
#define AT_AUDIO_OUT 1
#define AT_FUNDAMENTAL 2
#define AT_FINETUNE 3

typedef struct {

	float* p_InputBuffer;
	float* p_OutputBuffer;
	float* fundamental;
	float* finetune;
	fft_vars* fmembvars; // member variables for fft routine

	unsigned long fs; // Sample rate
	int noverlap;
	
	PitchDetector pdetector;
	CircularBuffer buffer;
} ZamTuner
;
#endif
