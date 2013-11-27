#include "circular_buffer.h"

void InstantiateCircularBuffer(CircularBuffer* buffer, unsigned long SampleRate) {
	if (SampleRate>=88200) {
		buffer->cbsize = 65536;
	}
	else {
		buffer->cbsize = 32768;
	}
	buffer->corrsize = buffer->cbsize / 2 + 1;

	buffer->cbi = (float*) calloc(buffer->cbsize, sizeof(float));
	buffer->cbf = (float*) calloc(buffer->cbsize, sizeof(float));
	buffer->cbiwr = 0;
}
