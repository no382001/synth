// includes
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <complex>
#include <raylib.h>
#include <mutex>
#include <stdint.h>

// defines
#define PI2 M_PI * 2
#define RINGBUF_SIZE 2048
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 700

typedef struct {
    size_t size_bytes;
    size_t n_elements;
    float *base;
    float *end;
    float *curr;
} RingBuffer;


int allocate_ringbuffer(RingBuffer* buf, size_t n_elements) {
    buf->size_bytes = n_elements*sizeof(float);
    buf->n_elements = n_elements;
    buf->base = (float*)calloc(n_elements,sizeof(float));
    
    if(!buf->base) return 1;
    
    buf->curr = buf->base;
    buf->end = buf->base + n_elements;
    return 0;
}

void ringbuffer_push_back(RingBuffer* buf, float* data, size_t n_elements, size_t interleaved) {
    size_t cnt = 0;
    while (n_elements-- > 0 ){
        *(buf->curr) = data[cnt++ * interleaved];
        buf->curr++;
        buf->curr -= buf->n_elements * (buf->curr == buf->end);
    }
    
}
void fft(float *data, size_t stride, std::complex<float>* out, size_t n) {
    if(n<=1) {
        out[0]=std::complex<float>{data[0],0}; // base case
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

void magnitude(std::complex<float>* dft,float* mag,size_t n) {
    for(size_t i = 0; i < n; i++) {
        mag[i] = std::abs(dft[i]);
    }
}

RingBuffer buf{};
int memchanged = 0;
unsigned int channels;
unsigned int samplesize;
std::mutex lock;
unsigned int frameslast = 0;
void *buffer_g;
unsigned int frames_g;
float notes[] = {0, 80, 120, 180, 240, 320, 520, 820, 1100, 1350, 1600, 2100, 2600, 3200, 3600, 4000, 4600, 5200, 5750, 6500, 8000, 11000, 20000};
#define NOTES (sizeof(notes) / sizeof(float))
#define RECT_W (WINDOW_WIDTH / (NOTES-1))

void stream_callback(void *bufferData, unsigned int frames) {
    /*if (frames != frameslast)
        printf("%u frames\n", frames);*/
    
    frameslast = frames;
    lock.lock(); // im not sure if we need the locks, it should be thread safe right?
    memchanged = 1;
    buffer_g = bufferData;
    frames_g = frames;
    lock.unlock();
}


int main(int argc, char **argv)
{
    size_t indices[NOTES];
    float notelevelmax[NOTES - 1];
    
    SetTraceLogLevel(LOG_WARNING);
    InitAudioDevice();
    
    if (argc < 2) {
        fprintf(stderr, "Provide input file");
        return 1;
    }
    
    Music music = LoadMusicStream(argv[1]);
    float musiclength = GetMusicTimeLength(music);
    music.looping = false;

    for (size_t i = 0; i < NOTES; i++) {
        indices[i] = (int)notes[i] * RINGBUF_SIZE / music.stream.sampleRate;
    }
    
    
    channels = music.stream.channels;
    samplesize = music.stream.sampleSize;

    allocate_ringbuffer(&buf, RINGBUF_SIZE);
    
    std::complex<float> *fftres = (std::complex<float> *)malloc(sizeof(std::complex<float>) * buf.n_elements);
    float *mags = (float *)malloc(buf.n_elements / 2 * sizeof(float));
    
    AttachAudioStreamProcessor(music.stream, stream_callback);
    PlayMusicStream(music);
    InitWindow((int)buf.n_elements, WINDOW_HEIGHT, "");
    SetWindowSize((NOTES - 1) * RECT_W, WINDOW_HEIGHT);
    SetTargetFPS(60);
    
    float maxmagalltime = 0;
    float maxmag = 0;
    size_t maxmagidx = 0;
    float maxmagrelpos = 0;
    int key;
    int indexcnt;
    int height;
    float heightrel;
    
    Color c = {0,0,0,255};
    
    while (!WindowShouldClose()) {

        UpdateMusicStream(music);
        ClearBackground(GRAY);
        lock.lock();
        if (memchanged) {
            ringbuffer_push_back(&buf, (float *)buffer_g, frames_g, channels);
            memchanged = 0;
        }
        lock.unlock();
        memset(notelevelmax, 0, (NOTES - 1) * sizeof(float));
        
        fft(buf.base, 1, fftres, buf.n_elements);
        
        magnitude(fftres, mags, buf.n_elements / 2); // func

        indexcnt = 0;
        maxmag = 0;
        for (size_t i = indices[0]; i < indices[NOTES - 1]; i++) {
            if (i == indices[indexcnt + 1])
                indexcnt++;
            if (mags[i] > notelevelmax[indexcnt]) {
                notelevelmax[indexcnt] = mags[i];
                if(mags[i]>maxmagalltime){
                    maxmagalltime = mags[i];
                } 
                if (mags[i] > maxmag) {
                    maxmag = mags[i];
                    maxmagidx = indexcnt;
                }
            }
        }
        //relative position within frequencies scaled from 0 to 1
        maxmagrelpos = (float)maxmagidx/(float)(NOTES-1); 
        BeginDrawing();

        for (size_t i = 0; i < NOTES - 1; i++) {
            height = (int)(notelevelmax[i] / maxmagalltime * WINDOW_HEIGHT);
            height = (height < WINDOW_HEIGHT) * height + (height > WINDOW_HEIGHT) * WINDOW_HEIGHT;
            heightrel = (height / (float)WINDOW_HEIGHT) * 0.8f;
            c.r = (unsigned char)(255 * heightrel);
            c.g = (unsigned char)(200 * (1 - heightrel));
            DrawRectangle((int)i * RECT_W, WINDOW_HEIGHT - height, RECT_W, height, c);
        }

        EndDrawing();
    }

    StopMusicStream(music);
    DetachAudioStreamProcessor(music.stream, stream_callback);
    free(buf.base);
    UnloadMusicStream(music);
    CloseAudioDevice();
    free(fftres);
    free(mags);
    CloseWindow();
}