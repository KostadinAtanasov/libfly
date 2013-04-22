/******************************************************************************
 * fly_debugging.h
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

#ifndef LIBFLY_FLY_DEBUGGING_H
#define LIBFLY_FLY_DEBUGGING_H

#if defined(FLY_ENABLE_LOG) || defined(FLY_ENABLE_ASSERT)
#include <stdio.h>
#endif

#if defined(FLY_ENABLE_ASSERT)
#include <assert.h>
#endif

#ifdef FLY_ENABLE_LOG

static inline void fly_do_log(const char *subsys, const char *msg)
{
	printf(subsys);
	printf(" ");
	printf(msg);
	printf("\n");
}

#define fly_log(subsys, msg)\
	fly_do_log(subsys, msg)

#else

#define fly_log(subsys, mesg)

#endif /* FLY_ENABLE_LOG */

#ifdef FLY_ENABLE_ASSERT

#define fly_assert(cond, msg)\
	if (!cond)\
		printf(msg);\
	assert(cond)

#else

#define fly_assert(cond, msg)

#endif /* FLY_ENABLE_ASSERT */

#endif /* LIBFLY_FLY_DEBUGGING_H */

