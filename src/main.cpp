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

#define VISUAL_HEIGHT 200
#define VISUAL_WIDTH 200

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

void magnitude(std::complex<float>* dft,float* mag,size_t n) {
    for(size_t i = 0; i < n; i++) {
        mag[i] = std::abs(dft[i]);
    }
}

RingBuffer buf{};
int memchanged = 0;
unsigned int channels;
unsigned int samplesize;
//std::mutex lock;
unsigned int frameslast = 0;
void *buffer_g;
unsigned int frames_g;
float notes[] = {0, 80, 120, 180, 240, 320, 520, 820, 1100, 1350, 1600, 2100, 2600, 3200, 3600, 4000, 4600, 5200, 5750, 6500, 8000, 11000, 20000};
#define NOTES (sizeof(notes) / sizeof(float))
#define RECT_W (VISUAL_WIDTH / (NOTES-1))

void stream_callback(void *bufferData, unsigned int frames) {
    frameslast = frames;
    memchanged = 1;
    buffer_g = bufferData;
    frames_g = frames;
}

float frequency = 440.0f;
float audioFrequency = 440.0f;
float oldFrequency = 440.0f;
float sineIdx = 0.0f;

void sine_wave(void *buffer, unsigned int frames) {
    audioFrequency = frequency + (audioFrequency - frequency)*0.95f;
    float incr = audioFrequency/44100.0f;
    short *d = (short *)buffer;
    for (unsigned int i = 0; i < frames; i++) {
        d[i] = (short)(12000.0f*sinf(2*PI*sineIdx));
        sineIdx += incr;
        if (sineIdx > 1.0f) sineIdx -= 1.0f;
    }
}

#define MAX_SAMPLES               512
#define MAX_SAMPLES_PER_UPDATE   4096

void mode1() {
    size_t indices[NOTES];
    float notelevelmax[NOTES - 1];

    AudioStream as;
    as = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(as, sine_wave);
    
    for (size_t i = 0; i < NOTES; i++) {
        indices[i] = (int)notes[i] * RINGBUF_SIZE / as.sampleRate;
    }
    
    channels = as.channels;
    samplesize = as.sampleSize;

    allocate_ringbuffer(&buf, RINGBUF_SIZE);
    
    std::complex<float> *fftres = (std::complex<float> *)malloc(sizeof(std::complex<float>) * buf.n_elements);
    float *mags = (float *)malloc(buf.n_elements / 2 * sizeof(float));
    
    AttachAudioStreamProcessor(as, stream_callback);
    PlayAudioStream(as);
    
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "synth");
    SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
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

    short *data = (short *)malloc(sizeof(short) * MAX_SAMPLES);
    Vector2 mousePosition = {-100.0f, -100.0f};
    int waveLength = 1;
    Vector2 position = {0, 0};


    while (!WindowShouldClose()) {
        // mouse and freq changer

        mousePosition = GetMousePosition();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            float fp = (float)(mousePosition.y);
            frequency = 40.0f + (float)(fp);

            float pan = (float)(mousePosition.x) / (float)WINDOW_WIDTH;
            SetAudioStreamPan(as, pan);
        }

        if (frequency != oldFrequency)
        {
            waveLength = (int)(22050 / frequency);
            if (waveLength > MAX_SAMPLES / 2)
                waveLength = MAX_SAMPLES / 2;
            if (waveLength < 1)
                waveLength = 1;

            for (int i = 0; i < waveLength * 2; i++) {
                data[i] = (short)(sinf(((2 * PI * (float)i / waveLength))) * 32000);
            }
            for (int j = waveLength * 2; j < MAX_SAMPLES; j++) {
                data[j] = (short)0;
            }
            oldFrequency = frequency;
        }

        // fft shjit

        ClearBackground(GRAY);

        if (memchanged) {
            ringbuffer_push_back(&buf, (float *)buffer_g, frames_g, channels);
            memchanged = 0;
        }

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
            height = (int)(notelevelmax[i] / maxmagalltime * VISUAL_HEIGHT);
            height = (height < VISUAL_HEIGHT) * height + (height > VISUAL_HEIGHT) * VISUAL_HEIGHT;
            heightrel = (height / (float)VISUAL_HEIGHT) * 0.8f;
            c.r = (unsigned char)(255 * heightrel);
            c.g = (unsigned char)(200 * (1 - heightrel));
            DrawRectangle((int)i * RECT_W +100, VISUAL_HEIGHT - height, RECT_W, height, c);
        }

        auto curr = buf.base;
        auto i = 1;
        while (i < buf.n_elements) {
            int x1 = (800 * (i - 1)) / buf.n_elements;
            int x2 = (800 * i) / buf.n_elements;
            int y1 = 400 / 2 + (int)(curr[(i - 1 + buf.n_elements) % buf.n_elements] * (800 / 4));
            int y2 = 400 / 2 + (int)(curr[i] * (800 / 4));
            DrawLine(x1, y1, x2, y2, BLACK);
            i++;
        }

        EndDrawing();
    }

    //StopMusicStream(music);
    DetachAudioStreamProcessor(as, stream_callback);
    free(buf.base);
    //UnloadMusicStream(music);
    CloseAudioDevice();
    free(fftres);
    free(mags);
    CloseWindow();
}

void mode2(int argc, char **argv){
    size_t indices[NOTES];
    float notelevelmax[NOTES - 1];
    
    SetTraceLogLevel(LOG_WARNING);
    InitAudioDevice();

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

        if (memchanged) {
            ringbuffer_push_back(&buf, (float *)buffer_g, frames_g, channels);
            memchanged = 0;
        }

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
            height = (int)(notelevelmax[i] / maxmagalltime * VISUAL_HEIGHT);
            height = (height < VISUAL_HEIGHT) * height + (height > VISUAL_HEIGHT) * VISUAL_HEIGHT;
            heightrel = (height / (float)VISUAL_HEIGHT) * 0.8f;
            c.r = (unsigned char)(255 * heightrel);
            c.g = (unsigned char)(200 * (1 - heightrel));
            DrawRectangle((int)i * RECT_W +100, VISUAL_HEIGHT - height, RECT_W, height, c);
        }

        auto curr = buf.base;
        auto i = 1;
        while (i < buf.n_elements) {
            int x1 = (800 * (i - 1)) / buf.n_elements;
            int x2 = (800 * i) / buf.n_elements;
            int y1 = 400 / 2 + (int)(curr[(i - 1 + buf.n_elements) % buf.n_elements] * (800 / 4));
            int y2 = 400 / 2 + (int)(curr[i] * (800 / 4));
            DrawLine(x1, y1, x2, y2, BLACK);
            i++;
        }

        EndDrawing();
    }

}

void AudioInputCallback(void *buffer, unsigned int frames) {
    audioFrequency = frequency + (audioFrequency - frequency)*0.95f;

    float incr = audioFrequency/44100.0f;
    short *d = (short *)buffer;

    for (unsigned int i = 0; i < frames; i++)
    {
        d[i] = (short)(32000.0f*sinf(2*PI*sineIdx));
        sineIdx += incr;
        if (sineIdx > 1.0f) sineIdx -= 1.0f;
    }
}


int main(int argc, char **argv) {
    
    SetTraceLogLevel(LOG_WARNING);
    InitAudioDevice();

    if (argc < 2) {
        mode1();
    } else {
        mode2(argc,argv);
    }
}

// put the mutex back