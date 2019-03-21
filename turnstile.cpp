#include <cassert>
#include "turnstile.h"

Menager::Menager() :protect_menager(), turnstiles() {
    for (size_t i = 0; i < MIN_TURNSTILES; i++) {
        turnstiles.push(new Turnstile);
    }
    all = MIN_TURNSTILES;
    free = MIN_TURNSTILES;
    one_in = new Turnstile;
}

Menager &get_menager() {
    static Menager m;
    return m;
}

Menager::~Menager() {
    Turnstile *t;
    while (!get_menager().turnstiles.empty()) {
        t = get_menager().turnstiles.front();
        get_menager().turnstiles.pop();
        delete t;
    }
    delete one_in;
}

bool Menager::add_turnstiles() {
    for (size_t i = 0; i < all; i++) {
        turnstiles.push(new Turnstile);
    }
    free += all;
    all *= 2;
    return true;
}

bool Menager::del_turnstiles() {
    for (size_t i = 0; i < all / 2; i++) {
        auto t = turnstiles.front();
        turnstiles.pop();
        delete t;
    }
    free -= (all / 2);
    all /= 2;
    return true;
}

Turnstile *Menager::get_turnstile() {
    std::lock_guard<std::mutex> lock(protect_menager);
    if (turnstiles.empty()) {
        add_turnstiles();
    }
    assert(!get_menager().turnstiles.empty());

    Turnstile *t = get_menager().turnstiles.front();
    assert(t != nullptr);

    get_menager().turnstiles.pop();
    get_menager().free--;

    return t;
}

bool Menager::return_turnstile(Turnstile* m) {
    std::lock_guard<std::mutex> lock(protect_menager);
    get_menager().free++;
    get_menager().turnstiles.push(m);
    if (get_menager().all > MIN_TURNSTILES && get_menager().free >= get_menager().all * 3 / 4) {
        get_menager().del_turnstiles();
    }
    return true;
}

Mutex::Mutex() {
    m = nullptr;
}

void Mutex::lock() {
    get_menager().protect[(size_t) this % MAX_PROTECT].lock();
    if (m == nullptr) {
        m = get_menager().one_in;
        get_menager().protect[(size_t) this % MAX_PROTECT].unlock();
    } else {
        if (m == get_menager().one_in) {
            m = get_menager().get_turnstile();
        }
        m->waiting++;

        get_menager().protect[(size_t) this % MAX_PROTECT].unlock();

        std::unique_lock<std::mutex> lk(m->mut);
        m->cv.wait(lk, [&] { return m->release; });

        get_menager().protect[(size_t) this % MAX_PROTECT].lock();
        m->release = false;
        m->waiting--;
        if (m->waiting == 0){
            get_menager().return_turnstile(m);
            m = get_menager().one_in;
        }
        get_menager().protect[(size_t) this % MAX_PROTECT].unlock();

    }
}

void Mutex::unlock() {
    std::lock_guard<std::mutex> lock(get_menager().protect[(size_t) this % MAX_PROTECT]);
    assert(m != nullptr);
    if (m == get_menager().one_in) {
        m = nullptr;
    } else {
        std::unique_lock<std::mutex> lk(m->mut);
        m->release = true;
        m->cv.notify_one();
    }
}