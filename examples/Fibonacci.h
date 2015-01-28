/*
 * Fibonacci.h
 *
 *  Created on: 30/ago/2014
 *      Author: bob
 */

#ifndef FIBONACCI_H_
#define FIBONACCI_H_

#include <forThres.h>

namespace NSFibonacci {

class Fibonacci: public NSforThres::FTHTask {
private:
	// Value to compute fibonacci function for.
	// It is replaced with the answer when computed.
	volatile unsigned long long number;
public:
	Fibonacci(unsigned int n);
	virtual ~Fibonacci();
	void run();
	int getAnswer();
};

class NotDoneException: public std::exception {
	virtual const char* what() const throw () {
		return "Fibonacci: calculation not completed";
	}
};

} /* namespace NSFibonacci */

#endif /* FIBONACCI_H_ */
