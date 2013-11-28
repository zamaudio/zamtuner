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

int Zamtunerdsp::read_timer(int i, float nearestnotehz, int sample_rate) {
	float prezero = sin(2.0*PI*nearestnotehz*(i-1)/sample_rate);
	float current = sin(2.0*PI*nearestnotehz*i/sample_rate);
	if (		(prezero < 0.0 && current >= 0.0) ) {
       		// zero crossing at end of wave
		return i;
	}
	return 0;
}

void Zamtunerdsp::process (float *p, int n)
{
	unsigned long N = Zamtunerdsp::buffer.cbsize;
        unsigned long Nf = Zamtunerdsp::buffer.corrsize;
        float fs = Zamtunerdsp::fs;
	unsigned long sample_count = n;

        float* pfInput=p;
        float* pfOutput=p;

	// Delay locked loop
	float nper = sample_count;
	float tper = sample_count / fs;
	float omega = 2.0*PI/440.0;
	float b = sqrt(2.0*omega);
	float c = omega*omega;
	float e;

	//init loop
	float e2 = tper;
	float t0 = Zamtunerdsp::read_timer(0, 440.0, fs);
	float t1 = t0 + e2;
	int k = 0;


        unsigned long lSampleIndex;
        for (lSampleIndex = 0; lSampleIndex < sample_count; lSampleIndex++)  {
		// init sample counts
		float n0 = 0;
		float n1 = nper;

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
                        float freqfound = 0.f;
                        float notefound = 0.f;
                        float diff = 0.f;
			float nearestnotehz = 0.f;

			// Delay locked loop iteration
			k = (k+1+(int)fs) % ((int)fs);

			// read timer and calculate loop error
			e = read_timer(k, nearestnotehz, fs) - t1;

			//update loop
			t0 = t1;
			t1 += b*e + e2;
			e2 += c*e;

                        if(pperiod>0 && fabs(in) > 0.0005) {
                                freqfound = 1.f/pperiod;
                                notefound = 12.0*log(freqfound/440.0)/log(2.0)+49.0;
                                if (notefound - rint(notefound) > 0.5) {
                                        nearestnote = rint(notefound) + 1;
                                } else {
                                        nearestnote = rint(notefound);
                                }
                                diff = nearestnote - notefound;
                                nearestnotenum = (nearestnote - 49 + 48) % 12;
                                Zamtunerdsp::fundamental = nearestnotenum;
				nearestnotehz = 440.0*powf(2.0, (nearestnote-49)/12.0);
				omega = 2.0*PI*1.0/freqfound;
				b = sqrt(2.0*omega);
				c = omega*omega;
				
				
				Zamtunerdsp::meter = t1/nearestnotehz*fs/10.0;//t1/freqfound*fs/10.0;
				printf("meter = %f\n", Zamtunerdsp::meter);
				printf("freqfound = %f\n", freqfound);

                        } else {
                                Zamtunerdsp::fundamental = -1.f;
                                Zamtunerdsp::meter = 0.f;
                                //printf("WtF pitch\n");
        		}

			//update sample counts
			n0 = n1;
			n1 += nper;
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
