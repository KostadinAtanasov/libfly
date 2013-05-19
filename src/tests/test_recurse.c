/******************************************************************************
 * test_recurse.c
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

#include <libfly/fly.h>

#include <stdlib.h> /* for malloc */
#include <math.h> /* for log validation */
#include <stdio.h> /* for sprintf */
#include <unistd.h> /* for sysconf */

/******************************************************************************
 * Profiling stuff
 *****************************************************************************/
#include <sys/time.h>
static inline double get_time_in_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000.0) + tv.tv_usec;
}

static inline double get_time_diff_in_usec(double prevtime)
{
	double nowtime = get_time_in_usec();
	return nowtime - prevtime;
}
/******************************************************************************
 * End of profiling stuff
 *****************************************************************************/

#define NBTASKS			16
#define TASKARRSIZE		16
#define LOG_LOOP_SIZE	5000000
#define LOG_PRECISION	0.001f

static float nat_log(float num)
{
	float res = (num - 1) / (num + 1);
	float curr = res;
	float mult = res * res;
	int i;
	for (i = 3; i < LOG_LOOP_SIZE; i += 2) {
		curr *= mult;
		res += curr / i;
	}
	return 2.f * res;
}

struct task_param {
	float	*arr;
	float	*res;
	int		size;
}; /* struct task_param */

static void parallel_for_func(int ind, void *ptr)
{
	struct task_param *tp = (struct task_param*)ptr;
	tp->res[ind] = nat_log(tp->arr[ind]);
}

static void *task_func(void *param)
{
	struct task_param *tp = (struct task_param*)param;
	fly_parallel_for(tp->size, parallel_for_func, tp);
	return tp;
}

static void init_data(struct task_param *arr, int nbtasks, int taskarrsize)
{
	int i;
	for (i = 0; i < nbtasks; i++) {
		int j;
		arr[i].size = taskarrsize;
		arr[i].arr = malloc(sizeof(float) * taskarrsize);
		arr[i].res = malloc(sizeof(float) * taskarrsize);
		for (j = 0; j < taskarrsize; j++)
			arr[i].arr[j] = (float)i + 0.5f;
	}
}

static void uninit_data(struct task_param *arr, int nbtasks)
{
	int i;
	for (i = 0; i < nbtasks; i++)
		free(arr[i].arr);
}

static void clean_tasks(struct fly_task **tasks, int count)
{
	int i;
	for (i = 0; i < count; i++)
		fly_destroy_task(tasks[i]);
}

static int eq_with_eps(float lhs, float rhs, float ep)
{
	if (lhs > rhs) {
		return rhs >= (lhs - ep);
	} else {
		return lhs >= (rhs - ep);
	}
}

static int validate_log(struct task_param *param, int count)
{
	int i;
	int err = 0;
	for (i = 0; i < count; i++) {
		int j;
		for (j = 0; j < TASKARRSIZE; j++) {
			float mathlog = log(param[i].arr[j]);
			if (!eq_with_eps(param[i].res[j], mathlog, LOG_PRECISION)) {
				char msg[256] = {0};
				sprintf(msg, "bad log ad index: %d; nat_log %f; mat log %f",
						i, param[i].res[j], mathlog);
				fly_log("[test_recurse]", msg);
				err = 1;
			}
		}
	}
	return !err;
}

int main(int argc, char **argv)
{
	int					errcode;
	long				nbcpus;
	double				timestart;
	double				timedelta;
	double				olddelta;
	char				msg[256] = {0};

	int					counter;
	struct fly_task		*tasks[NBTASKS];
	struct task_param	task_params[NBTASKS];

	init_data(task_params, NBTASKS, TASKARRSIZE);

	nbcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (nbcpus < 0)
		nbcpus = 2; /* hardcode to some multithread value... */

	/* start libfly with nbcpus threads */
	errcode = fly_simple_init(nbcpus);
	fly_assert(FLY_SUCCEEDED(errcode), "fly_simple_init failed");

	/* start task profiling - including tasks creation */
	timestart = get_time_in_usec();
	for (counter = 0; counter < NBTASKS; counter++) {
		tasks[counter] = fly_create_task(task_func, &task_params[counter]);
		if (!tasks[counter]) {
			clean_tasks(tasks, counter);
			fly_assert(0, "fly_create_task failed");
			fly_uninit();
			uninit_data(task_params, NBTASKS);
			return -1;
		}
	}

	for (counter = 0; counter < NBTASKS; counter++) {
		errcode = fly_push_task(tasks[counter]);
		if (!FLY_SUCCEEDED(errcode)) {
			fly_wait_tasks(tasks, counter - 1);
			clean_tasks(tasks, NBTASKS);
			fly_assert(0, "fly_push_task failed");
			fly_uninit();
			uninit_data(task_params, NBTASKS);
			return -1;

		}
	}

	errcode = fly_wait_tasks(tasks, NBTASKS);
	timedelta = get_time_diff_in_usec(timestart);
	sprintf(msg, "tasks execution took:\t\t%f us", timedelta);
	fly_log("[test_recurse]", msg);
	(void)errcode;
	fly_assert(FLY_SUCCEEDED(errcode), "flay_wait_tasks return error");
	clean_tasks(tasks, NBTASKS);

	validate_log(task_params, NBTASKS);

	olddelta = timedelta;
	timestart = get_time_in_usec();
	for (counter = 0; counter < NBTASKS; counter++) {
		int j;
		for (j = 0; j < TASKARRSIZE; j++) {
			parallel_for_func(j, &task_params[counter]);
		}
	}
	timedelta = get_time_diff_in_usec(timestart);
	sprintf(msg, "for loop execution took:\t%f us", timedelta);
	fly_log("[test_recurse]", msg);
	sprintf(msg, "speed bump:\t\t\t%f", timedelta / olddelta);
	fly_log("[test_recurse]", msg);

	/* shutdown libfly */
	errcode = fly_uninit();
	(void)errcode;
	fly_assert(FLY_SUCCEEDED(errcode), "fly_uninit failed");

	uninit_data(task_params, NBTASKS);

	return 0;
}
