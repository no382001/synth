#include "utils.h"
#include "audio.h"

const int SAMPLE_RATE = 8000;
const int BUFFER_SIZE = 4096;

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
std::mutex mtx;

float* postproc;

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define SCALING 3
#define UNIT 0.01f
bool on = false;


std::vector<atomic_sample> g_at;
int framecount = 0;
void oscillator_callback(void *userdata, Uint8 *stream, int len) {
    if (!g_at.empty()){
        auto a = g_at.at(framecount);
        
        if (framecount < g_at.size() - 1){
            framecount++;
        } else {
            framecount = 0;
        }
        
        for (int i = 0; i < len; i++) {
                float v = a.buffer[i];
                stream[i] = (uint8_t)((v * 127.5f) + 127.5f);
                postproc[i] = stream[i];
        }
    }
}

int main() {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    postproc = new float[512];
    auto osc_vol = 0.5f;
    float osc_note_offset = 440.00f;
    auto osc_note = (float)SAMPLE_RATE / osc_note_offset;

    for (size_t i = 0; i < 2; i++) {
        oscv.push_back(create_osc((float)SAMPLE_RATE/(osc_note_offset - 10*i), osc_vol));
    }

    init_sine_wave(g_at);

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

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        auto i = 1;
        // miert kell lefelezni hogy eltunjon az `overlap`? nem ugyanugy csak rateszed a kovetkezo buffert a ringben?
        while (i < 512/2) {
            int x1 = (i-1) * SCALING;
            int y1 = postproc[i-1] * SCALING;

            int x2 =  i * SCALING;
            int y2 = postproc[i] * SCALING;

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