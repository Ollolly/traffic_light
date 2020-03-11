#include <chrono>
#include <sstream> 
#include <stdexcept>

#include <sys/file.h> //flock
#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <fcntl.h> //open
#include <unistd.h> //close

#include "traffic_light.hpp"

namespace traffic_light {

/************* Eexceptions *************/
const char* Failed_to_lock_file::what() const noexcept
{
    return "Failed to lock the file with flock_func.";
}

const char* Failed_to_unlock_file::what() const noexcept
{
    return "Failed to unlock the file.";
}

const char* Failed_to_open_file::what() const noexcept
{
    return "Failed to open the file.";
}

const char* Failed_to_write_to_file::what() const noexcept
{
    return "Failed to write to file.";
}

const char* Failed_create_a_timer::what() const noexcept
{
    return "Failed to create a timer.";
}

const char* Failed_set_a_timer::what() const noexcept
{
    return "Failed to set a timer.";
}

const char* Failed_to_init_semaphore::what() const noexcept
{
    return "Failed to init semaphore.";
}

const char* Failed_on_sem_post::what() const noexcept
{
    return "Failed to perform semaphore post.";
}

const char* Failed_on_sem_wait::what() const noexcept
{
    return "Failed to perform semaphore wait.";
}

/************* Traffic_Light *************/
Traffic_Light::Traffic_Light(size_t green_frequency,
                             size_t green_duration) 
                             : 
                             m_green_frequency(green_frequency),
                             m_green_duration(green_duration),
                             m_can_be_green(false)
{}

size_t Traffic_Light::get_green_frequency() const
{
    return m_green_frequency;
}

size_t Traffic_Light::get_green_duration() const
{
    return m_green_duration;
}

void Traffic_Light::set_green_status(bool status) 
{
    m_can_be_green = status;
}

bool Traffic_Light::get_green_status() const
{
    return m_can_be_green;
}

void Traffic_Light::turn_on_green(int fd)
{
    std::stringstream tmp;
    tmp << "i'm green now. my green frequency is: " << m_green_frequency << 
            " and duration is " << m_green_duration << "    ";
    std::string msg = tmp.str();

    if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
        throw Failed_to_lock_file();
    }

    if (write(fd, msg.c_str(), msg.size()) < 0) { 
        throw Failed_to_write_to_file();
    }

    //hold the file for traffic light green duration
    std::this_thread::sleep_for(std::chrono::seconds(this->get_green_duration()));

    if (flock(fd, LOCK_UN) == -1){
         throw Failed_to_unlock_file();
    }
}

/************* Mng_Traffic_Lights *************/
Mng_Traffic_Lights::Mng_Traffic_Lights(const char* file_path, 
									   size_t traffic_ligths_amount, 
                                       size_t tl_data[])
                                       :
                                       m_fd(0),
                                       m_queue(), 
                                       m_queue_thread(),
                                       m_tl_threads(traffic_ligths_amount),
                                       m_mutex(),
                                       m_semaphore(),
                                       m_is_mng_run(true)
{
    m_fd = open(file_path, O_WRONLY);
    if (m_fd == -1) {
        throw Failed_to_open_file();
    }

    if (sem_init(&m_semaphore, 0, 1) != 0) {
        throw Failed_to_init_semaphore();
    }

    m_queue_thread = std::thread(&Mng_Traffic_Lights::operate_traffic_light_queue, this);

    for (std::thread& th : m_tl_threads) {
        th = std::thread(&Mng_Traffic_Lights::traffic_light_run, this, *tl_data, *(tl_data + 1));
        tl_data += 2;
    }
}

Mng_Traffic_Lights::~Mng_Traffic_Lights() 
{
    m_is_mng_run = false;
    m_queue_thread.join(); 
    for (std::thread& th : m_tl_threads) 
    {
        th.join();
    }

    close(m_fd);
}

void Mng_Traffic_Lights::traffic_light_run(size_t tl_frequency, size_t tl_duration)
{
    Traffic_Light tl(tl_frequency, tl_duration);

    itimerspec timer = {};

    while (1) {
        this->set_timer(tl_frequency, &timer);
        this->add_traffic_light_to_queue(&tl);
        //wait untit can turn on green or mng stopped
        while(not tl.get_green_status() && m_is_mng_run) {
        }

        if (not m_is_mng_run) {
            break;
        }

        tl.turn_on_green(m_fd);
        if (sem_post(&m_semaphore) < 0) {
            throw Failed_on_sem_post();
        }

        tl.set_green_status(false);
    }
}

void Mng_Traffic_Lights::operate_traffic_light_queue()
{
    while (1) {
        //wait untit queue is not empty or mng stopped
        while(m_queue.empty() && m_is_mng_run) {
        }

        if (not m_is_mng_run) {
            break;
        }

        std::unique_lock<std::mutex> mlock(m_mutex);
        Traffic_Light* tl = m_queue.front();
        m_queue.pop();
        mlock.unlock();
        tl->set_green_status(true);
        if (sem_wait(&m_semaphore) < 0) {
            throw Failed_on_sem_wait();
        }
    }
}

void Mng_Traffic_Lights::add_traffic_light_to_queue(Traffic_Light* tl) 
{
    std::unique_lock<std::mutex> mlock(m_mutex);
    m_queue.push(tl);
}

int Mng_Traffic_Lights::set_timer(size_t tl_frequency, itimerspec* new_val)
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

} //traffic_light


