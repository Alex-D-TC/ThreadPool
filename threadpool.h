#include <queue>
#include <pthread.h>
#include <stdexcept>

#define array_size unsigned long long

class ThreadPool {

private:
    
    typedef void* (*tFunc)(void *);

    struct ThreadData {
        void* args;
        tFunc threadFunc;   
    
        void operator()() {
            threadFunc(args);
        }
    };

    array_size count;
    bool isOver;

    pthread_t *threads;
    std::queue<ThreadData> functions;
    
    pthread_mutex_t taskFetchMutex = PTHREAD_MUTEX_INITIALIZER;

    static void* threadLoop(void* p) {

        static_cast<ThreadPool*>(p)->fetchTasks();

        return NULL;
    }


    void fetchTasks() {

        while(!isOver || functions.size() != 0) {

            pthread_mutex_lock(&taskFetchMutex);

            if(functions.size() == 0) {
                pthread_mutex_unlock(&taskFetchMutex);
                continue;
            }
            
            ThreadData data = functions.front();
 
            functions.pop();           
            
            pthread_mutex_unlock(&taskFetchMutex); 

            data();
        }
     
    }

    void startThreads() {
  
        for(array_size i = 0; i < count; ++i) {
            pthread_create(&(threads[i]), NULL, threadLoop, this);
        }
    }
 
public:

    ThreadPool()
    : count(10), isOver(false), threads(new pthread_t[count]) {
        startThreads();
    }

    ThreadPool(array_size size)
    : count(size), isOver(false), threads(new pthread_t[count]) {
        startThreads();
    }

    ~ThreadPool() {

        if(!isOver) {
            throw std::runtime_error("awaitTermination not called");
        }

        delete[] threads;
        
        pthread_mutex_destroy(&taskFetchMutex);
    }
    
    void awaitTermination() {

        if(isOver)
            return; //  If a user calls awaitTermination twice, the pthread_join call on a joined
                    // thread will crash everything :(

        isOver = true; 
    
        for(array_size i = 0; i < count; ++i) {
            pthread_join(threads[i], NULL);
        }
    }

    void add(void* (*function) (void*), void* arg) {
  
        if(isOver)
            return;

        ThreadData data;
        data.threadFunc = function;
        data.args = arg;

        functions.push(data);               
    }
};

