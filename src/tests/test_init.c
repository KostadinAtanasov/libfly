/******************************************************************************
 * test_init.c
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
#include <libfly/fly_debugging.h>

#include <unistd.h> /* for sysconf */

int main(int argc, char **argv)
{
	int errcode = FLYESUCCESS;
	long nbcpus;
	
	nbcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (nbcpus < 0)
		nbcpus = 2; /* hardcode to some multithread value... */

	errcode = fly_simple_init(nbcpus);
	fly_assert(FLY_SUCCEEDED(errcode), "fly_simple_init failed");

	errcode = fly_uninit();
	fly_assert(FLY_SUCCEEDED(errcode), "fly_uninit failed");
	(void)errcode;

	fly_log("[test_init]", "All tests pass!");

	return 0;
}
