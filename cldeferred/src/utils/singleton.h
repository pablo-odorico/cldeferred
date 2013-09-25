#ifndef SINGLETON_H
#define SINGLETON_H

template <class T>
class Singleton
{
public:
    static T& instance() {
        static T instance;
        return instance;
    }

private:
    /*
    // Hide ctor, dtor, copy ctor and assign operator
    Singleton() {}
    ~Singleton() {}
    Singleton(Singleton const&) {}
    Singleton& operator=(Singleton const&) {}
    */
};

#endif // SINGLETON_H
