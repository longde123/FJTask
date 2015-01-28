/*
 * FTHTaskRunner.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#include "FTHTaskRunner.h"

namespace NSforThres {

/**
 *  Constructor called only during FTHTaskRunnerGroup initialization
 **/
FTHTaskRunner::FTHTaskRunner(FTHTaskRunnerGroup *g) {
	// TODO Auto-generated constructor stub
	deq = new FTHDeque();
	group = g;
	active = false;
	scanPriority = 2;
	runs = 0;
	scans = 0;
	steals = 0;
	rand_seed = time(NULL);
	scanPriority = FTHTaskRunnerGroup::DEFAULT_SCAN_PRIORITY;
	runPriority = getPriority();
	setDaemon(true);
}

FTHTaskRunner::~FTHTaskRunner() {
	delete deq;
}

/**
 * Return the FTHTaskRunnerGroup of which this thread is a member
 **/
FTHTaskRunnerGroup* FTHTaskRunner::getGroup() {
	return group;
}

/* ------------ Other BookKeeping ------------------- */

/**
 * Set the priority to use while scanning.
 * We do not bother synchronizing access, since
 * by the time the value is needed, both this FTHTaskRunner
 * and its FTHTaskRunnerGroup will
 * necessarily have performed enough synchronization
 * to avoid staleness problems of any consequence.
 **/
void FTHTaskRunner::setScanPriority(int pri) {
	scanPriority = pri;
}

/**
 * Set the priority to use while running tasks.
 * Same usage and rationale as setScanPriority.
 **/
void FTHTaskRunner::setRunPriority(int pri) {
	runPriority = pri;
}

/* ------------ Scheduling  ------------------- */

/**
 * Do all but the pop() part of yield or join, by
 * traversing all DEQs in our group looking for a task to
 * steal. If none, it checks the entry queue.
 * <p>
 * Since there are no good, portable alternatives,
 * we rely here on a mixture of Thread.yield and priorities
 * to reduce wasted spinning, even though these are
 * not well defined. We are hoping here that the JVM
 * does something sensible.
 * @param waitingFor if non-null, the current task being joined
 **/

//TODO: this part has to be executed by another thread
void FTHTaskRunner::scan(FTHTask *waitingFor) {

	FTHTask *task = NULL;

	// to delay lowering priority until first failure to steal
	bool lowered = false;

	/*
	 Circularly traverse from a random start index.

	 This differs slightly from cilk version that uses a random index
	 for each attempted steal.
	 Exhaustive scanning might impede analytic tractablity of
	 the scheduling policy, but makes it much easier to deal with
	 startup and shutdown.
	 */

	FTHTaskRunner **ts = group->getArray();
	int size = group->getSize();
	int idx = rand_r(&rand_seed) % size;

	for (int i = 0; i < size; ++i) {

		FTHTaskRunner *t = ts[idx];
		if (++idx >= size)
			idx = 0; // circularly traverse

		if (t != NULL && t != this) {

			if (waitingFor != NULL && waitingFor->isDone()) {
				break;
			} else {

				task = t->deq->steal();
				if (task != NULL) {
					/*steal done*/
					break;
				} else if (isInterrupted()) {
					break;
				} else if (!lowered) { // if this is first fail, lower priority
					lowered = true;
					setPriority(scanPriority);
				} else {           // otherwise we are at low priority; just yield
					yield();
				}
			}
		}

	}

	if (task == NULL) {
		/* entry queue */
		//TODO: insert here external steal
		task = group->pollEntryQueue();
	}

	if (lowered)
		setPriority(runPriority);

	if (task != NULL && !task->isDone()) {
		task->setFTHTaskRunner(this);
		task->run();
		task->setDone();
	}

}

/**
 * Same as scan, but called when current thread is idling.
 * It repeatedly scans other threads for tasks,
 * sleeping while none are available.
 * <p>
 * This differs from scan mainly in that
 * since there is no reason to return to recheck any
 * condition, we iterate until a task is found, backing
 * off via sleeps if necessary.
 **/

