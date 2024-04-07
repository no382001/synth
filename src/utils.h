#pragma once
#define SDL_MAIN_HANDLED

#include "SDL_audio.h"
#include "SDL_timer.h"
#include <SDL.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <complex>
#include <mutex>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <iostream>

#define PI2 (M_PI * 2)
#define RINGBUF_SIZE 1024

typedef struct {
    size_t size_bytes;
    size_t n_elements;
    uint8_t *base;
    uint8_t *end;
    uint8_t *curr;
} RingBuffer;

int allocate_ringbuffer(RingBuffer* buf, size_t n_elements);
void ringbuffer_push_back(RingBuffer* buf, uint8_t* data, size_t n_elements, size_t interleaved);
void fft(float *data, size_t stride, std::complex<float>* out, size_t n);