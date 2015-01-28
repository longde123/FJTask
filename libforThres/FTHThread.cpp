/*
 * FTHThread.cpp
 *
 *  Created on: 28/ago/2014
 *      Author: bob
 */

#include "FTHThread.h"

namespace NSforThres {

FTHThread::FTHThread() {
	tid = 0;
	interrupted = false;
	daemon = true;
	priority = 2;
	running = false;
	detached = false;
}

FTHThread::~FTHThread() {
	if (running == true && detached == false) {
	        pthread_detach(tid);
	    }
	    if (running == true) {
	        pthread_cancel(tid);
	    }
}

static void * runThread(void *t){
	((FTHThread *)t)->run();
	return NULL;
}

int FTHThread::start(){
	/* Invoking a member function on a new thread */
	//th = new std::thread(&FTHThread::run,this);
	//tid = th->get_id();
	int result = pthread_create(&tid, NULL, runThread, this);
	    if (result == 0) {
	        running = true;
	}
	 return result;
}


int FTHThread::getPriority(){
	return priority;
}

pthread_t FTHThread::getTid(){
	return tid;
}

void FTHThread::setPriority(int newPriority){
	priority = newPriority;
}
void FTHThread::interrupt(){
	interrupted = true;
}
bool FTHThread::isInterrupted(){
	return interrupted;
}

int FTHThread::join(){
	int result = -1;
	    if (running == true) {
	        result = pthread_join(tid, NULL);
	        if (result == 0) {
	            detached = true;
	        }
	    }
	    return result;
}

void FTHThread::yield(){
	//pthread_yield();
	sched_yield();
}

void FTHThread::setDaemon(bool val){
	daemon = val;
}

int FTHThread::detach()
{
    int result = -1;
    if (running == true && detached == false) {
        result = pthread_detach(tid);
        if (result == 0) {
            detached = true;
        }
    }
    return result;
}

} /* namespace NSforThres */
