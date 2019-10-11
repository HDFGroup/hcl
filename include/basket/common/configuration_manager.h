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

//
// Created by hariharan on 3/19/19.
//

#ifndef INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H
#define INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H

#include <basket/common/debug.h>
#include <basket/common/enumerations.h>
#include <basket/common/singleton.h>
#include <fstream>
#include <vector>
#include <basket/common/data_structures.h>
#include "typedefs.h"

namespace basket{

    class ConfigurationManager {
    public:
        uint16_t RPC_PORT;
        uint16_t RPC_THREADS;
        RPCImplementation RPC_IMPLEMENTATION;
        int MPI_RANK, COMM_SIZE;
        CharStruct TCP_CONF;
        CharStruct VERBS_CONF;
        CharStruct VERBS_DOMAIN;
        really_long MEMORY_ALLOCATED;

        bool IS_SERVER;
        uint16_t MY_SERVER;
        int NUM_SERVERS;
        bool SERVER_ON_NODE;
        CharStruct SERVER_LIST_PATH;
        std::vector<CharStruct> SERVER_LIST;
        CharStruct BACKED_FILE_DIR;

        bool DYN_CONFIG;  // Does not do anything (yet)

      ConfigurationManager():
              SERVER_LIST(),
              BACKED_FILE_DIR("/dev/shm"),
              MEMORY_ALLOCATED(1024ULL * 1024ULL * 128ULL),
              RPC_PORT(8080), RPC_THREADS(1),
#if defined(BASKET_ENABLE_RPCLIB)
              RPC_IMPLEMENTATION(RPCLIB),
#elif defined(BASKET_ENABLE_THALLIUM_TCP)
        RPC_IMPLEMENTATION(THALLIUM_TCP),
#elif defined(BASKET_ENABLE_THALLIUM_ROCE)
        RPC_IMPLEMENTATION(THALLIUM_ROCE),
#endif
              TCP_CONF("ofi+tcp"), VERBS_CONF("verbs"), VERBS_DOMAIN("mlx5_0"),
              IS_SERVER(false), MY_SERVER(0), NUM_SERVERS(1),
              SERVER_ON_NODE(true), SERVER_LIST_PATH("./server_list"), DYN_CONFIG(false) {
          AutoTrace trace = AutoTrace("ConfigurationManager");
          MPI_Comm_size(MPI_COMM_WORLD, &COMM_SIZE);
          MPI_Comm_rank(MPI_COMM_WORLD, &MPI_RANK);
      }

        std::vector<CharStruct> LoadServers(){
          SERVER_LIST=std::vector<CharStruct>();
          fstream file;
          file.open(SERVER_LIST_PATH.c_str(), ios::in);
          if (file.is_open()) {
              std::string file_line;
              std::string server_node_name;
              int count;
              while (getline(file, file_line)) {
                  int split_loc = file_line.find(':');  // split to node and net
                  if (split_loc != std::string::npos) {
                      server_node_name = file_line.substr(0, split_loc);
                      count = atoi(file_line.substr(split_loc+1, std::string::npos).c_str());
                  } else {
                      // no special network
                      server_node_name = file_line;
                      count = 1;
                  }
                  // server list is list of network interfaces
                  for(int i=0;i<count;++i){
                      SERVER_LIST.emplace_back(server_node_name);
                  }
              }
          } else {
              printf("Error: Can't open server list file %s\n", SERVER_LIST_PATH.c_str());
              exit(EXIT_FAILURE);
          }
          NUM_SERVERS = SERVER_LIST.size();
          file.close();
          return SERVER_LIST;
      }
      void ConfigureDefaultClient(std::string server_list_path=""){
          if(server_list_path!="") SERVER_LIST_PATH = server_list_path;
          LoadServers();
          IS_SERVER=false;
          MY_SERVER=MPI_RANK%NUM_SERVERS;
          SERVER_ON_NODE=false;
      }

        void ConfigureDefaultServer(std::string server_list_path=""){
            if(server_list_path!="") SERVER_LIST_PATH = server_list_path;
            LoadServers();
            IS_SERVER=true;
            MY_SERVER=MPI_RANK%NUM_SERVERS;
            SERVER_ON_NODE=true;
        }
    };

}

#endif //INCLUDE_BASKET_COMMON_CONFIGURATION_MANAGER_H
