/*
 * FTHTaskRunner.h
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#ifndef FTHTASKRUNNER_H_
#define FTHTASKRUNNER_H_

#include "ForThres.h"
#include "FTHThread.h"

namespace NSforThres {

class FTHTaskRunner : public FTHThread {
	friend class FTHTask;
	friend class FTHTaskRunnerGroup;
public:
	FTHTaskRunner(FTHTaskRunnerGroup *g);
	virtual ~FTHTaskRunner();
	/* ------------ DEQ Representation ------------ */
	FTHDeque *deq;

	/* ------------  composite operations --------- */
	void run();
protected:
	void setScanPriority(int pri);
	void setRunPriority(int pri);

	bool active;
	unsigned rand_seed;
	/** Random starting point generator for scan() **/

	/** Priority to use while scanning for work **/
	int scanPriority;
	/** Priority to use while running tasks **/
	int runPriority;
	FTHTaskRunnerGroup *group;

	void taskYield();
	void taskJoin(FTHTask *w);
	void coInvoke(FTHTask *tasks);
	void coInvoke(FTHTask *w, FTHTask *v);
	void slowCoInvoke(FTHTask *w, FTHTask *v);
	void coInvoke(FTHTask **tasks, int count);
	void slowCoInvoke(FTHTask **tasks, int count);

	FTHTaskRunnerGroup* getGroup();
	/* ------------ Scheduling  ------------------- */
	void scan(FTHTask *waitingFor);
	void scanWhileIdling();
	/*stats variables*/
	static const bool COLLECT_STATS = true;
	int runs;
	int scans;
	int steals;
};

} /* namespace NSforThres */

#endif /* FTHTASKRUNNER_H_ */
