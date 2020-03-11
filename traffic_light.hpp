#ifndef TRAFFIC_LIGHT_H_ 
#define TRAFFIC_LIGHT_H_ 

#include <cstring>
#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <semaphore.h>
#include <stdexcept>
#include <sys/timerfd.h>

namespace traffic_light {

class Failed_to_lock_file : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_to_unlock_file : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_to_open_file : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_to_write_to_file : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_create_a_timer : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_set_a_timer : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_to_init_semaphore : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_on_sem_post : public std::exception {
    virtual const char* what() const noexcept override;
};

class Failed_on_sem_wait : public std::exception {
    virtual const char* what() const noexcept override;
};

class Traffic_Light {
public:
    // frequency and duration are measured in seconds
    Traffic_Light(size_t green_frequency, size_t green_duration);
    ~Traffic_Light() = default;
    Traffic_Light(const Traffic_Light& other) = default;
    Traffic_Light& operator=(const Traffic_Light& other) = default; 

    size_t get_green_frequency() const;
    size_t get_green_duration() const;
    void set_green_status(bool status);
    bool get_green_status() const;
    void turn_on_green(int fd);

private:
    size_t m_green_frequency;
    size_t m_green_duration;
    bool m_can_be_green;
};

/* Traffic Lights init values are passed to Mng_Traffic_Lights into
* array : tl_data value. 
* For each traffic light, the data stored in two consecutive indexes, 
* when the first is frequency and the second is duration. 
*/
class Mng_Traffic_Lights {
public:
    Mng_Traffic_Lights(const char* file_path, size_t traffic_ligths_amount, size_t tl_data[]);
    ~Mng_Traffic_Lights();
    Mng_Traffic_Lights(const Mng_Traffic_Lights& other) = delete;
    Mng_Traffic_Lights& operator=(const Mng_Traffic_Lights& other) = delete;

private:
    int m_fd;
    std::queue<Traffic_Light*> m_queue;
    std::thread m_queue_thread;
    std::vector<std::thread> m_tl_threads;
    std::mutex m_mutex;
    sem_t m_semaphore;
    bool m_is_mng_run;

    void traffic_light_run(size_t tl_frequency, size_t tl_duration); 
    void operate_traffic_light_queue(); 
    void add_traffic_light_to_queue(Traffic_Light* tl);
    int set_timer(size_t tl_frequency, itimerspec* new_val);
};

} //traffic_light 

#endif //TRAFFIC_LIGHT_H_

