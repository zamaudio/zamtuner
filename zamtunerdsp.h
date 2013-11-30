extern "C" {
#include "circular_buffer.h"
#include "pitch_detector.h"
#include "fft.h"
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

        float nper;
        float tper;
        float oldomega;
        float e2;
        float t0;
        float t1;
        float n0;
        float n1;

private:

};

};
