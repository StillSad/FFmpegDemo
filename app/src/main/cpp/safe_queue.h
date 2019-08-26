//
// Created by ICE on 2019-08-15.
//

#ifndef FFMPEGDEMO_SAFE_QUEUE_H
#define FFMPEGDEMO_SAFE_QUEUE_H

#include <queue>
#include <pthread.h>

using namespace std;

template <typename T>
class SafeQueue{
    typedef void (*ReleaseCallback)(T *);
    typedef void (*SyncHandle)(queue<T> &);

public:
    SafeQueue(){
        pthread_mutex_init(&mutex,0);
        pthread_cond_init(&cond,0);
    }
    ~SafeQueue(){
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    void push(T value){
        pthread_mutex_lock(&mutex);

        if(work) {
            q.push(value);
            pthread_cond_signal(&cond);
        } else {
            if(releaseCallback) {
                releaseCallback(&value);
            }
        }

        pthread_mutex_unlock(&mutex);
    }

    int pop(T &value){
        int ret = 0;

        pthread_mutex_lock(&mutex);

        while (work && q.empty()) {
            pthread_cond_wait(&cond,&mutex);
        }

        if(!q.empty()){
            value = q.front();
            q.pop();
            ret = 1;
        }

        pthread_mutex_unlock(&mutex);
        return ret;

    }

    void setWork(int work){
        pthread_mutex_lock(&mutex);

        this->work =work;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);

    }

    int empty(){
        return q.empty();
    }

    int size(){
        return q.size();
    }

    void clear(){
        pthread_mutex_lock(&mutex);

        unsigned int size = q.size();
        for (int i = 0; i < size; ++i) {
            //取出队首元素
            T value = q.front();
            if(releaseCallback) {
                releaseCallback(&value);
            }
            q.pop();
        }
        pthread_mutex_unlock(&mutex);
    }

    void setReleaseCallback(ReleaseCallback releaseCallback){
        this->releaseCallback = releaseCallback;
    }

    void setSyncHandle(SyncHandle syncHandle) {
        this->syncHandle = syncHandle;
    }

    /**
     * 同步操作
     */
     void sync() {
         pthread_mutex_lock(&mutex);
         syncHandle(q);
         pthread_mutex_unlock(&mutex);
     }

private:
    queue<T> q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work;
    ReleaseCallback releaseCallback;
    SyncHandle  syncHandle;
};


#endif //FFMPEGDEMO_SAFE_QUEUE_H
