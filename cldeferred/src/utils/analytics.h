#ifndef ANALYTICS_H
#define ANALYTICS_H

class Analytics
{
public:
    static Analytics& instance() {
        static Analytics instance;
        return instance;
    }

private:
    Analytics();
    Analytics(const Analytics& other);
};

#endif // ANALYTICS_H
