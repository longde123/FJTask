/*
 * FTHTask.cpp
 *
 *  Created on: Aug 7, 2014
 *      Author: bob
 */

#include "FTHTask.h"

namespace NSforThres {

FTHTask::FTHTask() {
	// TODO Auto-generated constructor stub
	taskRunner = NULL;
}

FTHTask::~FTHTask() {
	// TODO Auto-generated destructor stub
}

FTHTaskRunner* FTHTask::getFTHTaskRunner() {
	return taskRunner;
}

void FTHTask::setFTHTaskRunner(FTHTaskRunner* t) {
	taskRunner = t;
}

FTHTaskRunnerGroup* FTHTask::getFTHTaskRunnerGroup() {
	return getFTHTaskRunner()->getGroup();
}

bool FTHTask::isDone() {
	return done;
}

void FTHTask::cancel(){
	setDone();
}

void FTHTask::reset(){
	done = false;
}

void FTHTask::start() {
	getFTHTaskRunnerGroup()->executeTask(this);
}

void FTHTask::yield() {
	getFTHTaskRunner()->taskYield();
}



void FTHTask::fork() {
	getFTHTaskRunner()->deq->push(this);
}

void FTHTask::join() {
	getFTHTaskRunner()->taskJoin(this);
}

void FTHTask::invoke(FTHTask *t) {
	if (!t->isDone()) {
		t->run();
		t->setDone();
	}
}

void FTHTask::coInvoke(FTHTask* task1, FTHTask* task2) {
	getFTHTaskRunner()->coInvoke(task1, task2);
}

void FTHTask::coInvoke(FTHTask **tasks, int count){
	getFTHTaskRunner()->coInvoke(tasks, count);
}

void FTHTask::setDone() {
	done = true;
}

} /* namespace NSforThres */
