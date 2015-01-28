/*
 * FTHThread.h
 *
 *  Created on: 28/ago/2014
 *      Author: bob
 */

#ifndef FTHTHREAD_H_
#define FTHTHREAD_H_

//#include <thread>
#include <pthread.h>
#include <sched.h>

namespace NSforThres {

class FTHThread {
private:
//	std::thread *th;
//	std::thread::id tid;
	int priority;
	bool interrupted;
	bool daemon;
	bool running;
	bool detached;
	pthread_t  tid;

	//static void exec(FTHThread *t);
public:
	FTHThread();
	virtual ~FTHThread();
	int start();
	int getPriority();
	pthread_t getTid(); /*std::thread::id */
	void setPriority(int newPriority);
	void interrupt();
	bool isInterrupted();
	int join();
	void yield();
	void setDaemon(bool val);
	int detach();
	virtual void run() = 0;
};

} /* namespace NSforThres */

#endif /* FTHTHREAD_H_ */
