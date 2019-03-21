#ifndef SRC_TURNSTILE_H_
#define SRC_TURNSTILE_H_

#include <type_traits>
#include <mutex>
#include <vector>
#include <deque>
#include <queue>
#include <condition_variable>

const size_t MAX_PROTECT = 251;
const size_t MIN_TURNSTILES = 16;

class Turnstile {
public:
    std::condition_variable cv;
    std::mutex mut;
    bool release;
    size_t waiting;

    Turnstile() : mut(){
        release = false;
        waiting = 0;
    }
};

class Mutex {
public:
    Mutex();
    Mutex(const Mutex&) = delete;

    void lock();    // NOLINT
    void unlock();  // NOLINT

private:
    Turnstile *m;
};


class Menager {
public:
    std::mutex protect_menager;
    std::mutex protect[MAX_PROTECT];
    std::queue<Turnstile*> turnstiles;
    size_t all;
    size_t free;
    Turnstile *one_in;

    Menager();
    ~Menager();

    Turnstile* get_turnstile();
    bool return_turnstile(Turnstile *m);
    bool add_turnstiles();
    bool del_turnstiles();
};

#endif  // SRC_TURNSTILE_H_
