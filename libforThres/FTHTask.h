/*
 * FTHTask.h
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#ifndef FTHTASK_H_
#define FTHTASK_H_

#include "ForThres.h"
namespace NSforThres {

class FTHTask {
	friend class FTHTaskRunner;
	friend class FTHTaskRunnerGroup;
	friend class InvokableFTHTask;
private:
	volatile bool done;
	FTHTaskRunner *taskRunner;
public:
	FTHTask();
	virtual ~FTHTask();
	FTHTaskRunner* getFTHTaskRunner();

	FTHTaskRunnerGroup* getFTHTaskRunnerGroup();
	bool isDone();
	void cancel();
	void reset();
	void start();
	void fork();
	void join();
	virtual void run() = 0;
	void yield();
	static void invoke(FTHTask *t);
	void coInvoke(FTHTask *task1, FTHTask *task2);
	void coInvoke(FTHTask **tasks, int count);
protected:
	void setDone();
	void setFTHTaskRunner(FTHTaskRunner* t);
};

} /* namespace NSforThres */

#endif /* FTHTASK_H_ */
