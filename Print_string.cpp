#include <iostream>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>


class thread_handler
{
private:
    std::string main_str;
    size_t ouput_len;
    size_t n_threads;
    std::vector<std::thread> threads;
    int executable_thread_number;
    int pos;
    bool begin;
    std::condition_variable cv;
    std::mutex run_mutex;
    std::atomic<int> n_times;

    void print(int i)
    {
        size_t c = ouput_len;
        std::cout << "Thread " << i << " is printing ";
        while (c > 0)
        {
            int printing_len = std::min(pos + c, main_str.size()) - pos;
            std::cout << main_str.substr(pos, printing_len);
            pos = (pos + printing_len) % main_str.size();
            c -= printing_len;
        }
        std::cout << std::endl;
    }
    void run(int i)
    {
        while (1)
        {
            std::unique_lock<std::mutex> ul(run_mutex);
            cv.wait(ul, [this, i]()
                    { 
                        return n_times == 0 || i == executable_thread_number; 
                    });
            if (n_times == 0)
            {
                return;
            }
            print(i);
            executable_thread_number = (executable_thread_number + 1);
            if (executable_thread_number == n_threads)
            {
                executable_thread_number = 0;
                std::cout << std::endl;
                n_times--;
                if (n_times == 0)
                {
                    cv.notify_all();
                    return;
                }
            }
            ul.unlock();
            cv.notify_all();
        }
    }

public:
    thread_handler(std::string s, size_t n_trds, size_t oup_len, int n_t) : main_str{s},
                                                                            n_threads{n_trds},
                                                                            ouput_len{oup_len},
                                                                            executable_thread_number{-1},
                                                                            pos{0},
                                                                            n_times{n_t}
    {
        threads.reserve(n_threads);
        for (int i = 0; i < n_threads; i++)
        {
            threads.emplace_back(&thread_handler::run, this, i);
        }
    }

    void start()
    {
        executable_thread_number = 0;
        cv.notify_all();
    }

    ~thread_handler()
    {
        for (int i = 0; i < n_threads; i++)
        {
            threads[i].join();
        }
    }
};

void solve(std::string s, int n_threads, int output_len, int n_times)
{
    thread_handler t(s, n_threads, output_len, n_times);
    t.start();
}

int main()
{
    std::string s = "abcde";
    int n_threads = 4;
    int len = 16;
    int n_times = 5;
    solve(s, n_threads, len, n_times);
}