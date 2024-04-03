#include "utils.h"

int allocate_ringbuffer(RingBuffer* buf, size_t n_elements) {
    buf->size_bytes = n_elements*sizeof(uint8_t);
    buf->n_elements = n_elements;
    buf->base = (uint8_t*)calloc(n_elements,sizeof(uint8_t));
    
    if(!buf->base) return 1;
    
    buf->curr = buf->base;
    buf->end = buf->base + n_elements;
    return 0;
}

void ringbuffer_push_back(RingBuffer* buf, uint8_t* data, size_t n_elements, size_t interleaved) {
    uint8_t cnt = 0;
    while (n_elements-- > 0 ){
        *(buf->curr) = data[cnt++ * interleaved];
        buf->curr++;
        buf->curr -= buf->n_elements * (buf->curr == buf->end);
    }
}

void fft(float *data, size_t stride, std::complex<float>* out, size_t n) {
    if(n <= 1) {
        out[0] = std::complex<float>{data[0], 0}; // base case
        return;
    }

    fft(data, stride * 2, out, n / 2); // for first half
    fft(data+stride,
        stride * 2,
        out + n / 2,
        n / 2); // for second half

    // butterfly operation
    for (size_t k = 0; k < n / 2; k++) {
        float t = (float)k / n;
        std::complex<float> v = std::exp(std::complex<float>{0, -PI2 * t}) * out[k + n / 2]; // calculate twiddle factor
        std::complex<float> e = out[k]; // store value
        out[k] = e + v; // update first half
        out[k + n / 2] = e - v; // update second half
    }
}