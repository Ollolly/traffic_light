#include <iostream>
#include <chrono>
#include <sys/file.h> //flock
#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/timerfd.h>
#include <unistd.h> //sleep
#include <queue>

#include "traffic_light.hpp"

using namespace traffic_light;

int test_traffic_light_create();
int test_traffic_light_be_green();
int test_set_timer();
int test_mng_create();
int test_tl_thread_func();
int test_queue_func();


int main()
{
    size_t err_count = 0;
    err_count += test_traffic_light_create();
    err_count += test_traffic_light_be_green();
    err_count += test_set_timer();
    err_count += test_mng_create();
    err_count += test_tl_thread_func();
    err_count += test_queue_func();


} //main

/**************************************************************************/
int test_traffic_light_create()
{
    size_t result = 0;

    size_t frequency = 2;
    size_t duration = 4;
    Traffic_Light tl(frequency, duration);

    if (tl.get_green_frequency() != frequency){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
        std::cout << tl.get_green_frequency() << std::endl;
    }

    if (tl.get_green_duration() != duration){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }

    if (tl.get_green_status() != false){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }

    tl.set_green_status(true);
    if (tl.get_green_status() != true){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }

    return result;
}
/**************************************************************************/
int test_traffic_light_be_green()
{
    size_t result = 0;
    size_t frequency = 2;
    size_t duration = 4;
    Traffic_Light tl1(frequency, duration);

    int fd = open("green.txt", O_WRONLY);
    if (fd == -1) {
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }

    frequency = 4;
    duration = 12;
    Traffic_Light tl2(frequency, duration);

    tl1.turn_on_green(fd);
    tl2.turn_on_green(fd);

    return result;
}

/**************************************************************************/
int set_timer(size_t tl_frequency, itimerspec* new_val)
{
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (fd == -1) {
        throw Failed_create_a_timer();
    }
    
    new_val->it_value.tv_sec = tl_frequency; 
    new_val->it_value.tv_nsec = 0;
    new_val->it_interval.tv_sec = tl_frequency; 
    new_val->it_interval.tv_nsec = 0;

    if (timerfd_settime(fd, 0, new_val, nullptr) == -1) {
        throw Failed_set_a_timer();
    }
    
    return fd;
}

int test_set_timer()
{
    size_t result = 0;
    size_t frequency = 1;
    size_t duration = 2;
    Traffic_Light tl(frequency, duration);

    int fd = open("green.txt", O_WRONLY);
    if (fd == -1) {
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }


    itimerspec timer = {};
    set_timer(frequency, &timer);
    int count = 0;
    while (count < 50) {
       //tl.turn_on_green(fd);
        ++count;
        std::cout << count << " ";
    }

    std::cout << std::endl;

    return result;
}
/**************************************************************************/
int test_mng_create()
{
    size_t result = 0;

    size_t data[10] = {2,4,9,5,1,1,8,2};

    
    Mng_Traffic_Lights mng("green.txt", 4, data);

 /*   if ( mng.m_is_mng_run != true){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                << __LINE__ << std::endl;
    }

    if ( mng.m_tl_threads.size() != 1){
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                << __LINE__ << std::endl;
    }
   */ 

    sleep(80);

    return result;
}
/**************************************************************************/
void tl_thread_func(size_t tl_frequency, size_t tl_duration, bool* m_is_mng_run, Traffic_Light *tl, int fd)
{
    Traffic_Light t(tl_frequency, tl_duration);
    std::cout << t.get_green_duration() << std::endl;

    itimerspec timer = {};
    set_timer(tl_frequency, &timer);

int loops = 10;
    while (loops > 0) {
        //this->add_to_queue(&tl);
        //wait untit can turn on green
        while(not tl->get_green_status() && *m_is_mng_run) {
        }

        if (not m_is_mng_run) {
            break;
        }

        std::cout << " status green ::";
        tl->turn_on_green(fd);
        tl->set_green_status(false);
        std::cout << "new status  " << tl->get_green_status() << std::endl;

    } //m_is_mng_run

    std::cout << " stop " << std::endl;
}

int test_tl_thread_func()
{
    size_t result = 0;
    size_t frequency = 3;
    size_t duration = 5;
    Traffic_Light tl(frequency, duration);

    int fd = open("green.txt", O_WRONLY);
    if (fd == -1) {
        ++result;
        std::cout << "error in function: " << __FUNCTION__ << " line "
                  << __LINE__ << std::endl;
    }

    bool run = true;
    std::thread thread = std::thread(&tl_thread_func, frequency, duration, &run, &tl, fd);

    sleep(2);
    std::cout << "111111" << std::endl;
    tl.set_green_status(true);
    sleep(5);
    std::cout << "222222" << std::endl;
    tl.set_green_status(true);

    sleep(2);
    std::cout << "33333" << std::endl;

    run = false;
    thread.join();

    return result;
}
/**************************************************************************/
void queue_thread_func(std::queue<int>* m_queue, bool* run)
{
    std::mutex m_mutex;

    while (1) {
        while(m_queue->empty() && *run) {
             std::cout << " ******** size: " << m_queue->size() << " run: " << *run << std::endl;
        }

        if (not *run) {
            std::cout << " break";
            break;
        }

        std::unique_lock<std::mutex> mlock(m_mutex);
        int tl = m_queue->front();
        m_queue->pop();
        mlock.unlock();
        //tl->set_green_status(true);
        std::cout << " " << tl;
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    } //m_is_mng_run
}

int test_queue_func()
{
    std::queue<int> m_queue;

    bool run = true;
    std::thread thread = std::thread(&queue_thread_func, &m_queue, &run);
/*
    std::cout << "111111" << std::endl;
    sleep(5);
    for (int i = 0; i < 3; ++i) {
        m_queue.push(i);
    }

    sleep(5);
    std::cout << "222222" << std::endl;
    for (int i = 0; i < 5; ++i) {
        m_queue.push(i);
    }
*/
    sleep(3);
    std::cout << "33333" << std::endl;
    run = false;
    thread.join();

    std::cout << "4444" << std::endl;

    return 0;
}
