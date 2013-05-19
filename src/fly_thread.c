/******************************************************************************
 * fly_thread.c
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

#include "fly_thread.h"
#include <libfly/fly_error.h>
#include "fly_globals.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h> /* for sprintf */

#define FLY_PROCESS_MAX_NAME_LEN	256
#define FLY_THREAD_DEFAULT_READ_LEN	512
#define FLY_INVALID_FD				-1

#define FLY_PROCFS_STATUS_FMT_STR	"/proc/self/task/%d/status"

/******************************************************************************
 * Proc file system parsing.
 *****************************************************************************/
static enum fly_thread_state get_thread_state(int fd,
	enum fly_thread_state expected_state);

/******************************************************************************
 * fly_thread interface implementation.
 *****************************************************************************/
static void *fly_thread_wrap_func(void *_thread)
{
	struct fly_thread *thread = (struct fly_thread*)_thread;

	char path[512] = {0};

	thread->tid = syscall(SYS_gettid);
	thread->state = FLY_THREAD_RUNNING;

	sprintf(path, FLY_PROCFS_STATUS_FMT_STR, thread->tid);
	thread->fd = open(path, O_RDONLY);

	return thread->func(thread->param);
}

int fly_thread_init(struct fly_thread *thread,
		fly_thread_func func, void *param)
{
	thread->attr = fly_malloc(sizeof(pthread_attr_t));
	if (thread->attr) {
		if (pthread_attr_init(thread->attr) == 0) {
			thread->func = func;
			thread->param = param;
			thread->state = FLY_THREAD_IDLE;
			thread->fd = FLY_INVALID_FD;
			return FLYESUCCESS;
		} else {
			fly_free(thread->attr);
			return FLYEATTR;
		}
	}
	return FLYENORES;
}

int fly_thread_uninit(struct fly_thread *thread)
{
	pthread_attr_destroy(thread->attr);
	fly_free(thread->attr);
	if (thread->fd != FLY_INVALID_FD)
		close(thread->fd);
	return FLYESUCCESS;
}

int fly_thread_start(struct fly_thread *thread)
{
	int ret;
	ret = pthread_create(&thread->pthread, thread->attr,
			fly_thread_wrap_func, thread);
	return ret;
}

int fly_thread_wait(struct fly_thread *thread)
{
	pthread_join(thread->pthread, NULL);
	return FLYESUCCESS;
}

enum fly_thread_state fly_thread_get_state(struct fly_thread *thread)
{
	thread->state &= FLY_THREAD_CLEAR_OS_BITS_MASK;
	thread->state = get_thread_state(thread->fd, thread->state);
	return thread->state;
}

int fly_thread_is_client_block(struct fly_thread *thread)
{
	enum fly_thread_state state = fly_thread_get_state(thread);
	if ((state & FLY_THREAD_CLIENT_ISLEEP)
		|| (state & FLY_THREAD_CLIENT_USLEEP)) {
		if (!(state & FLY_THREAD_SLEEP))
			return 1;
	}
	return 0;
}

int fly_thread_is_me(struct fly_thread *thread)
{
	return pthread_equal(pthread_self(), thread->pthread);
}

/******************************************************************************
 * Proc file system parsing.
 *****************************************************************************/
static enum fly_thread_state get_thread_state(int fd,
	enum fly_thread_state expected_state)
{
	enum fly_thread_state ret = expected_state;
	char buff[FLY_THREAD_DEFAULT_READ_LEN];
	int readed = read(fd, buff, FLY_THREAD_DEFAULT_READ_LEN);
	if (readed != -1) {
		int i;
		const int stateend = 7;
		for (i = 5;; i++) {
			if (buff[i] == '\n') {
				i += stateend;
				while ((buff[i] == '\t') || (buff[i] == ' ')) i++;
				switch (buff[i]) {
				case 'D':
					ret |= FLY_THREAD_CLIENT_USLEEP;
					break;
				case 'R':
					ret |= FLY_THREAD_RUNNING;
					break;
				case 'S':
					ret |= FLY_THREAD_CLIENT_ISLEEP;
					break;
				case 'T':
					ret |= FLY_THREAD_TRAPPED;
					break;
				case 'X':
				case 'Z':
				default:
					/* Should never come here... */
					break;
				}
				break;
			}
		}
	}
	return ret;
}
