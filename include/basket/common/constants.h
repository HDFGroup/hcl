/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of Basket
 * 
 * Basket is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_BASKET_COMMON_CONSTANTS_H_
#define INCLUDE_BASKET_COMMON_CONSTANTS_H_
#include <stdint.h>
#include <basket/common/data_structures.h>

const uint16_t RPC_PORT = 8080;
const uint16_t RPC_THREADS = 1;
const int TEST_REQUEST_SIZE = 1000;
const CharStruct PATH_SEPARATOR = "/";

#endif  // INCLUDE_BASKET_COMMON_CONSTANTS_H_
