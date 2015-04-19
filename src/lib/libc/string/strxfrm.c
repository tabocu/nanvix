/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright(C) 2011-2015 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Nanvix.
 * 
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * 
 * @brief strxfrm() implementation.
 */
 
#include <sys/types.h>

/**
 * @brief String transformation.
 * 
 * @details Transforms the string pointed to by @p s2 and places the resulting
 *          string into the array pointed to by @p s1. The transformation is
 *          such that if strcmp() is applied to two transformed strings,
 *          it returns a value greater than, equal to or less than 0,
 *          corresponding to the result of strcoll() applied to the same two
 *          original strings. No more than @p n bytes are placed into the
 *          resulting array pointed to by @p s1, including the terminating null
 *          byte. If @p n is 0, @p s1 is permitted to be a null pointer. If
 *          copying takes place between objects that overlap, the behavior is
 *          undefined.
 * 
 * @returns The length of the transformed string (not including the terminating
 *          null byte) is returned. If the value returned is @p n or more, the
 *          contents of the array pointed to by @p s1 are indeterminate.
 * 
 *          On error, errno is set, but no return value is reserved to indicate
 *          an error. 
 */
size_t strxfrm(char *s1, const char *s2, size_t n)
{
	size_t res;
	res = 0;
	
	while ((n-- > 0) && (*s2 != '\0'))
	{
		*s1++ = *s2++;
		res++;
	}
	
	while (*s2)
	{
		s2++;
		res++;
	}

	return (res);
}