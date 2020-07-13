#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <mutex>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>
#include <functional>

#include <pthread.h>
#include <sys/syscall.h>

std::atomic<bool> g_stop_producers { false };
std::atomic<bool> g_stop_consumers { false };

void SetThreadNameNative(const char* name)
{
    pthread_setname_np(pthread_self(), name);
}

void log(const char* msg)
{
    static std::mutex logMutex;
    auto tid = syscall(__NR_gettid);
    std::lock_guard<std::mutex> lock(logMutex);
    auto ts = boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time());
    std::clog << "[" << tid << "] " << ts << ": " << msg << std::endl;
}

using events_per_second_t = unsigned;

pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER; 
int g_state = 0;

void producer(events_per_second_t rate)
{
    SetThreadNameNative("producer");
    log("producer started");
    std::chrono::milliseconds period(unsigned(1000.0 / rate));
    auto last = std::chrono::high_resolution_clock::now();
    size_t events = 0u;
    while (!g_stop_producers.load(std::memory_order_relaxed))
    {
        {
            pthread_mutex_lock(&g_m);
            g_state |= 1;
            pthread_mutex_unlock(&g_m);
            pthread_cond_signal(&g_cv);
        }
        ++events;
        auto now = std::chrono::high_resolution_clock::now();
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        if (diff_ms > 5000)
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "producer rate = %.3f", events * 1000.0f / diff_ms);
            log(buf);
            events = 0u;
            last = now;
        }
        std::this_thread::sleep_for(period);
    }
    log("producer stopped");
}

void consumer(events_per_second_t max_rate)
{
    SetThreadNameNative("cosumer");
    log("consumer started");
    std::chrono::milliseconds period(unsigned(1000.0 / max_rate));
    auto last = std::chrono::high_resolution_clock::now();
    size_t wakeups = 0u;
    while (!g_stop_consumers.load(std::memory_order_relaxed))
    {
        {
            pthread_mutex_lock(&g_m);
            g_state &= ~1;
            while (0 == (g_state & 1))
            {
                g_state += 2;
                pthread_cond_wait(&g_cv, &g_m);
                g_state -= 2;
            }
            pthread_mutex_unlock(&g_m);
        }
        ++wakeups;
        auto now = std::chrono::high_resolution_clock::now();
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        if (diff_ms > 5000)
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "consumer rate = %.3f", wakeups * 1000.0f / diff_ms);
            log(buf);
            wakeups = 0u;
            last = now;
        }
        std::this_thread::sleep_for(period);
    }
    log("consumer stopped");
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "USAGE: " << argv[0] << " <nr-producers/nr-threads> <producers-rate> <consumer-max-rate>" << std::endl;
        return 1;
    }
    auto producer_factor = boost::lexical_cast<float>(argv[1]);
    auto producer_rate = boost::lexical_cast<events_per_second_t>(argv[2]);
    auto consumer_max_rate = boost::lexical_cast<events_per_second_t>(argv[2]);

    unsigned poolSize = std::thread::hardware_concurrency();
    unsigned nr_producers = poolSize * producer_factor;

    std::vector<std::thread> producers, consumers;
    for (unsigned i = 0; i < poolSize; ++i)
    {
        consumers.emplace_back(std::bind(&consumer, consumer_max_rate));
    }

    for (unsigned i = 0; i < nr_producers; ++i)
    {
        producers.emplace_back(std::bind(&producer, producer_rate));
    }

    std::string line;
    while (std::getline(std::cin, line))
    {
        if ("quit" == line)
            break;
    }
    g_stop_consumers = true;
    for (auto& consumer: consumers)
        consumer.join();
    g_stop_producers = true;
    for (auto& producer: producers)
        producer.join();
    return 0;
}
