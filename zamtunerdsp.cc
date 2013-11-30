// Copyright (C) 2013 Damien Zammit <damien@zamaudio.com>
// Copyright (C) 2013 Robin Gareus <robin@gareus.org>

//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <math.h>
#include "zamtunerdsp.h"

namespace LV2M {

Zamtunerdsp::Zamtunerdsp (void) :
	meter(0.f),
	fundamental(-1.f),
	nearestnotehz(440.0),
	tuna_fc(0),
	prev_smpl(0),
	rms_omega(1.0f - expf(-2.0 * M_PI * .05 / 44100)),
	rms_signal(0),
	dll_initialized(false)
{
}

Zamtunerdsp::~Zamtunerdsp (void)
{
}

inline void Zamtunerdsp::IncrementPointer(CircularBuffer& buffer) {
        // Input write pointer logic
        CircularBuffer *buf = &buffer;
	buf->cbiwr++;
        if (buf->cbiwr >= buf->cbsize) {
                buf->cbiwr = 0;
        }
}

void Zamtunerdsp::process (float *p, int n)
{
	unsigned long N = Zamtunerdsp::buffer.cbsize;
        unsigned long Nf = Zamtunerdsp::buffer.corrsize;
        float fs = Zamtunerdsp::fs;
	unsigned long sample_count = n;

        float* pfInput=p;
        float* pfOutput=p;
	
	float freqfound = 0.f;

	/* localize variables */
	float* a_out = p;

	float prev_smpl = Zamtunerdsp::prev_smpl;
	float rms_signal = Zamtunerdsp::rms_signal;
	const float rms_omega  = Zamtunerdsp::rms_omega;

	float    detected_freq = 0;
	uint32_t detected_count = 0;

        unsigned long lSampleIndex;
        for (lSampleIndex = 0; lSampleIndex < sample_count; lSampleIndex++)  {

		// load data into circular buffer
                float in = (float) *(pfInput++);

                Zamtunerdsp::buffer.cbi[Zamtunerdsp::buffer.cbiwr] = in;

                Zamtunerdsp::IncrementPointer(Zamtunerdsp::buffer);

                // Every N/noverlap samples, run pitch estimation / manipulation code
                if ((Zamtunerdsp::buffer.cbiwr)%(N/Zamtunerdsp::noverlap) == 0) {
                        //  ---- Calculate pitch and confidence ----
                        float pperiod=get_pitch_period(&pdetector, obtain_autocovariance(&pdetector,fmembvars,&buffer,N),Nf,fs);
                        int nearestnotenum = 0;
                        int nearestnote = 0;
                        freqfound = 0.f;
                        float notefound = 0.f;
                        //float diff = 0.f;

                        if(pperiod>0 && fabs(in) > 0.0006) {
                                freqfound = 1.f/pperiod;
                                notefound = 12.0*log(freqfound/440.0)/log(2.0)+49.0;
                                if (notefound - rint(notefound) > 0.5) {
                                        nearestnote = rint(notefound) + 1;
                                } else {
                                        nearestnote = rint(notefound);
                                }
                                //diff = nearestnote - notefound;
                                nearestnotenum = (nearestnote - 49 + 48) % 12;
                                Zamtunerdsp::fundamental = nearestnotenum;
				nearestnotehz = 440.0*powf(2.0, (nearestnote-49)/12.0);
				//printf("nearestnotehz = %f\n", nearestnotehz);
			}
		}

		/* 1) calculate RMS */
		rms_signal +=  rms_omega * ( (in * in) - rms_signal) + 1e-12;

		/* no need to take sqrt, just compare against square, -30dB 
		 * threshold == (10^(.05 × -30)^2 = 0.001
		 */
		if (rms_signal < 0.001) {
			/* signal below threshold */
			Zamtunerdsp::dll_initialized = false;
			prev_smpl = 0;
			continue;
		}

		/* 2) detect frequency to track
		 *
		 */
		float freq = nearestnotehz;
		if (freq < 65) freq = 65;
		if (freq > 10000) freq = 10000;

		if (freq != Zamtunerdsp::tuna_fc) {
			Zamtunerdsp::tuna_fc = freq;
			
			/* calculate DLL coefficients */
			const double omega = 2.0 * M_PI * Zamtunerdsp::tuna_fc / Zamtunerdsp::fs;
			Zamtunerdsp::dll_b = 1.4142135623730950488 * omega; // sqrt(2)
			Zamtunerdsp::dll_c = omega * omega;
			Zamtunerdsp::dll_initialized = false;

			/* reinitialize filter */
			bandpass_setup(&(Zamtunerdsp::fb), Zamtunerdsp::fs
					, Zamtunerdsp::tuna_fc       /* center frequency */
					, Zamtunerdsp::tuna_fc * .15 /* bandwidth, a bit more than +-1 semitone */
					, 4 /*th order butterworth */);
		}
		
		/* 3) band-pass filter the signal, clean up for
		 * counting zero-transitions
		 */
		a_out[lSampleIndex] = bandpass_process(&(Zamtunerdsp::fb), in);

		/* 4) track phase of zero-transitions
		 * using a 2nd order phase-locked loop
		 */
		if (a_out[lSampleIndex] >= 0 && prev_smpl < 0) {
			/* rising edge */

			if (!Zamtunerdsp::dll_initialized) {
				/* re-initialize DLL */
				//printf("re-init DLL\n");
				Zamtunerdsp::dll_initialized = true;
				Zamtunerdsp::monotonic_cnt = 0;
				Zamtunerdsp::dll_e0 = Zamtunerdsp::dll_t0 = 0;
				Zamtunerdsp::dll_t1 = Zamtunerdsp::dll_e2 = Zamtunerdsp::fs / Zamtunerdsp::tuna_fc;
			} else {
				Zamtunerdsp::dll_e0 = (Zamtunerdsp::monotonic_cnt + lSampleIndex) - Zamtunerdsp::dll_t1;
				Zamtunerdsp::dll_t0 = Zamtunerdsp::dll_t1;
				Zamtunerdsp::dll_t1 += Zamtunerdsp::dll_b * Zamtunerdsp::dll_e0 + Zamtunerdsp::dll_e2;
				Zamtunerdsp::dll_e2 += Zamtunerdsp::dll_c * Zamtunerdsp::dll_e0;

				detected_freq += Zamtunerdsp::fs / (Zamtunerdsp::dll_t1 - Zamtunerdsp::dll_t0);
				//printf("detected Freq: %.2f\n", detected_freq);
				detected_count++;
			}
		}
		prev_smpl = a_out[lSampleIndex];
		*(pfOutput++)=in;
	}

	if (detected_count > 0) {
		Zamtunerdsp::meter = 12.0*logf(nearestnotehz/((detected_freq / (float)detected_count)))/logf(2.0);
		printf("freq : %.2f\n", detected_freq / (float)detected_count);
	} else if (!Zamtunerdsp::dll_initialized) {
		Zamtunerdsp::meter = 0; // no signal detected; below threshold
		Zamtunerdsp::fundamental = -1.f; // no signal detected; below threshold

	} /* otherwise no change, may be short cycle */

	Zamtunerdsp::prev_smpl = prev_smpl;
	Zamtunerdsp::rms_signal = rms_signal;
	Zamtunerdsp::monotonic_cnt += sample_count;
}

float Zamtunerdsp::readfine (void)
{
	return meter;
}

float Zamtunerdsp::readnote (void)
{
	return fundamental;
}

void Zamtunerdsp::init (float fsamp)
{
        InstantiateCircularBuffer(&buffer,fsamp);
        unsigned long N=buffer.cbsize;
        Zamtunerdsp::fmembvars = fft_con(N);
        Zamtunerdsp::fs = fsamp;
        Zamtunerdsp::noverlap = 6;

        InstantiatePitchDetector(&pdetector, fmembvars, N, fsamp);
}

}
