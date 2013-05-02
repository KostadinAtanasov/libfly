/******************************************************************************
 * fly_task.h
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

#ifndef LIBFLY_FLY_TASK_H
#define LIBFLY_FLY_TASK_H

struct fly_task {
	fly_task_func	func;
	void			*param;
	void			*result;

	/* data used by scheduler to identify this task */
	void			*sched_data;
}; /* struct fly_task */

#endif /* LIBFLY_FLY_TASK_H */
