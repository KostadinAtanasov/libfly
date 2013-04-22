/******************************************************************************
 * test_parallel_for.c
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

#include <math.h> /* for the cos validation */
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

#define DATA_SIZE 256
#define CALCCOS_LOOP_SIZE 10000000
#define CALCCOS_PRECISION 0.001f

float cosarr[DATA_SIZE];
float cosarr_validator[DATA_SIZE];
float cosarr_validator2[DATA_SIZE];
float sinarr_validator[DATA_SIZE];
float sinarr_validator2[DATA_SIZE];

static void init_cosarr()
{
	int i;
	for (i = 0; i < DATA_SIZE; i++) {
		if (i > 0)
			cosarr[i] = 10.f / (float)i;
		else
			cosarr[i] = 0.f;
		cosarr_validator[i] = 10.f;
	}
}

static int eq_with_eps(float lhs, float rhs, float ep)
{
	if (lhs > rhs) {
		return rhs >= (lhs - ep);
	} else {
		return lhs >= (rhs - ep);
	}
}

static int validate_cosarr(float epsilon)
{
	int i;
	int goterr = 0;
	for (i = 0; i < DATA_SIZE; i++) {
		float c = cos(cosarr[i]);
		float d = cosarr_validator[i];
		if (!eq_with_eps(c, d, epsilon)) {
			char msg[256] = {0};
			sprintf(msg, "failing index %d; calccos: %f; math.h cos: %f",
					i, d, c);
			fly_log("[test_parallel_for]", msg);
			goterr = 1;
		}
	}
	return !goterr;
}

static float calccos(float rad)
{
	float res = 1.f;
	float step = 1.f;
	int count = 1;
	for (; count < CALCCOS_LOOP_SIZE; count++) {
		step = (step * (-rad) * rad) / (((2 * count) - 1) * (2 * count));
		res += step;
	}
	return res;
}

static float calcsin(float rad)
{
	float res = rad;
	float step = rad;
	int count = 3;
	for (; count < (2*CALCCOS_LOOP_SIZE); count+=2) {
		step = (step * (-rad) * rad) / ((count - 1) * count);
		res += step;
	}
	return res;
}

void calccos_parallel(int ind, void *num)
{
	float rad = *(float*)num;
	cosarr_validator[ind] = calccos(rad);
	sinarr_validator[ind] = calcsin(rad);
}

void calccos_parallel_ptr(int ind, void *ptr)
{
	float *arr = (float*)ptr;
	cosarr_validator[ind] = calccos(arr[ind]);
	sinarr_validator[ind] = calcsin(arr[ind]);
}

int main(int argc, char **argv)
{
	int errcode = FLYESUCCESS;
	double timestart;
	double timedelta;
	char msg[256];
	long nbcpus;
	int counter;

	nbcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (nbcpus < 0)
		nbcpus = 2; /* hardcode to some multithread value... */

	/* init the data */
	init_cosarr();

	/* start libfly with 4 threads */
	errcode = fly_simple_init(nbcpus);
	fly_assert(FLY_SUCCEEDED(errcode), "fly_simple_init failed");

	timestart = get_time_in_usec();
	errcode = fly_parallel_for_arr(0, DATA_SIZE, calccos_parallel,
			cosarr, sizeof(float));
	timedelta = get_time_diff_in_usec(timestart);
	sprintf(msg, "parallel array for took:   %f us", timedelta);
	fly_log("[test_parallel_for]", msg);
	fly_assert(FLY_SUCCEEDED(errcode), "fly_parallel_for_arr failed");
	validate_cosarr(CALCCOS_PRECISION);
	if (FLY_SUCCEEDED(errcode)) {
		double olddelta = timedelta;
		timestart = get_time_in_usec();
		for (counter = 0; counter < DATA_SIZE; counter++) {
			cosarr_validator2[counter] = calccos(cosarr[counter]);
			sinarr_validator2[counter] = calcsin(cosarr[counter]);
		}
		timedelta = get_time_diff_in_usec(timestart);
		sprintf(msg, "sequential for took: %f us", timedelta);
		fly_log("[test_parallel_for]", msg);
		sprintf(msg, "speed bump         : %f", timedelta / olddelta);
		fly_log("[test_parallel_for]", msg);
	}

	for (counter = 0; counter < DATA_SIZE; counter++) {
		cosarr_validator[counter] = 0.f;
		sinarr_validator[counter] = 0.f;
	}

	timestart = get_time_in_usec();
	errcode = fly_parallel_for(DATA_SIZE, calccos_parallel_ptr, cosarr);
	timedelta = get_time_diff_in_usec(timestart);
	sprintf(msg, "parallel ptr for took:   %f us", timedelta);
	fly_log("[test_parallel_for]", msg);
	fly_assert(FLY_SUCCEEDED(errcode), "fly_parallel_for failed");
	validate_cosarr(CALCCOS_PRECISION);

	/* shutdown libfly */
	errcode = fly_uninit();
	(void)errcode;
	fly_assert(FLY_SUCCEEDED(errcode), "fly_uninit failed");

	errcode = validate_cosarr(CALCCOS_PRECISION);
	fly_assert(errcode, "fly_parallel_for not valid result");
	fly_log("[test_parallel_for]", "All tests pass!");

	return 0;
}
