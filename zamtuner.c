/* zamtuner.c  ZamTuner - LV2 tuner zamtuner
 * Copyright (c) 2013  Damien Zammit <damien@zamaudio.com>
 */

#include "zamtuner.h"
#include <stdio.h>

static LV2_Descriptor *ZamTunerDescriptor = NULL;

static void cleanup(LV2_Handle instance)
{
	ZamTuner * zamtuner=(ZamTuner*)instance;
	fft_des(zamtuner->fmembvars);
 	free(zamtuner->buffer.cbi);
	free(zamtuner->buffer.cbf);
	free(zamtuner->pdetector.cbwindow);
	free(zamtuner->pdetector.acwinv);
	free(zamtuner);
}
static void connectPort(LV2_Handle instance, uint32_t port, void *data)
{
	ZamTuner *zamtuner = (ZamTuner *)instance;
	switch (port) {
		case AT_AUDIO_IN:
			zamtuner->p_InputBuffer=(float*) data;
			break;
		case AT_AUDIO_OUT:
			zamtuner->p_OutputBuffer=(float*) data;
			break;
		case AT_FUNDAMENTAL:
			zamtuner->fundamental=(float*) data;
			break;
		case AT_FINETUNE:
			zamtuner->finetune=(float*) data;
			break;
	}
}

static LV2_Handle instantiate(const LV2_Descriptor *descriptor,
	    double s_rate, const char *path,
	    const LV2_Feature * const* features)
{
	ZamTuner *membvars = (ZamTuner *)malloc(sizeof(ZamTuner));
	InstantiateCircularBuffer(&membvars->buffer,s_rate);
	unsigned long N=membvars->buffer.cbsize;
	membvars->fmembvars = fft_con(N);
	membvars->fs = s_rate;
	membvars->noverlap = 4;
	
	InstantiatePitchDetector(&membvars->pdetector, membvars->fmembvars, N, s_rate);
	return membvars;
}

inline void IncrementPointer(CircularBuffer * buffer) {
	// Input write pointer logic
	buffer->cbiwr++;
	if (buffer->cbiwr >= buffer->cbsize) {
		buffer->cbiwr = 0;
	}
}

static void run(LV2_Handle instance, uint32_t sample_count)
{
	ZamTuner* zamtuner = (ZamTuner *)instance;
	
	unsigned long N = zamtuner->buffer.cbsize;
	unsigned long Nf = zamtuner->buffer.corrsize;
	float fs = zamtuner->fs;
	
	const float* pfInput=zamtuner->p_InputBuffer;
	float* pfOutput=zamtuner->p_OutputBuffer;
	
	unsigned long lSampleIndex;
	for (lSampleIndex = 0; lSampleIndex < sample_count; lSampleIndex++)  {
		// load data into circular buffer
		float in = (float) *(pfInput++);
		
		zamtuner->buffer.cbi[zamtuner->buffer.cbiwr] = in;
		
		IncrementPointer(&zamtuner->buffer);
		
		// Every N/noverlap samples, run pitch estimation / manipulation code
		if ((zamtuner->buffer.cbiwr)%(N/zamtuner->noverlap) == 0) {
			//  ---- Calculate pitch and confidence ----
			float pperiod=get_pitch_period(&zamtuner->pdetector, obtain_autocovariance(&zamtuner->pdetector,zamtuner->fmembvars,&zamtuner->buffer,N),Nf,fs);
			int nearestnote = 0;
			float freqfound = 0.f;
			float notefound = 0.f;
			float diff = 0.f;
			char note = ' ';
			char notesh = ' ';

			if(pperiod>0 && fabs(in) > 0.001) {
				freqfound = 1.f/pperiod;
				notefound = 12.0*log(freqfound/440)/log(2.0)+49.0;
				if (notefound - (int)notefound > 0.5) {
					nearestnote = (int)notefound + 1;
				} else {
					nearestnote = (int)notefound;
				}
				diff = nearestnote - notefound;
				nearestnote = (nearestnote - 49 + 12) % 12;
				switch (nearestnote) {
					case 0:
						note = 'A';
						notesh = ' ';
						break;
					case 1:
						note = 'A';
						notesh = '#';
						break;
					case 2:
						note = 'B';
						notesh = ' ';
						break;
					case 3:
						note = 'C';
						notesh = ' ';
						break;
					case 4:
						note = 'C';
						notesh = '#';
						break;
					case 5:
						note = 'D';
						notesh = ' ';
						break;
					case 6:
						note = 'D';
						notesh = '#';
						break;
					case 7:
						note = 'E';
						notesh = ' ';
						break;
					case 8:
						note = 'F';
						notesh = ' ';
						break;
					case 9:
						note = 'F';
						notesh = '#';
						break;
					case 10:
						note = 'G';
						notesh = ' ';
						break;
					case 11:
						note = 'G';
						notesh = '#';
						break;
				}
				
				//printf("Found note : %c%c %f %d\n", note, notesh, diff, nearestnote );	
				*(zamtuner->finetune) = diff;
				*(zamtuner->fundamental) = nearestnote; 

			} else { 
				*(zamtuner->finetune) = 0.f;
				*(zamtuner->fundamental) = -1.f;
				//printf("WtF pitch\n");
			}
		}
		
		*(pfOutput++)=in;
		
	}
}

static void init()
{
	ZamTunerDescriptor =
	 (LV2_Descriptor *)malloc(sizeof(LV2_Descriptor));

	ZamTunerDescriptor->URI = ZAMTUNER_URI;
	ZamTunerDescriptor->activate = NULL;
	ZamTunerDescriptor->cleanup = cleanup;
	ZamTunerDescriptor->connect_port = connectPort;
	ZamTunerDescriptor->deactivate = NULL;
	ZamTunerDescriptor->instantiate = instantiate;
	ZamTunerDescriptor->run = run;
	ZamTunerDescriptor->extension_data = NULL;
}

LV2_SYMBOL_EXPORT
const LV2_Descriptor *lv2_descriptor(uint32_t index)
{
	if (!ZamTunerDescriptor) init();

	switch (index) {
	case 0:
		return ZamTunerDescriptor;
	default:
		return NULL;
	}
}
