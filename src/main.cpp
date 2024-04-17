#define SDL_MAIN_HANDLED
#include "audio.h"
#include "ringbuffer.h"
#include <atomic>

#define FPS_INTERVAL 1000.0 //ms.

Uint32 fps_lasttime = SDL_GetTicks();
Uint32 fps_current;
Uint32 fps_frames = 0;

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define DEFAULT_OFFSET_X 1
#define DEFAULT_OFFSET_Y 25
#define SCALING 13.0
#define UNIT 0.01f

const int SAMPLE_RATE = 8000;
const int BUFFER_SIZE = 4096;
const int SAMPLE_FRAME_SIZE = 512;

oscillator* create_osc(float rate, float volume) {
    return new oscillator{
        .current_step = 0,
        .step_size = (2 * M_PI) / rate,
        .volume = volume,
    };
}

float next_sin(oscillator *os) {
    float ret = sinf(os->current_step);
    os->current_step += os->step_size;
    return ret * os->volume;
}

float next_rect(oscillator *os) {
    float ret = sinf(os->current_step) >= 0 ? 1.0f : -1.0f;
    os->current_step += os->step_size;
    return ret * os->volume;
}

#include <chrono>

static rb_t* global_rb = nullptr;
std::atomic<int> sample_count{0};

const int BPM = 100;
const float BEAT_DURATION = 60.0f / BPM;
const int SAMPLES_PER_BEAT = SAMPLE_RATE * BEAT_DURATION;

std::chrono::steady_clock::time_point last_beat_time = std::chrono::steady_clock::now();

void spec_callback(void *userdata, Uint8 *stream, int len) {
    assert(len == SAMPLE_FRAME_SIZE * sizeof(float));

    // current time and time since last beat
    auto now = std::chrono::steady_clock::now();
    float time_since_last_beat = std::chrono::duration<float>(now - last_beat_time).count();

    // calculate number of beats elapsed
    int beats_elapsed = time_since_last_beat / BEAT_DURATION;

    if (beats_elapsed >= 1) {
        int required_samples = SAMPLES_PER_BEAT * beats_elapsed;
        if (global_rb && rb_can_read(global_rb)) {
            rb_read(global_rb, reinterpret_cast<char*>(stream), len);
            sample_count.fetch_add(SAMPLE_FRAME_SIZE, std::memory_order_relaxed);
            last_beat_time += std::chrono::milliseconds(static_cast<int>(beats_elapsed * BEAT_DURATION * 1000));
        }
    }
}

oscillator* osc = nullptr;
float* postproc;

int cells[8][10]{ 0 };

int par_thread(void *data) {
    float par_thr_b[SAMPLE_FRAME_SIZE]{};    

    auto osc_vol = 0.5f;
    float osc_note_offset = 440.00f;
    auto osc_note = (float)SAMPLE_RATE / osc_note_offset;
    auto localosc = create_osc((float)SAMPLE_RATE/osc_note_offset, osc_vol);

    int row = 1;
    do {
        if (global_rb && rb_can_write(global_rb)){
            float n = 0;

            for (int i = 0; i < 8; i++){
                if(cells[i][row] == 1 || cells[i][row] == 2){ // if the button is toggled
                    for (int j = 0; j < SAMPLE_FRAME_SIZE; j++) {
                        if (cells[i][row] == 1)
                            par_thr_b[j] += next_sin(localosc); // sum the toggled channels
                        else
                            par_thr_b[j] += next_rect(localosc);
                    }
                    n++; // inc the numebr of mixed channels
                    localosc = create_osc((float)SAMPLE_RATE/osc_note_offset, osc_vol); // reset the osc so that the phase stays the same
                }
            }
            
            if (n != 0){
                for (int i = 0; i < SAMPLE_FRAME_SIZE; i++){ // average the output buffer
                    par_thr_b[i] /= n;
                }
            } else {
                for (int i = 0; i < SAMPLE_FRAME_SIZE; i++){
                    par_thr_b[i] = 0;
                }
            }
            
            auto r = rb_write(global_rb, reinterpret_cast<char *>(par_thr_b), SAMPLE_FRAME_SIZE * sizeof(float));
            
            for (int j = 0; j < SAMPLE_FRAME_SIZE; j++) {
                postproc[j] = par_thr_b[j];
            }

            row++; // inc the row
            //process_row_count.fetch_add(1,std::memory_order_relaxed);
            
            if (row == 9){
                row = 1;
            }
        }
    } while (1);
}

const int CELL_WIDTH = DEFAULT_WIDTH / 8;
const int CELL_HEIGHT = DEFAULT_HEIGHT / 10;

float curr_cell{1.0};

