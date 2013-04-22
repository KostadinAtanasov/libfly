/******************************************************************************
 * fly_atomic.h
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

#ifndef	LIBFLY_FLY_ATOMIC_H
#define LIBFLY_FLY_ATOMIC_H

#define fly_atomic_inc(ptr, val) \
	__sync_add_and_fetch(ptr, val)

#define fly_atomic_dec(ptr, val) \
	__sync_sub_and_fetch(ptr, val);

#define fly_atomic_cas(ptr, desired, val) \
	__sync_bool_compare_and_swap(ptr, desired, val)

#endif /* LIBFLY_FLY_ATOMIC_H */
