/*
 * FTHTaskRunnerGroup.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#include "FTHTaskRunnerGroup.h"
#include "InvokableFTHTask.h"

namespace NSforThres {

FTHTaskRunnerGroup::FTHTaskRunnerGroup(int groupSize) :
		entryQueue() {
	entryQueue = new FTHDeque();
	entries = 0;
	activeCount = 0;
	nstarted = 0;
	threads = new FTHTaskRunner*[groupSize];
	this->groupSize = groupSize;
	initializeThreads();
	initTime = std::chrono::system_clock::now();
}

/*call the constructor of every TaskRunner*/
void FTHTaskRunnerGroup::initializeThreads() {
	for (int i = 0; i < this->groupSize; ++i)
		threads[i] = new FTHTaskRunner(this);
}

FTHTaskRunnerGroup::~FTHTaskRunnerGroup() {
	// TODO Auto-generated destructor stub
	for (int i = 0; i < this->groupSize; ++i)
			delete threads[i];
}

/**
 * Arrange for execution of the given task
 * by placing it in a work queue. If the argument
 * is not of type FJTask, it is embedded in a FJTask via
 * <code>FJTask.Wrap</code>.
 * @exception InterruptedException if current Thread is
 * currently interrupted
 **/

/*void FTHTaskRunnerGroup::execute(FTHTask *t) //throw interrupted exception
 * {
 entryQueue->push(t);
 signalNewTask();
 }*/

/**
 * Specialized form of execute called only from within FJTasks
 **/
void FTHTaskRunnerGroup::executeTask(FTHTask *t) {
	/*try { */
	entryQueue->push(t);
	signalNewTask();
	/* } catch (InterruptedException ex) {
	 Thread.currentThread().interrupt();
	 } */
}

/**
 * Start a task and wait it out. Returns when the task completes.
 * @exception InterruptedException if current Thread is
 * interrupted before completion of the task.
 **/

void FTHTaskRunnerGroup::invoke(FTHTask *r) /*throws InterruptedException*/{
	InvokableFTHTask *w = new InvokableFTHTask(r);
	entryQueue->push(w);
	signalNewTask();
	/*awaitfortermination*/
	w->awaitTermination();
}

/**
 * Try to shut down all FJTaskRunner threads in this group
 * by interrupting them all. This method is designed
 * to be used during cleanup when it is somehow known
 * that all threads are idle.
 * FJTaskRunners only
 * check for interruption when they are not otherwise
 * processing a task (and its generated subtasks,
 * if any), so if any threads are active, shutdown may
 * take a while, and may lead to unpredictable
 * task processing.
 **/

void FTHTaskRunnerGroup::interruptAll() {
	// paranoically interrupt current thread last if in group.
	//std::thread::id current = std::this_thread::get_id();
	pthread_t current = pthread_self();
	FTHTaskRunner *curr_t;
	bool stopCurrent = false;
	for (int i = 0; i < this->groupSize ; i++) {
		FTHTaskRunner *t = threads[i];
		if (t->getTid() == current) {
			stopCurrent = true;
			curr_t = t;
		} else {
			t->interrupt();
		}
	}
	if (stopCurrent)
		curr_t->interrupt();
}

/**
 * Set the priority to use while a FJTaskRunner is
 * polling for new tasks to perform. Default
 * is currently Thread.MIN_PRIORITY+1. The value
 * set may not go into effect immediately, but
 * will be used at least the next time a thread scans for work.
 **/
// synchronized
void FTHTaskRunnerGroup::setScanPriorities(int pri) {
	std::unique_lock<std::mutex> lck(sync_mutex);
	for (int i = 0; i < this->groupSize ; i++) {
		FTHTaskRunner *t = threads[i];
		t->setScanPriority(pri);
		if (!t->active)
			t->setPriority(pri);
	}
}

/**
 * Set the priority to use while a FJTaskRunner is
 * actively running tasks. Default
 * is the priority that was in effect by the thread that
 * constructed this FJTaskRunnerGroup. Setting this value
 * while threads are running may momentarily result in
 * them running at this priority even when idly waiting for work.
 **/
//public synchronized void
void FTHTaskRunnerGroup::setRunPriorities(int pri) {
	std::unique_lock<std::mutex> lck(sync_mutex);
	for (int i = 0; i < this->groupSize ; i++) {
		FTHTaskRunner *t = threads[i];
		t->setRunPriority(pri);
		if (t->active)
			t->setPriority(pri);
	}
}

/** Return the number of FJTaskRunner threads in this group **/

int FTHTaskRunnerGroup::getSize() {
	return groupSize;
}

/**
 * Return the number of threads that are not idly waiting for work.
 * Beware that even active threads might not be doing any useful
 * work, but just spinning waiting for other dependent tasks.
 * Also, since this is just a snapshot value, some tasks
 * may be in the process of becoming idle.
 **/
//public synchronized int
int FTHTaskRunnerGroup::getActiveCount() {
	std::unique_lock<std::mutex> lck(sync_mutex);
	return activeCount;
}

