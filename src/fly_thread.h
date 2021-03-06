/******************************************************************************
 * fly_thread.h
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

#ifndef FLY_THREAD_H
#define FLY_THREAD_H

#include <sys/types.h>
#include <pthread.h>
#include <time.h>

typedef void *(*fly_thread_func)(void*);

enum fly_thread_state {
	FLY_THREAD_IDLE				= 0x01,
	FLY_THREAD_RUNNING			= 0x02,
	FLY_THREAD_SLEEP			= 0x04,
	FLY_THREAD_CLIENT_RUNNING	= 0x08,
	FLY_THREAD_CLIENT_ISLEEP	= 0x10,	/* interruptable sleep(mutex, etc) */
	FLY_THREAD_CLIENT_USLEEP	= 0x20,	/* uninterruptable sleep(IO) */
	FLY_THREAD_TRAPPED			= 0x40	/* signal or debugger stop us */
}; /* enum fly_thread_state */

#define FLY_THREAD_CLEAR_OS_BITS_MASK 7

struct fly_thread {
	pthread_t				pthread;
	pthread_attr_t			*attr;

	fly_thread_func			func;
	void					*param;

	enum fly_thread_state	state;

	/* thread id given by the OS */
	pid_t					tid;
	int						fd;
}; /* struct fly_thread */

int fly_thread_init(struct fly_thread *thread,
		fly_thread_func func, void *param);
int fly_thread_uninit(struct fly_thread *thread);
int fly_thread_start(struct fly_thread *thread);

int fly_thread_wait(struct fly_thread *thread);

enum fly_thread_state fly_thread_get_state(struct fly_thread *thread);
int fly_thread_is_client_block(struct fly_thread *thread);
int fly_thread_is_me(struct fly_thread *thread);

static inline void fly_thread_sleep(unsigned int nanosec)
{
	struct timespec delay;
	delay.tv_sec = nanosec / 1000000000;
	delay.tv_nsec = nanosec % 1000000000;
	nanosleep(&delay, NULL);
}

#endif /* FLY_THREAD_H */
