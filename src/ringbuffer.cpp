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
            rb->readToOut();
        } while (1);
    }

    static void* write_wrapper(void* context) {
        auto rb = static_cast<ringbuffer*>(context);
        do {
            rb->writeFromIn();
            sleep(1); // one thread has to be slower than the other?
        } while (1);
    }

    // write from m_in to the internal ring buffer
    void writeFromIn() {
        pthread_mutex_lock(&m_mutex);

        std::cout << "m_in:  ";
        for (int i = 0; i < m_in_size; i++) {
            std::cout << m_in[i] << " ";
        }
        std::cout << std::endl;

        std::cout << "m_buffer:";
        for (int i = 0; i < m_size; i++) {
            std::cout << m_buffer[i] << " ";
        }
        std::cout << std::endl;

        for(size_t i = 0; i < m_in_size; ++i) {
            m_buffer[m_writer_next] = m_in[i];
            m_writer_next = (m_writer_next + 1) % m_size;
            if (m_reader_unread < m_size) {
                ++m_reader_unread;
            } else {
                // todo: handle overflow or advance head (if implementing a circular overwrite mechanism)
            }
        }
        pthread_cond_signal(&m_written_cond); // notify the waiting reader
        pthread_mutex_unlock(&m_mutex);
    }

    // read into m_out from the internal ring buffer
    void readToOut() {
        pthread_mutex_lock(&m_mutex);

        std::cout << "m_out: ";
        for (int i = 0; i < m_in_size; i++) {
            std::cout << m_out[i] << " ";
        }
        std::cout << std::endl;

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
};


int main(void)
{
    pthread_t reader_thread, writer_thread;

    int from[5] = {1,11,3,4};
    int to[5]{};
    ringbuffer<int> rb(10,(int*)&from,5,(int*)&to,5);

    puts("Starting threads. Type text and press enter, or type ctrl-d at empty line to quit.");
    pthread_create(&reader_thread, NULL, ringbuffer<int>::read_wrapper, &rb);
    pthread_create(&writer_thread, NULL, ringbuffer<int>::write_wrapper, &rb);

    int n = 1;
    while (1){
        for (size_t i = 0; i < 5; i++) {
            from[i] = i + n;
        }
        n++;
        sleep(0.5);
    }

    pthread_join(writer_thread, NULL);
    pthread_join(reader_thread, NULL);

    return 0;
}

// g++ ringbuffer.cpp -lpthread -std=c++17 -g