int main(int argc, char *argv) {
    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        std::cout << "SDL_INIT_EVERYTHING" << std::endl;
    }

    postproc = new float[SAMPLE_FRAME_SIZE];
    auto osc_vol = 0.5f;
    float osc_note_offset = 440.00f;
    auto osc_note = (float)SAMPLE_RATE / osc_note_offset;

    osc = create_osc((float)SAMPLE_RATE/osc_note_offset, osc_vol);

    global_rb = rb_new(SAMPLE_FRAME_SIZE * sizeof(float) * 5);

    // http://forums.libsdl.org/viewtopic.php?p=28652
    // https://en.wikipedia.org/wiki/Tempo#Beats_per_minute

    SDL_AudioSpec spec = {
        .freq = SAMPLE_RATE, // every second
        .format = AUDIO_F32,
        .channels = 1,
        .samples = SAMPLE_FRAME_SIZE, // SAMPLE_RATE / SAMPLE_FRAME_SIZE -> per second
        .callback = spec_callback
    };

    if (SDL_OpenAudio(&spec, NULL) < 0) {
        printf("Failed to open Audio Device: %s\n", SDL_GetError());
        return 1;
    }

    SDL_PauseAudio(0);

    SDL_Thread *thread = SDL_CreateThread(par_thread, "ExampleThread", NULL);

    SDL_Event event;
    SDL_Window *window = SDL_CreateWindow("anyad",DEFAULT_OFFSET_X,DEFAULT_OFFSET_Y,DEFAULT_WIDTH,DEFAULT_HEIGHT,0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window,-1, SDL_RENDERER_PRESENTVSYNC);

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
                        osc = create_osc(osc_note, osc_vol);
                    } else if (e.key.keysym.sym == SDLK_DOWN){
                        osc_vol -= UNIT;
                        osc = create_osc(osc_note, osc_vol); 
                    } else if (e.key.keysym.sym == SDLK_RIGHT){
                        osc_note_offset += UNIT*10;
                        osc = create_osc((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                    } else if (e.key.keysym.sym == SDLK_LEFT){
                        osc_note_offset -= UNIT*10;
                        osc = create_osc((float)SAMPLE_RATE / osc_note_offset, osc_vol);
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    int x, y;
                    SDL_GetMouseState(&x, &y);
                    
                    int col = x / CELL_WIDTH;
                    int row = y / CELL_HEIGHT;

                    cells[col][row]++;
                    if (cells[col][row] == 3){
                        cells[col][row] = 0;
                    }

                    break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

        /* waveform */
        auto i = 1;
        while (i < SAMPLE_FRAME_SIZE) {
            int x1 = (i-1) + DEFAULT_WIDTH/2 - SAMPLE_FRAME_SIZE/2;
            int y1 = postproc[i-1] * SCALING + 40;

            int x2 = i + DEFAULT_WIDTH/2 - SAMPLE_FRAME_SIZE/2;
            int y2 = postproc[i] * SCALING + 40;

            SDL_RenderDrawLine(renderer,x1,y1,x2,y2);
            //SDL_RenderDrawPoint(renderer, x1, y1);
            i++;
        }

        /* clicky table */
        for (int row = 1; row < 10; ++row) {
            for (int col = 0; col < 8; ++col) {
                SDL_Rect cell;
                cell.x = col * CELL_WIDTH;
                cell.y = row * CELL_HEIGHT;
                cell.w = CELL_WIDTH;
                cell.h = CELL_HEIGHT;

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                if (cells[col][row] == 1){
                    SDL_RenderFillRect(renderer, &cell);
                } else if (cells[col][row] == 2){
                    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
                    SDL_RenderFillRect(renderer, &cell);
                } else {
                    SDL_RenderDrawRect(renderer, &cell);
                }

            }
        }

        /* indicator line */

        curr_cell = (sample_count.load(std::memory_order_relaxed) % 9) + 1;

        SDL_RenderDrawLine(renderer,
            0,
            (curr_cell+1)*CELL_HEIGHT - CELL_HEIGHT/2,
            DEFAULT_WIDTH,
            (curr_cell+1)*CELL_HEIGHT - CELL_HEIGHT/2);
        

        /* fps <-- vsync seems to work */ 
        fps_frames++;
        if (fps_lasttime < SDL_GetTicks() - FPS_INTERVAL) {
            fps_lasttime = SDL_GetTicks();
            fps_current = fps_frames;
            fps_frames = 0;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

/* 
- read a text file w/ some symbols in it representing the cached sounds
- start playing them at some speed, figure out what `speed` means
- processing thread, an event thread, audio thread, and gui thread


-> if i want to change the playback speed, wtf do i change? the callback speed of the audiothread?

- how tf do i test how accurate the bpm is?

-> maybe cheat a bit and resample the sound in the processing thread? interpolate in real time? is that a good idea at all?

some people say that you should have diff threads, but olufson says thats not how it should work

*/