/*
 * FTHTaskRunnerGroup.h
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#ifndef FTHTASKRUNNERGROUP_H_
#define FTHTASKRUNNERGROUP_H_

#include "ForThres.h"

namespace NSforThres {

class FTHTaskRunnerGroup {
	friend class FTHTaskRunner;
	friend class FTHTask;
private:
	static const bool COLLECT_STATS = true;
	/**
	 * The number of times to scan other threads for tasks
	 * before transitioning to a mode where scans are
	 * interleaved with sleeps (actually timed waits).
	 * Upon transition, sleeps are for duration of
	 * scans / SCANS_PER_SLEEP milliseconds.
	 * <p>
	 * This is not treated as a user-tunable parameter because
	 * good values do not appear to vary much across JVMs or
	 * applications. Its main role is to help avoid some
	 * useless spinning and contention during task startup.
	 **/
	static const long SCANS_PER_SLEEP = 15L;
	/**
	 * The maximum time (in msecs) to sleep when a thread is idle,
	 * yet others are not, so may eventually generate work that
	 * the current thread can steal. This value reflects the maximum time
	 * that a thread may sleep when it possibly should not, because there
	 * are other active threads that might generate work. In practice,
	 * designs in which some threads become stalled because others
	 * are running yet not generating tasks are not likely to work
	 * well in this framework anyway, so the exact value does not matter
	 * too much. However, keeping it in the sub-second range does
	 * help smooth out startup and shutdown effects.
	 **/
	static const long MAX_SLEEP_TIME = 100L;
	std::chrono::time_point<std::chrono::system_clock> initTime;
	int entries;
	std::mutex sync_mutex;
	std::condition_variable cv;
	int groupSize;
public:
	FTHTaskRunnerGroup(int groupSize);
	virtual ~FTHTaskRunnerGroup();
	/*void execute(FTHTask *t) made for runnable, useless?*/
	void executeTask(FTHTask *t);
	void invoke(FTHTask *t);
	void interruptAll();
	//synchronized
	void setScanPriorities(int pri);
	//synchronized
	void setRunPriorities(int pri);
	int getSize();
	//synchronized
	int getActiveCount();
	void stats();

protected:
	static const int DEFAULT_SCAN_PRIORITY = 2;
	FTHTaskRunner **threads;
	FTHDeque *entryQueue;
	int activeCount;
	int nstarted;

	/* ------------ Methods called only by FJTaskRunners ------------- */
	FTHTaskRunner** getArray();
	FTHTask* pollEntryQueue();
	//synchronized
	bool getActive(FTHTaskRunner *t);
	//synchronized
	void setActive(FTHTaskRunner *t);
	//synchronized
	void setInactive(FTHTaskRunner *t);
	//synchronized
	void checkActive(FTHTaskRunner *t, long scans);

	/* ------------ Utility methods  ------------- */

	/**
	 * Start or wake up any threads waiting for work
	 **/
	void signalNewTask();

	/**
	 * Create all FJTaskRunner threads in this group.
	 **/
	void initializeThreads();
};

} /* namespace NSforThres */

#endif /* FTHTASKRUNNERGROUP_H_ */

