/*
 * main.cpp
 *
 *  Created on: 29/ago/2014
 *      Author: Roberto Belli
 */
#include "libforThres/ForThres.h"
#include "examples/Fibonacci.h"
using namespace std;
using namespace NSforThres;


static void dequeTest() {
	FTHDeque deq;
	std::cout << "Tests FTHDeque" << std::endl;
	/*populating deque*/
	for (size_t i = 0; i < 10; i++) {
		deq.push((FTHTask *) i);
	}

	/*test Steal 1*/
	for (size_t i = 0; i < 10; i++) {
		if (((size_t) deq.steal()) != i) {
			cout << "Error 1 Steal" << endl;
			abort();
		}
	}

	/*populating deque*/
	for (size_t i = 0; i < 10; i++) {
		deq.push((FTHTask *) i);
	}

	/*test Pop 1*/
	for (size_t i = 10; i > 0; i--) {
		if (((size_t) deq.pop()) != i - 1) {
			cout << "Error 1 Pop" << endl;
			abort();
		}
	}

	/*test multiple Push*/
	size_t numitems = 1000;
	FTHTask **tasks = new FTHTask*[numitems];
	for (size_t i = 0; i < numitems; i++) {
		tasks[i] = ((FTHTask *) i);
	}

	deq.multiplePush(tasks, numitems);

	/*steal half items*/
	int ext_count = 0;
	for (size_t i = 0; i < numitems / 2; i++) {
		ext_count++;
		if (((size_t) deq.steal()) != i) {
			cout << "Error 2 Steal" << endl;
			abort();
		}
	}

	/*pop other half*/
	for (size_t i = numitems; i > numitems / 2; i--) {
		ext_count++;
		size_t item = (size_t) deq.pop();
		if (item != i - 1) {
			cout << "Error 2 Pop " << item << " != " << i << endl;
			abort();
		}
	}

	if (ext_count != numitems) {
		cout << "Error Test Code " << ext_count << " != " << numitems << endl;
		abort();
	}





	cout << "FTHDeque Test Completed without errors " << endl;

}

static void mainFibonacciTest() { // sample driver
	std::cout << "Tests Fibonacci" << std::endl;
	/*try {*/
      int groupSize = 4;    // 2 worker threads
      unsigned int num = 35;         // compute fib(35) 95 max - 40 = 1 min(4 taskrunner)
      cout << "Fibonacci( " << num  << " ):" << endl;
      FTHTaskRunnerGroup *group = new FTHTaskRunnerGroup(groupSize);
      NSFibonacci::Fibonacci *f = new NSFibonacci::Fibonacci(num);
      group->invoke(f);
      int result = f->getAnswer();
      cout << "Answer: " << result << endl;
      group->stats();
  /*  }
    catch (InterruptedException ex) {
    	cout << "Interrupted" << endl;
    }*/
  }

int main() {
	std::cout << "Tests ForThres" << std::endl;
	//TODO: overflow
	//dequeTest();

	mainFibonacciTest();
	return 0;
}
