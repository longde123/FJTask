/*
 * InvokableFTHTask.h
 *
 *  Created on: 29/ago/2014
 *      Author: bob
 */

#ifndef INVOKABLEFTHTASK_H_
#define INVOKABLEFTHTASK_H_

#include "FTHTask.h"

namespace NSforThres {

class InvokableFTHTask: public FTHTask {
private:
	std::mutex sync_mutex;
	std::condition_variable cv;
public:
	FTHTask *wrapped;
	bool terminated;
	InvokableFTHTask(FTHTask *task);
	virtual ~InvokableFTHTask();
	void run();
	void setTerminated();
	void awaitTermination();
};

} /* namespace NSforThres */

#endif /* INVOKABLEFTHTASK_H_ */
