/******************************************************************************
 * fly_globals.h
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

#ifndef	LIBFLY_FLY_GLOBALS_H
#define LIBFLY_FLY_GLOBALS_H

#include <libfly/fly.h>

extern struct fly_sched fly_sched;

extern fly_malloc_func fly_malloc;
extern fly_free_func   fly_free;

#endif /* LIBFLY_FLY_GLOBALS_H */
