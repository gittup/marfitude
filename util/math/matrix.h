/*
   Marfitude
   Copyright (C) 2006 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/** @file
 * Yay...matricizeamalize
 */

#include "matrix_t.h"

void matrix_3x3inv(union matrix *dest, const union matrix *src);
void matrix_mul(union matrix *dest, const union matrix *src);
void matrix_copy(union matrix *dest, const union matrix *src);
void matrix_transpose(union matrix *dest, const union matrix *src);
void matrix_print(union matrix *m);

/** Macro to print the name of the matrix, and then display the matrix. */
#define print_matrix(s) do {printf("Matrix: "#s"\n"); matrix_print(s); printf("\n");} while(0)
