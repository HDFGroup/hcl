/*
 * Copyright (C) 2019  Hariharan Devarajan, Keith Bateman
 *
 * This file is part of HCL
 * 
 * HCL is free software: you can redistribute it and/or modify
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

#include <hcl/common/data_structures.h>
/**
 * Outstream conversions
 */
std::ostream &operator<<(std::ostream &os, char const *m) {
    return os << std::string(m);
}
std::ostream &operator<<(std::ostream &os, uint8_t const &m) {
    return os << std::to_string(m);
}
std::ostream &operator<<(std::ostream &os, CharStruct const &m){
    return os   << "{TYPE:CharStruct," << "value:" << m.c_str()<<"}";
}
