#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <mutex>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <cassert>
#include <atomic>

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

using mutex_t = boost::asio::detail::conditionally_enabled_mutex;
using event_t = boost::asio::detail::conditionally_enabled_event;
using events_per_second_t = unsigned;

mutex_t g_m(true);
event_t g_e;

void producer(events_per_second_t rate)
{
    SetThreadNameNative("producer");
    log("producer started");
    std::chrono::milliseconds period(unsigned(1000.0 / rate));
    auto last = boost::posix_time::microsec_clock::universal_time();
    size_t events = 0u;
    while (!g_stop_producers.load(std::memory_order_relaxed))
    {
        {
            mutex_t::scoped_lock l(g_m);
            g_e.maybe_unlock_and_signal_one(l);
        }
        ++events;
        auto now = boost::posix_time::microsec_clock::universal_time(); 
        auto diff_ms = (now - last).total_milliseconds();
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
    auto last = boost::posix_time::microsec_clock::universal_time();
    size_t wakeups = 0u;
    while (!g_stop_consumers.load(std::memory_order_relaxed))
    {
        {
            mutex_t::scoped_lock l(g_m);
            g_e.clear(l);
            g_e.wait(l);
        }
        ++wakeups;
        auto now = boost::posix_time::microsec_clock::universal_time(); 
        auto diff_ms = (now - last).total_milliseconds();
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

    boost::thread_group producers, consumers;
    for (unsigned i = 0; i < poolSize; ++i)
    {
        consumers.create_thread(boost::bind(&consumer, consumer_max_rate));
    }

    for (unsigned i = 0; i < nr_producers; ++i)
    {
        producers.create_thread(boost::bind(&producer, producer_rate));
    }

    std::string line;
    while (std::getline(std::cin, line))
    {
        if ("quit" == line)
            break;
    }
    g_stop_consumers = true;
    consumers.join_all();
    g_stop_producers = true;
    producers.join_all();
    return 0;
}
