/******************************************************************************
 * fly_sem.h
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

#ifndef LIBFLY_FLY_SEM_H
#define LIBFLY_FLY_SEM_H

#include "fly_thread.h"

#include <semaphore.h>

struct fly_sem {
	sem_t	sem;
}; /* struct fly_sem */

static inline int fly_sem_init(struct fly_sem *sem)
{
	return sem_init(&sem->sem, 0, 0);
}

static inline int fly_sem_uninit(struct fly_sem *sem)
{
	return sem_destroy(&sem->sem);
}

static inline int fly_sem_wait(struct fly_sem *sem, struct fly_thread *thread)
{
	int err;
	thread->state = FLY_THREAD_SLEEP;
	err = sem_wait(&sem->sem);
	thread->state = FLY_THREAD_RUNNING;
	return err;
}

static inline int fly_sem_notrack_wait(struct fly_sem *sem)
{
	return sem_wait(&sem->sem);
}

static inline int fly_sem_post(struct fly_sem *sem)
{
	return sem_post(&sem->sem);
}

#endif /* LIBFLY_FLY_SEM_H */
