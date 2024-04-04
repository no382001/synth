#include "utils.h"

const int SAMPLE_RATE = 8000;
const int BUFFER_SIZE = 4096;

typedef struct {
  float current_step;
  float step_size;
  float volume;
} oscillator;

oscillator* create_osc(float rate, float volume) {
    return new oscillator{
        .current_step = 0,
        .step_size = (2 * M_PI) / rate,
        .volume = volume,
    };
}

float next(oscillator *os) {
    //float ret = sinf(os->current_step) >= 0 ? 1.0f : -1.0f;
    float ret = sinf(os->current_step);
    os->current_step += os->step_size;
    return ret * os->volume;
}

std::vector<oscillator*> oscv;
RingBuffer rbuf{};
std::mutex mtx;


#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define SCALING 3
#define UNIT 0.01f

bool on = false;

void oscillator_callback(void *userdata, Uint8 *stream, int len) {
    for (int i = 0; i < len; i++) {
        
        if (on){
            auto v = std::accumulate(oscv.begin(), oscv.end(), .0f,
                [](float acc, oscillator* x) { return (acc + next(x))/oscv.size(); });
            stream[i] = (uint8_t)((v * 127.5f) + 127.5f);
        } else {
            float v = next(oscv[0]);
            stream[i] = (uint8_t)((v * 127.5f) + 127.5f) ;
        }
    }

    ringbuffer_push_back(&rbuf, (uint8_t*)stream, len, 1);
}

int main() {

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    auto osc_vol = 0.5f;
    float osc_note_offset = 440.00f;
    auto osc_note = (float)SAMPLE_RATE / osc_note_offset;

    for (size_t i = 0; i < 2; i++) {
        oscv.push_back(create_osc((float)SAMPLE_RATE/(osc_note_offset - 10*i), osc_vol));
    }
    
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
    SDL_Window *window = SDL_CreateWindow("anyad",0,0,DEFAULT_WIDTH,DEFAULT_HEIGHT,0);
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
                    oscv[0] = create_osc(osc_note, osc_vol);
                } else if (e.key.keysym.sym == SDLK_DOWN){
                    osc_vol -= UNIT;
                    oscv[0] = create_osc(osc_note, osc_vol); 
                } else if (e.key.keysym.sym == SDLK_RIGHT){
                    osc_note_offset += UNIT*10;
                    oscv[0] = create_osc((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                } else if (e.key.keysym.sym == SDLK_LEFT){
                    osc_note_offset -= UNIT*10;
                    oscv[0] = create_osc((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                } else if (e.key.keysym.sym == SDLK_RSHIFT){
                    on = !on;
                }
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        if (on)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        else
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        auto i = 1;
        // miert kell lefelezni hogy eltunjon az `overlap`? nem ugyanugy csak rateszed a kovetkezo buffert a ringben?
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