void FTHTaskRunner::scanWhileIdling() {
	FTHTask *task = NULL;

	bool lowered = false;
	long iters = 0;

	FTHTaskRunner **ts = group->getArray();
	int size = group->getSize();
	int idx = rand_r(&rand_seed) % size;

	do {
		for (int i = 0; i < size; ++i) {

			FTHTaskRunner *t = ts[idx];
			if (++idx >= size)
				idx = 0; // circularly traverse

			if (t != NULL && t != this) {
				if (COLLECT_STATS)
					++scans;

				task = t->deq->steal();
				if (task != NULL) {
					if (COLLECT_STATS)
						++steals;
					if (lowered)
						setPriority(runPriority);
					group->setActive(this);
					break;
				}
			}
		}

		if (task == NULL) {
			if (isInterrupted())
				return;

			if (COLLECT_STATS)
				++scans;
			task = group->pollEntryQueue();

			if (task != NULL) {
				if (COLLECT_STATS)
					++steals;
				if (lowered)
					setPriority(runPriority);
				group->setActive(this);
			} else {
				++iters;
				//  Check here for yield vs sleep to avoid entering group synch lock
				if (iters >= group->SCANS_PER_SLEEP) {
					group->checkActive(this, iters);
					if (isInterrupted())
						return;
				} else if (!lowered) {
					lowered = true;
					setPriority(scanPriority);
				} else {
					yield();
				}
			}
		}
	} while (task == NULL);

	if (!task->isDone()) {
		if (COLLECT_STATS)
			++runs;
		task->setFTHTaskRunner(this);
		task->run();
		task->setDone();
	}

}

/* ------------  composite operations ------------------- */

/**
 * Main runloop
 **/

void FTHTaskRunner::run() {
	/* try { */
	while (!isInterrupted()) {

		FTHTask *task = deq->pop();
		if (task != NULL) {
			if (!task->isDone()) {
				// inline FTHTask.invoke
				if (COLLECT_STATS)
					++runs;
				task->run();
				task->setDone();
			}
		} else
			scanWhileIdling();
	}
	/*}
	 finally {*/
	group->setInactive(this);

	/* } */
}

/**
 * Execute a task in this thread. Generally called when current task
 * cannot otherwise continue.
 **/

void FTHTaskRunner::taskYield() {
	FTHTask *task = deq->pop();
	if (task != NULL) {
		if (!task->isDone()) {
			if (COLLECT_STATS)
				++runs;
			task->run();
			task->setDone();
		}
	} else
		scan(NULL);
}

/**
 * Process tasks until w is done.
 * Equivalent to <code>while(!w.isDone()) taskYield(); </code>
 **/

void FTHTaskRunner::taskJoin(FTHTask *w) {

	while (!w->isDone()) {

		FTHTask *task = deq->pop();
		if (task != NULL) {
			if (!task->isDone()) {
				task->run();
				task->setDone();
				if (task == w)
					return; // fast exit if we just ran w
			}
		} else {
			scan(w);
		}
	}
}

/**
 * A specialized expansion of
 * <code> w.fork(); invoke(v); w.join(); </code>
 **/
//protected final
void FTHTaskRunner::coInvoke(FTHTask *w, FTHTask *v) {
	w->setFTHTaskRunner(this);
	v->setFTHTaskRunner(this);
	int t = deq->top;
	if (t < (deq->base & deq->deqLength - 1) + deq->deqLength) {

		//deq->push(w);
		/*if the deque is not overflowing , fast insert*/
		deq->deq_array[(t & deq->deqLength - 1)] = w;
		deq->top = (t + 1);

		if (!v->isDone()) {
			this->runs += 1;
			v->run();
			v->setDone();
		}

		while (!w->isDone()) {
			FTHTask *task = deq->pop();
			if (task != NULL) {
				if (!task->isDone()) {
					this->runs += 1;
					task->run();
					task->setDone();
					if (task == w)
						return;
				}
			} else
				scan(w);
		}
	} else {
		slowCoInvoke(w, v);
	}
}

/*if the deque is close to overflow is used the slow coInvoke*/
void FTHTaskRunner::slowCoInvoke(FTHTask *w, FTHTask *v) {
	w->setFTHTaskRunner(this);
	v->setFTHTaskRunner(this);
	deq->push(w);
	this->runs += 1;
	FTHTask::invoke(v);
	taskJoin(w);
}

/**
 * Array-based version of coInvoke
 **/
void FTHTaskRunner::coInvoke(FTHTask** tasks, int count) {
	int nforks = count - 1;

	int t = deq->top;

	if ((nforks >= 0) && (t + nforks < (deq->base & deq->deqLength - 1) + deq->deqLength)) {

		//TODO: deq->multiplePush(tasks, nforks);
		for (int i = 0; i < nforks; i++) {
			deq->deq_array[(t++ & deq->deqLength - 1)] = tasks[i];
			deq->top = t;
		}

		FTHTask *v = tasks[nforks];
		//invoke v
		if (!v->isDone()) {
			this->runs += 1;
			v->run();
			v->setDone();
		}

		for (int i = 0; i < nforks; i++) {
			FTHTask *w = tasks[i];
			//taskjoin(w)
			while (!w->isDone()) {
				FTHTask *task = deq->pop();
				if (task != NULL) {
					if (!task->isDone()) {
						this->runs += 1;
						task->run();
						task->setDone();
					}
				} else
					scan(w);
			}
		}
	} else {
		slowCoInvoke(tasks, count);
	}
}

