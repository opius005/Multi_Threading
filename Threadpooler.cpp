

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include <queue>
#include <functional>
#include <utility>
#include <sstream>
#include <future>

class ThreadPool
{
private:
    size_t n_threads;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;

public:
    ThreadPool(size_t n_t) : n_threads{n_t}, stop{false}
    {
        threads.reserve(n_threads);
        for (int i = 0; i < n_threads; i++)
        {
            threads.emplace_back(
                [this]()->void
                {
                    while(1)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> ul(queue_mutex);
                            cv.wait(ul,[this]{return stop||!tasks.empty();});
                            if(stop&&tasks.empty()) return;
                            task=std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    } 
                }
            );
        }
    }

    template <class F, class... Args>
    auto enque_task(F &&task, Args &&...args) -> std::future<decltype(task(args...))>
    {
        using ret_type = decltype(task(args...));
        auto prom = std::make_shared<std::promise<ret_type>>();
        std::future<ret_type> result = prom->get_future();
        auto fun{
            [prom, task = std::forward<F>(task), ... args = std::forward<Args>(args)]() -> void
            {
                prom->set_value(task(args...));
            }};
        {
            std::unique_lock<std::mutex> ul(queue_mutex);
            tasks.emplace(std::move(fun));
        }
        cv.notify_one();
        std::cout << "Enq done \n";
        return result;
    }

    ~ThreadPool()
    {
        std::unique_lock<std::mutex> ul(queue_mutex);
        stop = true;
        ul.unlock();
        cv.notify_all();
        for (int i = 0; i < n_threads; i++)
        {
            if (threads[i].joinable())
            {
                threads[i].join();
            }
            else
            {
                std::cout << "Problem while joining for " << i << std::endl;
            }
        }
    }
};

int main()
{
    int n = 4, k = 15;
    ThreadPool tp(n);
    std::vector<std::future<int>> res;
    std::cout << "Thread pool created with " << n << " Threads\n";
    for (int i = 0; i < k; i++)
    {
        res.emplace_back(tp.enque_task([i](int a)
                                       {
            printf("Task %d executed by thread %s \n",i,(std::stringstream ()<<std::this_thread::get_id()).str().c_str());
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return a+1; }, i));
            // std::cout<<res.back().get()<<std::endl;
    }
     for (int i = 0; i < k; i++)
     {
         printf("%d \n", res[i].get());
     }
}
