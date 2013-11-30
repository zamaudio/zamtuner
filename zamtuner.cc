/* zamtuner.cc
 *
 * Copyright (C) 2013 Damien Zammit <damien@zamaudio.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "zamtunerdsp.h"

#define MTR_URI "http://zamaudio.com/lv2/zamtuner#zamtuner"
#define HIST_LEN (751)

using namespace LV2M;

typedef enum {
	MTR_INPUT0   = 0,
	MTR_OUTPUT0  = 1,
	MTR_LEVEL0   = 2,
	MTR_FUND0    = 3
} PortIndex;

typedef struct {
	float  rlgain;
	float  p_refl;
	float* reflvl;

	Zamtunerdsp *mtr[2];

	float* level[2];
	float* fund[2];
	float* input[2];
	float* output[2];
	float* peak[2];
	float* hold;

	int chn;
	float peak_max[2];
	float peak_hold;

	double rate;
	bool ui_active;
	int follow_transport_mode; // bit1: follow start/stop, bit2: reset on re-start.

	bool tranport_rolling;
	bool ebu_integrating;
	bool dbtp_enable;

	float *radarS, radarSC;
	float *radarM, radarMC;
	int radar_pos_cur, radar_pos_max;
	uint32_t radar_spd_cur, radar_spd_max;
	int radar_resync;
	uint64_t integration_time;
	bool send_state_to_ui;
	uint32_t ui_settings;
	float tp_max;

	int histM[HIST_LEN];
	int histS[HIST_LEN];
	int hist_maxM;
	int hist_maxS;

} LV2meter;


static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	LV2meter* self = (LV2meter*)calloc(1, sizeof(LV2meter));

	if (!self) return NULL;

        else {
                self->chn = 1;
                self->mtr[0] = new Zamtunerdsp();
                static_cast<Zamtunerdsp *>(self->mtr[0])->init(rate);
        }	

	self->p_refl = -18.0;
	self->rlgain = 1.f;

	self->peak_max[0] = 0;
	self->peak_max[1] = 0;
	self->peak_hold   = 0;

	return (LV2_Handle)self;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	LV2meter* self = (LV2meter*)instance;

	switch ((PortIndex)port) {
	case MTR_INPUT0:
		self->input[0] = (float*) data;
		break;
	case MTR_OUTPUT0:
		self->output[0] = (float*) data;
		break;
	case MTR_LEVEL0:
		self->level[0] = (float*) data;
		break;
	case MTR_FUND0:
		self->fund[0] = (float*) data;
		break;
	}
}

static void
run(LV2_Handle instance, uint32_t n_samples)
{
	LV2meter* self = (LV2meter*)instance;

        if (self->p_refl != -18.0) {
                self->p_refl = -18.0;
                self->rlgain = powf (10.0f, 0.05f * (self->p_refl + 18.0));
        }

	int c;
	for (c = 0; c < self->chn; ++c) {

		float* const input  = self->input[c];
		float* const output = self->output[c];

		self->mtr[c]->process(input, n_samples);

		*self->level[c] = self->rlgain * self->mtr[c]->readfine();
		*self->fund[c] = self->mtr[c]->readnote();

		if (input != output) {
			memcpy(output, input, sizeof(float) * n_samples);
		}
	}
}

static void
cleanup(LV2_Handle instance)
{
	LV2meter* self = (LV2meter*)instance;
	for (int c = 0; c < self->chn; ++c) {
        	fft_des(self->mtr[c]->fmembvars);
        	free(self->mtr[c]->buffer.cbi);
        	free(self->mtr[c]->buffer.cbf);
        	free(self->mtr[c]->pdetector.cbwindow);
        	free(self->mtr[c]->pdetector.acwinv);
		delete self->mtr[c];
	}

	free(instance);
}

const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor0 = { 
	MTR_URI, 
	instantiate, 
	connect_port,
	NULL, 
	run, 
	NULL,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	return &descriptor0;
}


