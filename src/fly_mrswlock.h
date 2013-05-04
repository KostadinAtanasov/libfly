/******************************************************************************
 * fly_mrswlock.h
 *
 * Copyright (C) 2013 Kostadin Atanasov <pranayama111@gmail.com>
 *
 * This file is part of libfly.
 *
 * libfly is free software: you can redistribute it and/or modify
 * if under the terms of GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * libfly is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should received a copy of the GNU Lesser General Public License
 * along with libfly. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

/*
 * Multiple Readers Single Writer Lock.
 * Simple non recursive lock, which allows up to maxin readers at the same time
 * or just one writer. To prevent writers starvation writers have the chance to
 * acquire the lock before any reader.
 */

#ifndef LIBFLY_FLY_MRSWLOCK_H
#define LIBFLY_FLY_MRSWLOCK_H

#include "fly_thread.h"

#include <pthread.h>
#include <semaphore.h>

struct fly_mrswlock {
	pthread_mutex_t	plock;
	sem_t			rsem;
	sem_t			wsem;
	int				nbin;
	int				nbrwaiting;
	int				nbwwaiting;
	int				maxin;
}; /* struct fly_mrswlock */

static inline int fly_mrswlock_init(struct fly_mrswlock *lock, int maxin)
{
	pthread_mutex_init(&lock->plock, NULL);
	sem_init(&lock->rsem, 0, 0);
	sem_init(&lock->wsem, 0, 0);
	lock->nbin = 0;
	lock->nbrwaiting = 0;
	lock->nbwwaiting = 0;
	lock->maxin = maxin;
	return 0;
}

static inline void fly_mrswlock_uninit(struct fly_mrswlock *lock)
{
	pthread_mutex_destroy(&lock->plock);
	sem_destroy(&lock->rsem);
	sem_destroy(&lock->wsem);
}

static inline int fly_mrswlock_notrack_rlock(struct fly_mrswlock *lock)
{
	int hasit = 0;
	int waiting = 0;
	int wakewriters = 0;
	while (!hasit) {
		pthread_mutex_lock(&lock->plock);
		if (lock->nbin < lock->maxin) {
			if (lock->nbwwaiting == 0) {
				lock->nbin++;
				hasit = 1;
				if (waiting)
					lock->nbrwaiting--;
			} else {
				wakewriters = 1;
			}
			pthread_mutex_unlock(&lock->plock);
		} else {
			if (!waiting) {
				lock->nbrwaiting++;
				waiting = 1;
			}
			pthread_mutex_unlock(&lock->plock);
			if (wakewriters) {
				wakewriters = 0;
				sem_post(&lock->wsem);
			} else {
				sem_wait(&lock->rsem);
			}
		}
	}
	return hasit;
}

static inline int fly_mrswlock_notrack_wlock(struct fly_mrswlock *lock)
{
	int hasit = 0;
	int waiting = 0;
	while (!hasit) {
		pthread_mutex_lock(&lock->plock);
		if (lock->nbin == 0) {
			lock->nbin = lock->maxin;
			hasit = 1;
			if (waiting)
				lock->nbwwaiting--;
			pthread_mutex_unlock(&lock->plock);
		} else {
			if (!waiting) {
				lock->nbwwaiting++;
				waiting = 1;
			}
			pthread_mutex_unlock(&lock->plock);
			sem_wait(&lock->wsem);
		}
	}
	return hasit;
}

static inline int fly_mrswlock_rlock(struct fly_mrswlock *lock,
		struct fly_thread *thread)
{
	int ret;
	thread->state = FLY_THREAD_SLEEP;
	ret = fly_mrswlock_notrack_rlock(lock);
	thread->state = FLY_THREAD_RUNNING;
	return ret;
}

static inline int fly_mrswlock_wlock(struct fly_mrswlock *lock,
		struct fly_thread *thread)
{
	int ret;
	thread->state = FLY_THREAD_SLEEP;
	ret = fly_mrswlock_notrack_wlock(lock);
	thread->state = FLY_THREAD_RUNNING;
	return ret;
}

static inline int fly_mrswlock_runlock(struct fly_mrswlock *lock)
{
	int nbwwaiting = 0;
	int nbrwaiting = 0;
	pthread_mutex_lock(&lock->plock);
	lock->nbin--;
	nbrwaiting = lock->nbrwaiting;
	nbwwaiting = lock->nbwwaiting;
	pthread_mutex_unlock(&lock->plock);
	if (nbwwaiting > 0)
		sem_post(&lock->wsem);
	else if (nbrwaiting > 0)
		sem_post(&lock->rsem);
	return 0;
}

static inline int fly_mrswlock_wunlock(struct fly_mrswlock *lock)
{
	int nbrwaiting = 0;
	int nbwwaiting = 0;
	pthread_mutex_lock(&lock->plock);
	lock->nbin = 0;
	nbrwaiting = lock->nbrwaiting;
	nbwwaiting = lock->nbwwaiting;
	pthread_mutex_unlock(&lock->plock);
	if (nbwwaiting > 0)
		sem_post(&lock->wsem);
	else if (nbrwaiting > 0)
		sem_post(&lock->rsem);
	return 0;
}

#endif /* LIBFLY_FLY_MRSWLOCK_H */
