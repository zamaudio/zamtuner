#include <math.h>
#include "zamtunerdsp.h"

namespace LV2M {

Zamtunerdsp::Zamtunerdsp (void) :
	meter(0.f),
	fundamental(-1.f),
        nper(44100.0/440.0),
	tper(1.0/440.0),
	oldomega(2.0*M_PI*440.0/44100.0),
        e2(1.0/440.0),
        t0(0.0),
        t1(2.0*M_PI*440.0),
        n0(0.0),
	n1(44100.0/440.0)
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
	double prezero = sin(2.0*PI*nearestnotehz*(i-1)/sample_rate);
	double current = sin(2.0*PI*nearestnotehz*i/sample_rate);
	if (		(prezero < 0.0 && current >= 0.0) ) {
       		// return timestamp at end of wave zero crossing
		//printf("zero detected\n");
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
	double b = sqrt(2.0)*oldomega;
	double c = oldomega*oldomega;
	double e;
	double omega = oldomega;

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
                        float freqfound = 0.f;
                        float notefound = 0.f;
                        //float diff = 0.f;
			float nearestnotehz = 0.f;


                        if(pperiod>0 && fabs(in) > 0.0005) {
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
				nper = fs / nearestnotehz;
				tper = 1.0 / nearestnotehz;
				omega = 2.0 * M_PI / nper;
				b = sqrt(2.0)*omega;
				c = omega*omega;

				if (fabs(omega - oldomega) > 0.001) {
					oldomega = omega;
					e2 = tper;
        				t0 = 0.0;
					t1 = t0 + e2;
				}

				// read timer and calculate loop error
				e = in - t1;

				//update loop
				t0 = t1;
				t1 += b*e + e2;
				e2 += c*e;

				Zamtunerdsp::meter = (t1-t0)*50.0;
				printf("meter =\t%f\n", meter);
				printf("t1 =\t%f\n", t1);
				printf("e2 =\t%f\n", e2);
				printf("freqfound =\t%f\n", freqfound);
				printf("nearestnotehz =\t%f\n", nearestnotehz);
				printf("\n");
                        } else {
                                Zamtunerdsp::fundamental = -1.f;
                                Zamtunerdsp::meter = 0.f;
                                //printf("WtF pitch\n");
        		}

			//update sample counts
			n0 = n1;
                }

                *(pfOutput++)=in;        
		n1 += nper;

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
        Zamtunerdsp::noverlap = 6;

        InstantiatePitchDetector(&pdetector, fmembvars, N, fsamp);
}

}
