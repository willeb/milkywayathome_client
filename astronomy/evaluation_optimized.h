/*
Copyright 2008, 2009 Travis Desell, Dave Przybylo, Nathan Cole,
Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik Magdon-Ismail
and Rensselaer Polytechnic Institute.

This file is part of Milkway@Home.

Milkyway@Home is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Milkyway@Home is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASTRONOMY_EVALUATION_H
#define ASTRONOMY_EVALUATION_H

#include "evaluation_state.h"
#include "parameters.h"
#include "star_points.h"

int	calculate_integrals(ASTRONOMY_PARAMETERS* ap, EVALUATION_STATE* es, STAR_POINTS* sp);
int	calculate_likelihood(ASTRONOMY_PARAMETERS* ap, EVALUATION_STATE* es, STAR_POINTS* sp);

#endif


