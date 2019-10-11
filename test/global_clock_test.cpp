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

#include <basket/clock/global_clock.h>
#include <iostream>
#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank, size;
  int i, j;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int num_servers = 1;
  if (argc > 1) num_servers = atoi(argv[1]);

  if (size < num_servers + 1) {
    std::cout << "MPI ranks should be at least one more than number of " <<
        "servers" << std::endl;
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }

  if (rank == 0) {
    std::cout << num_servers << " Server Test" << std::endl;
  }
  MPI_Barrier(MPI_COMM_WORLD);

  basket::global_clock *clock = new basket::global_clock("ticktock", rank < num_servers,
                                                         rank % num_servers,
                                                         num_servers,
                                                         true);

  for (i = 0; i < size; i++) {
    if (i == rank) {
      std::cout << "Time rank " << rank << ": " << clock->GetTime() <<
          std::endl;
      for (j = 0; j < num_servers; j++) {
        std::cout << "Time server " << j << " from rank " << rank << ": " <<
            clock->GetTimeServer(j) << std::endl;
      }
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  delete clock;
  MPI_Finalize();
}
