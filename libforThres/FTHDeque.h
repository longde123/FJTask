/*
 * FTHDeque.h
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#ifndef FTHDEQUE_H_
#define FTHDEQUE_H_

#include "ForThres.h"

namespace NSforThres {

class FTHDeque {
	friend class FTHTaskRunner;
private:
	std::mutex sync_mutex;
	std::mutex barrier_mutex;
	std::condition_variable cv;
	void slowPush(FTHTask *r);
	void checkOverflow();
	/*methods*/
	FTHTask* confirmTake(int oldBase);
	FTHTask* take();
	FTHTask* confirmPop(int provisionalTop);
	void put(FTHTask *r);
protected:
	static const int INITIAL_CAPACITY = 4096;
	static const int MAX_CAPACITY = 1073741824;
	volatile FTHTask** deq_array;
	volatile int deqLength;
	volatile int top;
	volatile int base;
public:
	FTHDeque();
	virtual ~FTHDeque();
	/*steals a task pointer bottom*/
	FTHTask* steal();
	/*push a task pointer in the front*/
	void push(FTHTask *t);
	/*push a task pointer in the front*/
	void multiplePush(FTHTask **t, int count);
	/* pops a task pointer from the front */
	FTHTask* pop();
	int getLength();
};

class MaxCapacityExceededException: public std::exception {
	virtual const char* what() const throw () {
		return "FTHDeque: maximum capacity exceeded";
	}
};

} /* namespace NSforThres */

/**
 * Double-ended queues support stack-based operations
 * push and pop, as well as queue-based operations put and take.
 * Normally, threads run their own tasks. But they
 * may also steal tasks from each others DEQs.
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
 *
 * Even with the double-indirection overhead of
 * volatile refs, using an array for the DEQ works out
 * better than linking them since fewer shared
 * memory locations need to be
 * touched or modified by the threads while using the DEQ.
 * Further, the double indirection may alleviate cache-line
 * sharing effects (which cannot otherwise be directly dealt with in Java).
 * <p>
 * The indices for the <code>base</code> and <code>top</code> of the DEQ
 * are declared as volatile.
 *
 * The main contention point with
 * multiple FJTaskRunner threads occurs when one thread is trying
 * to pop its own stack while another is trying to steal from it.
 * This is handled via a specialization of Dekker's algorithm,
 * in which the popping thread pre-decrements <code>top</code>,
 * and then checks it against <code>base</code>.
 *
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

#endif /* FTHDEQUE_H_ */
