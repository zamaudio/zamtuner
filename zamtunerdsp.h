#ifndef ZAMTUNERDSP_H
#define ZAMTUNERDSP_H

extern "C" {
#include "circular_buffer.h"
#include "pitch_detector.h"
#include "fft.h"
#include "spectr.c"
};


namespace LV2M {

class Zamtunerdsp
{
public:
	Zamtunerdsp (void);
	~Zamtunerdsp (void);

	void process (float *p, int n);
	float readfine (void);
	float readnote (void);
	int read_timer (int, float, int);
	inline void IncrementPointer(CircularBuffer& buffer);
	void init (float fsamp);

	fft_vars* fmembvars; // member variables for fft routine

	unsigned long fs; // Sample rate
	int noverlap;

	PitchDetector pdetector;
	CircularBuffer buffer;

	float meter;
	float fundamental;

	float* a_in;
	float* a_out;

	float* p_freq_in;
	float* p_freq_out;

	double nearestnotehz;

	struct FilterBank fb;

	float tuna_fc; //center freq of expected note

	/* discriminator */
	float prev_smpl;

	/* RMS / threshold */
	float rms_omega;
	float rms_signal;

	/* DLL */
	bool dll_initialized;
	uint32_t monotonic_cnt;
	double dll_e2, dll_e0;
	double dll_t0, dll_t1;
	double dll_b, dll_c;
};

};

#endif
