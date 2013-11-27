#include <math.h>
#include "zamtunerdsp.h"

namespace LV2M {

Zamtunerdsp::Zamtunerdsp (void) :
	meter(0.f),
	fundamental(-1.f)
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
                        int nearestnote = 0;
                        float freqfound = 0.f;
                        float notefound = 0.f;
                        float diff = 0.f;
			
                        if(pperiod>0 && fabs(in) > 0.0005) {
                                freqfound = 1.f/pperiod;
                                notefound = 12.0*log(freqfound/440)/log(2.0)+49.0;
                                if (notefound - (int)notefound > 0.5) {
                                        nearestnote = (int)notefound + 1;
                                } else {
                                        nearestnote = (int)notefound;
                                }
                                diff = nearestnote - notefound;
                                nearestnote = (nearestnote - 49 + 48) % 12;
                                Zamtunerdsp::meter = diff;
                                Zamtunerdsp::fundamental = nearestnote;

                        } else {
                                Zamtunerdsp::fundamental = -1.f;
                                Zamtunerdsp::meter = 0.f;
                                //printf("WtF pitch\n");
                        }
                }

                *(pfOutput++)=in;

        }
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
        Zamtunerdsp::noverlap = 2;

        InstantiatePitchDetector(&pdetector, fmembvars, N, fsamp);
}

}
