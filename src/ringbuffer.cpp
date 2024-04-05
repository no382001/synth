// https://stackoverflow.com/questions/14703707/circular-ring-buffer-with-blocking-read-and-non-blocking-write
#include <stdio.h>
#include <pthread.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>  

template <typename T>
struct ringbuffer {
    T* m_buffer = nullptr;
    T* m_in = nullptr;
    T* m_out = nullptr;
    int m_size = 0;
    int m_in_size = 0;
    int m_out_size = 0;
    unsigned m_reader_unread = 0;
    unsigned m_writer_next = 0;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_written_cond;

    ringbuffer(int size, T* in, int in_size, T* out, int out_size)
        : m_size(size), m_buffer(new T[size]), m_in(in), m_out(out), m_in_size(in_size), m_out_size(out_size) {
            pthread_mutex_init(&m_mutex,NULL);
            pthread_cond_init(&m_written_cond,NULL);
    }    
    
    ~ringbuffer() {
        delete[] m_buffer;
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_written_cond);
    }

    static void* read_wrapper(void* context) {
        auto rb = static_cast<ringbuffer*>(context);
        do {
            rb->read_to_out();
            rb->print_present_state();
        } while (1);
    }

    static void* write_wrapper(void* context) {
        auto rb = static_cast<ringbuffer*>(context);
        do {
            rb->write_from_in();
        } while (1);
    }

    // write from m_in to the internal ring buffer
    void write_from_in() {
        pthread_mutex_lock(&m_mutex);

        for(size_t i = 0; i < m_in_size; ++i) {
            m_buffer[m_writer_next] = m_in[i];
            m_writer_next = (m_writer_next + 1) % m_size;
            if (m_reader_unread < m_size) {
                ++m_reader_unread;
            }
        }
        pthread_cond_signal(&m_written_cond); // notify the waiting reader
        pthread_mutex_unlock(&m_mutex);
    }

    // read into m_out from the internal ring buffer
    void read_to_out() {
        pthread_mutex_lock(&m_mutex);

        if (m_reader_unread == 0) {
            pthread_cond_wait(&m_written_cond, &m_mutex);
        }
        if (m_reader_unread > 0) {
            int i = 0;
            while (m_reader_unread > 0){
                int pos = m_writer_next - m_reader_unread;
                if (pos < 0) pos += m_size;
                m_out[i] = m_buffer[pos];
                --m_reader_unread;
                i++;
            }
        }
        pthread_mutex_unlock(&m_mutex);
    }

    void print_present_state(bool b = false){
        std::cout << "m_in:   ";
        for (int i = 0; i < 5; i++) {
            std::cout << m_in[i] << " ";
        }
        std::cout << "|...| ";
        for (int i = 5; i > 0; i--) {
            std::cout << m_in[m_in_size - i] << " ";
        }
        std::cout << std::endl;

        std::cout << "m_out:  ";
        for (int i = 0; i < 5; i++) {
            std::cout << m_out[i] << " ";
        }
        std::cout << "|...| ";
        for (int i = 5; i > 0; i--) {
            std::cout << m_out[m_out_size - i] << " ";
        }
        std::cout << std::endl;
        
        if (b){
            std::cout << "m_buff: ";
            for (int i = 0; i < m_size; i++) {
                std::cout << m_buffer[i];
                if (i == (m_out_size)-1){
                    std::cout << "|";
                } else {
                    std::cout << " ";
                }
            }
        }
        std::cout << std::endl << " -- " << std::endl;
    }
};


#define N 512
int main(void)
{
    pthread_t reader_thread, writer_thread;

    int from[N]{};
    int to[N]{};
    ringbuffer<int> rb(N*2,
        (int*)&from,sizeof(from)/sizeof(int),
        (int*)&to,sizeof(to)/sizeof(int));

    pthread_create(&reader_thread, NULL, ringbuffer<int>::read_wrapper, &rb);
    pthread_create(&writer_thread, NULL, ringbuffer<int>::write_wrapper, &rb);

    int n = 1;
    while (1){
        for (size_t i = 0; i < N; i++) {
            from[i] = i + n;
        }
        n++;
        sleep(1);
    }

    pthread_join(writer_thread, NULL);
    pthread_join(reader_thread, NULL);

    return 0;
}

// g++ ringbuffer.cpp -lpthread -std=c++17 -g

/*
 * GUI | AUDIO | EVENTHANDLER
 *   \       \_____/
 *    \___________/
 * 
 * the audio is 8k 512 sample size which means that its way slower than the eventhandler and the gui
 * is the ringbuffer my only chance? nonblocking storage
 * i could also check if the buffer is the same and not replace it all the time 
 * 
 * 
 * 
 */