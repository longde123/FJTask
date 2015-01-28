/*
 * FTHDeque.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: Roberto Belli
 */

#include "FTHDeque.h"

namespace NSforThres {

FTHDeque::FTHDeque() {
	// TODO: check this new with volatile
	deq_array = new volatile FTHTask*[FTHDeque::INITIAL_CAPACITY];
	deqLength = FTHDeque::INITIAL_CAPACITY;
	top = 0;
	base = 0;
}

FTHDeque::~FTHDeque() {
	// TODO Auto-generated destructor stub
	delete[] deq_array;
}

int FTHDeque::getLength() {
	return deqLength;
}

FTHTask* FTHDeque::steal() {
	return take();
}

void FTHDeque::multiplePush(FTHTask **tasks, int count) {
	int t = this->top;
	if ((count >= 0) && (t + count < (this->base & this->deqLength - 1) + this->deqLength)) {
		for (int i = 0; i < count; i++) {
			this->deq_array[(t++ & this->deqLength - 1)] = tasks[i];
			this->top = t;
		}
	} else {
		for (int i = 0; i < count; i++) {
			push(tasks[i]);
		}
	}
}

/*final*/
void FTHDeque::push(FTHTask *r) {
	int t = this->top;

	if (t < (this->base & deqLength - 1) + deqLength) {
		this->deq_array[(t & deqLength - 1)] = r;
		this->top = (t + 1);
	} else {
		slowPush(r);
	}
}

/*synchronized*/
void FTHDeque::slowPush(FTHTask *r) {
	std::unique_lock < std::mutex > lck(sync_mutex);
	checkOverflow();
	push(r);
}

/*final synchronized*/
void FTHDeque::put(FTHTask *r) {
	std::unique_lock < std::mutex > lck(sync_mutex);
	while (true) {
		int b = this->base - 1;
		if (this->top < b + deqLength) {
			int newBase = b & deqLength - 1;
			/*insert task in queue */
			this->deq_array[newBase] = r;
			this->base = newBase;

			if (b != newBase) {
				int newTop = this->top & deqLength - 1;
				if (newTop < newBase)
					newTop += deqLength;
				this->top = newTop;
			}
			return;
		}
		checkOverflow();
	}
}

/*final*/
FTHTask* FTHDeque::pop() {
	int t = --top;

	if (this->base + 1 < t) {
		volatile FTHTask *res = deq_array[(t & deqLength - 1)];
		deq_array[(t & deqLength - 1)] = NULL;
		return (FTHTask *) res;
	}
	return confirmPop(t);
}

/*final synchronized*/
FTHTask* FTHDeque::confirmPop(int provisionalTop) {
	std::unique_lock < std::mutex > lck(sync_mutex);
	if (this->base <= provisionalTop) {
		volatile FTHTask *res = deq_array[(provisionalTop & deqLength - 1)];
		deq_array[(provisionalTop & deqLength - 1)] = NULL;
		return (FTHTask *) res;
	}

	this->top = (this->base = 0);
	return NULL;
}

/*final synchronized*/
FTHTask* FTHDeque::take() {
	std::unique_lock < std::mutex > lck(sync_mutex);
	int b = this->base++;

	if (b < this->top) {
		return confirmTake(b);
	}

	this->base = b;
	return NULL;
}

FTHTask* FTHDeque::confirmTake(int oldBase) {
	std::unique_lock < std::mutex > lck(barrier_mutex);
	//synchronized (this.barrier) {
	if (oldBase < this->top) {
		return (FTHTask*) deq_array[(oldBase & deqLength - 1)];
	}

	this->base = oldBase;
	return NULL;
	//}
}

void FTHDeque::checkOverflow() {
	int t = this->top;
	int b = this->base;

	if (t - b < deqLength - 1) {
		int newBase = b & deqLength - 1;
		int newTop = this->top & deqLength - 1;
		if (newTop < newBase)
			newTop += deqLength;
		this->top = newTop;
		this->base = newBase;

		int i = newBase;
		do {
			deq_array[i] = NULL;
			i = i - 1 & deqLength - 1;

			if (i == newTop)
				break;
		} while (deq_array[i] != NULL);
	} else {
		int newTop = t - b;
		int oldcap = deqLength;
		int newcap = oldcap * 2;

		if (newcap >= 1073741824) {
			throw MaxCapacityExceededException();
		}
		volatile FTHTask** newdeq = new volatile FTHTask*[newcap];

		for (int j = 0; j < oldcap; j++)
			newdeq[j] = deq_array[(b++ & oldcap - 1)];

		for (int j = oldcap; j < newcap; j++)
			newdeq[j] = NULL;
		delete[] deq_array;
		this->deq_array = newdeq;
		this->deqLength = newcap;
		this->base = 0;
		this->top = newTop;
	}
}

} /* namespace NSforThres */
