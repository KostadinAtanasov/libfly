/******************************************************************************
 * fly_memdebug.h
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

#ifndef	LIBFLY_FLY_MEMDEUG_H
#define LIBFLY_FLY_MEMDEUG_H

#ifdef FLY_MEMDEBUG

#include <libfly/fly_debugging.h>

#include <pthread.h>

pthread_mutex_t	fly_mem_lock;

size_t fly_memc = 0;
size_t fly_mem_array_size;

void **fly_mem_ptrs = NULL;
size_t *fly_mem_sizes = NULL;

#define FLY_MEM_START_ARR_SIZE 256

static inline int fly_mem_count_alloc_try_add(void *ptr, size_t size, size_t i)
{
	for (fly_memc = i; fly_memc < fly_mem_array_size; fly_memc++) {
		if (!fly_mem_ptrs[fly_memc]) {
			fly_mem_ptrs[fly_memc] = ptr;
			fly_mem_sizes[fly_memc] = size;
			return 1;
		}
	}
	return 0;
}

static inline void fly_init_memdebugf()
{
	fly_mem_array_size = FLY_MEM_START_ARR_SIZE;
	fly_mem_ptrs = malloc(FLY_MEM_START_ARR_SIZE * sizeof(void*));
	fly_mem_sizes = malloc(FLY_MEM_START_ARR_SIZE * sizeof(size_t));
	for (fly_memc = 0; fly_memc < fly_mem_array_size; fly_memc++) {
		fly_mem_ptrs[fly_memc] = NULL;
		fly_mem_sizes[fly_memc] = 0;
	}
	pthread_mutex_init(&fly_mem_lock, NULL);
}

#define fly_init_memdebug()\
	fly_init_memdebugf()

#define fly_uninit_memdebug()\
	pthread_mutex_destroy(&fly_mem_lock)

#define fly_memdebug_report(printfunc)\
	pthread_mutex_lock(&fly_mem_lock);\
	printfunc("starting memleak list...\n");\
	for (fly_memc = 0; fly_memc < fly_mem_array_size; fly_memc++) {\
		if (fly_mem_ptrs[fly_memc]) {\
			printfunc("address: %p in use with: %u bttes\n",\
				fly_mem_ptrs[fly_memc],\
				(unsigned int)fly_mem_sizes[fly_memc]);\
		}\
	}\
	printfunc("end of memleak list.\n");\
	pthread_mutex_unlock(&fly_mem_lock)

#define fly_mem_count_alloc(ptr, size)\
	pthread_mutex_lock(&fly_mem_lock);\
	if (ptr) {\
		if (!fly_mem_count_alloc_try_add(ptr, size, 0)) {\
			unsigned long oldsize = fly_mem_array_size;\
			void *tmp;\
			size_t alloc_size;\
			fly_mem_array_size *= 2;\
			alloc_size = fly_mem_array_size * sizeof(void*);\
			tmp = realloc(fly_mem_ptrs, alloc_size);\
			if (tmp)\
				fly_mem_ptrs = tmp;\
			alloc_size = fly_mem_array_size * sizeof(size_t);\
			tmp = realloc(fly_mem_sizes, alloc_size);\
			if (tmp)\
				fly_mem_sizes = tmp;\
			fly_mem_count_alloc_try_add(ptr, size, oldsize);\
		}\
	}\
	pthread_mutex_unlock(&fly_mem_lock)

#define fly_mem_count_free(ptr)\
	pthread_mutex_lock(&fly_mem_lock);\
	for (fly_memc = 0; fly_memc < fly_mem_array_size; fly_memc++) {\
		if (fly_mem_ptrs[fly_memc] == ptr) {\
			fly_mem_ptrs[fly_memc] = NULL;\
			fly_mem_sizes[fly_memc] = 0;\
			break;\
		}\
	}\
	pthread_mutex_unlock(&fly_mem_lock)

#else

#define fly_init_memdebug()
#define fly_uninit_memdebug()
#define fly_memdebug_report(printfunc)
#define fly_mem_count_alloc(ptr, size)
#define fly_mem_count_free(ptr)

#endif /* FLY_MEMDEBUG */

#endif /* LIBFLY_FLY_MEMDEUG_H */
