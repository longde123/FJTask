/*
 * InvokableFTHTask.cpp
 *
 *  Created on: 29/ago/2014
 *      Author: bob
 */

#include "InvokableFTHTask.h"

namespace NSforThres {

InvokableFTHTask::InvokableFTHTask(FTHTask *task) {
	this->wrapped = task;
	this->terminated = false;
}

InvokableFTHTask::~InvokableFTHTask() {
	// TODO Auto-generated destructor stub
}

void InvokableFTHTask::run() {
	/*try {*/
		//if ((this.wrapped instanceof FJTask))
		wrapped->setFTHTaskRunner(this->getFTHTaskRunner());
		FTHTask::invoke(wrapped);
	/*
		else
		this.wrapped.run();
	 * } finally {*/
		setTerminated();
	//}
}

//protected synchronized
void InvokableFTHTask::setTerminated() {
	std::unique_lock<std::mutex> lck(sync_mutex);
	terminated = true;
	cv.notify_all();
}

//protected synchronized
void InvokableFTHTask::awaitTermination() /*throws InterruptedException*/ {
	std::unique_lock<std::mutex> lck(sync_mutex);
	while (!terminated) cv.wait(lck);
}

}
/* namespace NSforThres */
