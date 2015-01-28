/*
 * ForThres.h
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#ifndef FORTHRES_H_
#define FORTHRES_H_
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <exception>

#include <mutex>
#include <condition_variable>
#include <chrono>

#include <vector>

namespace NSforThres{
class FTHDeque;
class FTHThread;
class FTHTask;
class FTHTaskRunner;
class FTHTaskRunnerGroup;

class MaxCapacityExceededException;
}

#include "FTHDeque.h"
//#include "FTHThread.h"
#include "FTHTask.h"
#include "FTHTaskRunner.h"
#include "FTHTaskRunnerGroup.h"

#define _FORTHRES_DEFAULT_DEQUE_SIZE 100


#endif /* FORTHRES_H_ */