/*slow version of invoke are enough, measure performances of both.*/
void FTHTaskRunner::slowCoInvoke(FTHTask** tasks, int count) {
	for (int i = 0; i < count; ++i) {
		tasks[i]->setFTHTaskRunner(this);
		deq->push(tasks[i]);
	}
	for (int i = 0; i < count; ++i)
		taskJoin(tasks[i]);
}

}
/* namespace NSforThres */

/**
 * Specialized Thread subclass for running FTHTasks.
 * <p>
 * Each FJTaskRunner keeps FJTasks in a double-ended queue (DEQ).
 * Double-ended queues support stack-based operations
 * push and pop, as well as queue-based operations put and take.
 * Normally, threads run their own tasks. But they
 * may also steal tasks from each others DEQs.
 * <p>
 * The algorithms are minor variants of those used
 * in <A href="http://supertech.lcs.mit.edu/cilk/"> Cilk</A> and
 * <A href="http://www.cs.utexas.edu/users/hood/"> Hood</A>, and
 * to a lesser extent
 * <A href="http://www.cs.uga.edu/~dkl/filaments/dist.html"> Filaments</A>,
 * but are adapted to work in Java.
 * <p>
 * The two most important capabilities are:
 * <ul>
 *  <li> Fork a FJTask:
 *  <pre>
 *  Push task onto DEQ
 *  </pre>
 *  <li> Get a task to run (for example within taskYield)
 *  <pre>
 *  If DEQ is not empty,
 *     Pop a task and run it.
 *  Else if any other DEQ is not empty,
 *     Take ("steal") a task from it and run it.
 *  Else if the entry queue for our group is not empty,
 *     Take a task from it and run it.
 *  Else if current thread is otherwise idling
 *     If all threads are idling
 *        Wait for a task to be put on group entry queue
 *  Else
 *      Yield or Sleep for a while, and then retry
 *  </pre>
 * </ul>
 * The push, pop, and put are designed to only ever called by the
 * current thread, and take (steal) is only ever called by
 * other threads.
 * All other operations are composites and variants of these,
 * plus a few miscellaneous bookkeeping methods.
 * <p>
 * Implementations of the underlying representations and operations
 * are geared for use on JVMs operating on multiple CPUs (although
 * they should of course work fine on single CPUs as well).
 * <p>
 * A possible snapshot of a FJTaskRunner's DEQ is:
 * <pre>
 *     0     1     2     3     4     5     6    ...
 *  +-----+-----+-----+-----+-----+-----+-----+--
 *  |     |  t  |  t  |  t  |  t  |     |     | ...  deq array
 *  +-----+-----+-----+-----+-----+-----+-----+--
 *           ^                       ^
 *          base                    top
 *   (incremented                     (incremented
 *       on take,                         on push
 *    decremented                     decremented
 *       on put)                          on pop)
 * </pre>
 * <p>
 * FJTasks are held in elements of the DEQ.
 * They are maintained in a bounded array that
 * works similarly to a circular bounded buffer. To ensure
 * visibility of stolen FJTasks across threads, the array elements
 * must be <code>volatile</code>.
 * Using volatile rather than synchronizing suffices here since
 * each task accessed by a thread is either one that it
 * created or one that has never seen before. Thus we cannot
 * encounter any staleness problems executing run methods,
 * although FJTask programmers must be still sure to either synch or use
 * volatile for shared data within their run methods.
 * <p>
 * However, since there is no way
 * to declare an array of volatiles in Java, the DEQ elements actually
 * hold VolatileTaskRef objects, each of which in turn holds a
 * volatile reference to a FJTask.
 * Even with the double-indirection overhead of
 * volatile refs, using an array for the DEQ works out
 * better than linking them since fewer shared
 * memory locations need to be
 * touched or modified by the threads while using the DEQ.
 * Further, the double indirection may alleviate cache-line
 * sharing effects (which cannot otherwise be directly dealt with in Java).
 * <p>
 * The indices for the <code>base</code> and <code>top</code> of the DEQ
 * are declared as volatile. The main contention point with
 * multiple FJTaskRunner threads occurs when one thread is trying
 * to pop its own stack while another is trying to steal from it.
 * This is handled via a specialization of Dekker's algorithm,
 * in which the popping thread pre-decrements <code>top</code>,
 * and then checks it against <code>base</code>.
 * To be conservative in the face of JVMs that only partially
 * honor the specification for volatile, the pop proceeds
 * without synchronization only if there are apparently enough
 * items for both a simultaneous pop and take to succeed.
 * It otherwise enters a
 * synchronized lock to check if the DEQ is actually empty,
 * if so failing. The stealing thread
 * does almost the opposite, but is set up to be less likely
 * to win in cases of contention: Steals always run under synchronized
 * locks in order to avoid conflicts with other ongoing steals.
 * They pre-increment <code>base</code>, and then check against
 * <code>top</code>. They back out (resetting the base index
 * and failing to steal) if the
 * DEQ is empty or is about to become empty by an ongoing pop.
 * <p>
 * A push operation can normally run concurrently with a steal.
 * A push enters a synch lock only if the DEQ appears full so must
 * either be resized or have indices adjusted due to wrap-around
 * of the bounded DEQ. The put operation always requires synchronization.
 * <p>
 * When a FJTaskRunner thread has no tasks of its own to run,
 * it tries to be a good citizen.
 * Threads run at lower priority while scanning for work.
 * <p>
 * If the task is currently waiting
 * via yield, the thread alternates scans (starting at a randomly
 * chosen victim) with Thread.yields. This is
 * well-behaved so long as the JVM handles Thread.yield in a
 * sensible fashion. (It need not. Thread.yield is so underspecified
 * that it is legal for a JVM to treat it as a no-op.) This also
 * keeps things well-behaved even if we are running on a uniprocessor
 * JVM using a simple cooperative threading model.
 * <p>
 * If a thread needing work is
 * is otherwise idle (which occurs only in the main runloop), and
 * there are no available tasks to steal or poll, it
 * instead enters into a sleep-based (actually timed wait(msec))
 * phase in which it progressively sleeps for longer durations
 * (up to a maximum of FJTaskRunnerGroup.MAX_SLEEP_TIME,
 * currently 100ms) between scans.
 * If all threads in the group
 * are idling, they further progress to a hard wait phase, suspending
 * until a new task is entered into the FJTaskRunnerGroup entry queue.
 * A sleeping FJTaskRunner thread may be awakened by a new
 * task being put into the group entry queue or by another FJTaskRunner
 * becoming active, but not merely by some DEQ becoming non-empty.
 * Thus the MAX_SLEEP_TIME provides a bound for sleep durations
 * in cases where all but one worker thread start sleeping
 * even though there will eventually be work produced
 * by a thread that is taking a long time to place tasks in DEQ.
 * These sleep mechanics are handled in the FJTaskRunnerGroup class.
 * <p>
 * Composite operations such as taskJoin include heavy
 * manual inlining of the most time-critical operations
 * (mainly FJTask.invoke).
 * This opens up a few opportunities for further hand-optimizations.
 * Until Java compilers get a lot smarter, these tweaks
 * improve performance significantly enough for task-intensive
 * programs to be worth the poorer maintainability and code duplication.
 * <p>
 * Because they are so fragile and performance-sensitive, nearly
 * all methods are declared as final. However, nearly all fields
 * and methods are also declared as protected, so it is possible,
 * with much care, to extend functionality in subclasses. (Normally
 * you would also need to subclass FJTaskRunnerGroup.)
 * <p>
 * None of the normal java.lang.Thread class methods should ever be called
 * on FJTaskRunners. For this reason, it might have been nicer to
 * declare FJTaskRunner as a Runnable to run within a Thread. However,
 * this would have complicated many minor logistics. And since
 * no FJTaskRunner methods should normally be called from outside the
 * FJTask and FJTaskRunnerGroup classes either, this decision doesn't impact
 * usage.
 * <p>
 * You might think that layering this kind of framework on top of
 * Java threads, which are already several levels removed from raw CPU
 * scheduling on most systems, would lead to very poor performance.
 * But on the platforms
 * tested, the performance is quite good.
 * <p>[<a href="http://gee.cs.oswego.edu/dl/classes/EDU/oswego/cs/dl/util/concurrent/intro.html"> Introduction to this package. </a>]
 * @see FJTask
 * @see FJTaskRunnerGroup
 **/

