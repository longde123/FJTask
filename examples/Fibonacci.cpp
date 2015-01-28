/*
 * Fibonacci.cpp
 *
 *  Created on: 30/ago/2014
 *      Author: bob
 */

#include "Fibonacci.h"

namespace NSFibonacci {

Fibonacci::Fibonacci(unsigned int n) {
	number = n;
}

Fibonacci::~Fibonacci() {
	// TODO Auto-generated destructor stub
}

// Computes fibonacci(n) = fibonacci(n-1) + fibonacci(n-2);  for n> 1
 //          fibonacci(0) = 0;
 //          fibonacci(1) = 1.
void Fibonacci::run() {
	int n = number;
	//TODO: set threshold
	if (n > 1) {
		Fibonacci *f1 = new Fibonacci(n - 1);
		Fibonacci *f2 = new Fibonacci(n - 2);

		coInvoke(f1, f2); // run these in parallel

		// we know f1 and f2 are computed, so just directly access numbers
		number = f1->number + f2->number;
	}
}

int Fibonacci::getAnswer() {
	if (!isDone())
		throw NotDoneException();
	return number;
}

} /* namespace NSFibonacci */

/*public class Fib extends FJTask {

 // Computes fibonacci(n) = fibonacci(n-1) + fibonacci(n-2);  for n> 1
 //          fibonacci(0) = 0;
 //          fibonacci(1) = 1.

 // Value to compute fibonacci function for.
 // It is replaced with the answer when computed.
 private volatile int number;

 public Fib(int n) { number = n; }

 public int getAnswer() {
 if (!isDone()) throw new Error("Not yet computed");
 return number;
 }

 public void run() {
 int n = number;
 if (n > 1) {
 Fib f1 = new Fib(n - 1);
 Fib f2 = new Fib(n - 2);

 coInvoke(f1, f2); // run these in parallel

 // we know f1 and f2 are computed, so just directly access numbers
 number = f1.number + f2.number;
 }
 }

 public static void main(String[] args) { // sample driver
 try {
 int groupSize = 2;    // 2 worker threads
 int num = 35;         // compute fib(35)
 FJTaskRunnerGroup group = new FJTaskRunnerGroup(groupSize);
 Fib f = new Fib(num);
 group.invoke(f);
 int result = f.getAnswer();
 System.out.println(" Answer: " + result);
 }
 catch (InterruptedException ex) {
 System.out.println("Interrupted");
 }
 }
 }
 */
