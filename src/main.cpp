#include "utils.h"

const int SAMPLE_RATE = 8000;
const int BUFFER_SIZE = 4096;

typedef struct {
  float current_step;
  float step_size;
  float volume;
} oscillator;

oscillator oscillate(float rate, float volume) {
    oscillator o = {
        .current_step = 0,
        .step_size = (2 * M_PI) / rate,
        .volume = volume,
    };
    return o;
}

float next(oscillator *os) {
    //float ret = sinf(os->current_step) >= 0 ? 1.0f : -1.0f;
    float ret = sinf(os->current_step);
    os->current_step += os->step_size;
    return ret * os->volume;
}

oscillator *OSC;
RingBuffer rbuf{};
std::mutex mtx;

// graphics defines
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define DEFAULT_OFFSET_X 1
#define DEFAULT_OFFEST_Y 25

#define SCALING 3

// sound defines
#define UNIT 0.2f

int countntat = 0;
void oscillator_callback(void *userdata, Uint8 *stream, int len) {
    for (int i = 0; i < len; i++) {
        float v = next(OSC);
        stream[i] = (uint8_t)((v * 127.5f) + 127.5f); // convert from float [-1.0, 1.0] to unsigned 8-bit [0, 255]
    }

    ringbuffer_push_back(&rbuf, (uint8_t*)stream, len, 1);
}

int main(int argc, char *argv)
{
    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        std::cout << "SDL_INIT_VIDEO aaa" << std::endl;
    }

    auto osc_vol = 0.2f;
    auto osc_note_offset = 440.00f;
    auto osc_note = (float)SAMPLE_RATE / osc_note_offset;
    oscillator osc = oscillate(osc_note, osc_vol);
    OSC = &osc;
    
    allocate_ringbuffer(&rbuf, RINGBUF_SIZE);

    SDL_AudioSpec spec = {
        .freq = SAMPLE_RATE,
        .format = AUDIO_U8,
        .channels = 1,
        .samples = 512,
        .callback = oscillator_callback,
    };

    if (SDL_OpenAudio(&spec, NULL) < 0) {
        printf("Failed to open Audio Device: %s\n", SDL_GetError());
        return 1;
    }

    SDL_PauseAudio(0);

    SDL_Event event;
    SDL_Window *window = SDL_CreateWindow("anyad",DEFAULT_OFFSET_X,DEFAULT_OFFEST_Y,DEFAULT_WIDTH,DEFAULT_HEIGHT,0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_PRESENTVSYNC);

    SDL_Init(SDL_INIT_VIDEO);

    while (true) {

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                SDL_DestroyRenderer(renderer);
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;

            // very sloppy osc mutation
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_UP) {
                    osc_vol += UNIT;
                    osc = oscillate(osc_note, osc_vol);
                } else if (e.key.keysym.sym == SDLK_DOWN){
                    osc_vol -= UNIT;
                    osc = oscillate(osc_note, osc_vol); 
                } else if (e.key.keysym.sym == SDLK_RIGHT){
                    osc_note_offset += UNIT*10;
                    osc = oscillate((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                } else if (e.key.keysym.sym == SDLK_LEFT){
                    osc_note_offset -= UNIT*10;
                    osc = oscillate((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                }
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        auto i = 1;
        while (i < 512/2) {
            int x1 = (i-1) * SCALING;
            int y1 = rbuf.base[i-1] * SCALING;

            int x2 =  i * SCALING;
            int y2 = rbuf.base[i] * SCALING;

            SDL_RenderDrawLine(renderer,x1,y1,x2,y2);
            
            SDL_RenderDrawPoint(renderer, x1, y1);
            i++;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16*2); // probably not needed w/ SDL_RENDERER_PRESENTVSYNC
        }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}