void FTHTaskRunnerGroup::stats() {
	std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now()
			- this->initTime;
//long time = System.currentTimeMillis() - this.initTime;
	double secs = elapsed_seconds.count();
	long totalRuns = 0L;
	long totalScans = 0L;
	long totalSteals = 0L;


	std::cout << "Thread\tQ Cap\tScans\tNew\tRuns\n";
	for (int i = 0; i < this->groupSize ; i++) {
		FTHTaskRunner *t = threads[i];
		int truns = t->runs;
		totalRuns += truns;

		int tscans = t->scans;
		totalScans += tscans;

		int tsteals = t->steals;
		totalSteals += tsteals;

		//std::string star(getActive(t) ? "*" : " ");
		if(getActive(t)){
		std::cout << "T" << i <<  "*" << "\t" << t->deq->getLength() << "\t" << tscans << "\t"
				<< tsteals << "\t" << truns << "\n";
		} else {
			std::cout << "T" << i <<  " " << "\t" << t->deq->getLength() << "\t" << tscans << "\t"
							<< tsteals << "\t" << truns << "\n";
		}
	}

	std::cout << "Total\t    \t" << totalScans << "\t" << totalSteals << "\t" << totalRuns << "\n";

	std::cout << "Execute: " << this->entries;
	std::cout << "\tTime: " << secs;

	long rps = 0L;
	if (secs != 0.0)
		rps = std::lround(totalRuns / secs);

	std::cout << "\tRate: " << rps << "\n";
}


/* ------------ Methods called only by FJTaskRunners ------------- */

/**
 * Return the array of threads in this group.
 * Called only by FJTaskRunner.scan().
 **/

FTHTaskRunner** FTHTaskRunnerGroup::getArray() {
	return threads;
}

/**
 * Return a task from entry queue, or null if empty.
 * Called only by FJTaskRunner.scan().
 **/

FTHTask* FTHTaskRunnerGroup::pollEntryQueue() {
	/*try {*/
	FTHTask *t = entryQueue->steal();
	return t;
	/*} catch (InterruptedException ex) { // ignore interrupts
	 Thread.currentThread().interrupt();
	 return null;
	 }*/
}

/**
 * Return active status of t.
 * Per-thread active status can only be accessed and
 * modified via synchronized method here in the group class.
 **/

//protected synchronized boolean
bool FTHTaskRunnerGroup::getActive(FTHTaskRunner *t) {
	std::unique_lock<std::mutex> lck(sync_mutex);
	return t->active;
}

/**
 * Set active status of thread t to true, and notify others
 * that might be waiting for work.
 **/

//protected synchronized void
void FTHTaskRunnerGroup::setActive(FTHTaskRunner *t) {
	std::unique_lock<std::mutex> lck(sync_mutex);
	if (!t->active) {
		t->active = true;
		++activeCount;
		if (nstarted < this->groupSize) {
			threads[nstarted++]->start();
		} else {
			cv.notify_all();
		}
	}
}

/**
 * Set active status of thread t to false.
 **/

//protected synchronized void
void FTHTaskRunnerGroup::setInactive(FTHTaskRunner *t) {
	std::unique_lock<std::mutex> lck(sync_mutex);
	if (t->active) {
		t->active = false;
		--activeCount;
	}
}

/**
 * Set active status of thread t to false, and
 * then wait until: (a) there is a task in the entry
 * queue, or (b) other threads are active, or (c) the current
 * thread is interrupted. Upon return, it
 * is not certain that there will be work available.
 * The thread must itself check.
 * <p>
 * The main underlying reason
 * for these mechanics is that threads do not
 * signal each other when they add elements to their queues.
 * (This would add to task overhead, reduce locality.
 * and increase contention.)
 * So we must rely on a tamed form of polling. However, tasks
 * inserted into the entry queue do result in signals, so
 * tasks can wait on these if all of them are otherwise idle.
 **/

//protected synchronized void
void FTHTaskRunnerGroup::checkActive(FTHTaskRunner *t, long scans) {
	/*
	 * locked on construction and unlocked on deconstruction
	 * is ok for synchronizing access to this method but maybe is slow
	 * */
	std::unique_lock<std::mutex> lck(sync_mutex);
	setInactive(t);

	/*try { */
// if nothing available, do a hard wait
	if (activeCount == 0 && entryQueue->steal() == NULL) {
		cv.wait(lck);
	} else {
		// If there is possibly some work,
		// sleep for a while before rechecking

		long msecs = scans / SCANS_PER_SLEEP;
		if (msecs > MAX_SLEEP_TIME)
			msecs = MAX_SLEEP_TIME;
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		if (msecs == 0) {
			cv.wait_until(lck, now + std::chrono::milliseconds(msecs));
		} else {
			// forces shortest possible sleep
			cv.wait_until(lck, now + std::chrono::nanoseconds(1));
		}
	}

	/*} catch (InterruptedException ex) {
	 notify(); // avoid lost notifies on interrupts
	 Thread.currentThread().interrupt();
	 }*/
}

/* ------------ Utility methods  ------------- */

/**
 * Start or wake up any threads waiting for work
 **/

/*synchronized*/
void FTHTaskRunnerGroup::signalNewTask() {
	std::unique_lock<std::mutex> lck(sync_mutex);
	if (COLLECT_STATS) {
		++entries;
	}
	if (nstarted < getSize()) {
		threads[nstarted++]->start();
	} else {
		cv.notify_one();
	}
}

/**
 * Create all FJTaskRunner threads in this group.
 **/

} /* namespace NSforThres */
