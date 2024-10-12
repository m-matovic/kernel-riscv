#include "../h/syscall_cpp.hpp"

void *operator new(size_t size)
{
    return mem_alloc(size);
}

void *operator new[](size_t size)
{
    return mem_alloc(size);
}

void operator delete(void *ptr) noexcept
{
    mem_free(ptr);
}

void operator delete[](void *ptr) noexcept
{
    mem_free(ptr);
}

Thread::Thread(void(* body)(void *), void *arg)
{
    this->body = body;
    this->arg = arg;
}

Thread::Thread()
{
    struct __hack
    {
        static void __func(Thread *thread)
        {
            thread->run();
        }
    };

    this->body = (void (*)(void *))__hack::__func;
    this->arg = this;
}

int Thread::start()
{
    return thread_create(&this->myHandle, this->body, this->arg);
}

void Thread::dispatch()
{
    thread_dispatch();
}

int Thread::sleep(time_t time)
{
    return time_sleep(time);
}

Thread::~Thread()
{
    delete this->myHandle;
}

Semaphore::Semaphore(unsigned init)
{
    sem_open(&this->myHandle, init);
}

int Semaphore::wait()
{
    return sem_wait(this->myHandle);
}

int Semaphore::signal()
{
    return sem_signal(this->myHandle);
}

int Semaphore::timedWait(time_t t)
{
    return sem_timed_wait(this->myHandle, t);
}

int Semaphore::tryWait()
{
    return sem_trywait(this->myHandle);
}

Semaphore::~Semaphore()
{
    sem_close(this->myHandle);
}

char Console::getc()
{
    return ::getc();
}

void Console::putc(char c)
{
    ::putc(c);
}
