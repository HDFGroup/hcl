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
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <utility>
#include <mpi.h>
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>
#include <queue>
#include <basket/common/data_structures.h>
#include <basket/queue/queue.h>

struct KeyType{
    size_t a;
    KeyType():a(0){}
    KeyType(size_t a_):a(a_){}
#ifdef BASKET_ENABLE_RPCLIB
    MSGPACK_DEFINE(a);
#endif
    /* equal operator for comparing two Matrix. */
    bool operator==(const KeyType &o) const {
        return a == o.a;
    }
    KeyType& operator=( const KeyType& other ) {
        a = other.a;
        return *this;
    }
    bool operator<(const KeyType &o) const {
        return a < o.a;
    }
    bool operator>(const KeyType &o) const {
        return a > o.a;
    }
    bool Contains(const KeyType &o) const {
        return a==o.a;
    }
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
    template<typename A>
    void serialize(A& ar) const {
        ar & a;
    }
#endif
};
namespace std {
    template<>
    struct hash<KeyType> {
        size_t operator()(const KeyType &k) const {
            return k.a;
        }
    };
}


int main (int argc,char* argv[])
{
    int provided;
    MPI_Init_thread(&argc,&argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        printf("Didn't receive appropriate MPI threading specification\n");
        exit(EXIT_FAILURE);
    }
    int comm_size,my_rank;
    MPI_Comm_size(MPI_COMM_WORLD,&comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
    int ranks_per_server=comm_size,num_request=10000;
    long size_of_request=1000;
    bool debug=false;
    bool server_on_node=false;
    if(argc > 1)    ranks_per_server = atoi(argv[1]);
    if(argc > 2)    num_request = atoi(argv[2]);
    if(argc > 3)    size_of_request = (long)atol(argv[3]);
    if(argc > 4)    server_on_node = (bool)atoi(argv[4]);
    if(argc > 5)    debug = (bool)atoi(argv[5]);

   /* if(comm_size/ranks_per_server < 2){
        perror("comm_size/ranks_per_server should be atleast 2 for this test\n");
        exit(-1);
    }*/
    int len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &len);
    if (debug) {
        printf("%s/%d: %d\n", processor_name, my_rank, getpid());
    }
    
    if(debug && my_rank==0){
        printf("%d ready for attach\n", comm_size);
        fflush(stdout);
        getchar();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    bool is_server=(my_rank+1) % ranks_per_server == 0;
    int my_server=my_rank / ranks_per_server;
    int num_servers=comm_size/ranks_per_server;

    // The following is used to switch to 40g network on Ares.
    // This is necessary when we use RoCE on Ares.
    std::string proc_name = std::string(processor_name);
    /*int split_loc = proc_name.find('.');
    std::string node_name = proc_name.substr(0, split_loc);
    std::string extra_info = proc_name.substr(split_loc+1, string::npos);
    proc_name = node_name + "-40g." + extra_info;*/

    size_t size_of_elem = sizeof(int);

    printf("rank %d, is_server %d, my_server %d, num_servers %d\n",my_rank,is_server,my_server,num_servers);

    const int array_size=TEST_REQUEST_SIZE;

    if (size_of_request != array_size) {
        printf("Please set TEST_REQUEST_SIZE in include/basket/common/constants.h instead. Testing with %d\n", array_size);
    }

    std::array<int,array_size> my_vals=std::array<int,array_size>();

    
    BASKET_CONF->IS_SERVER = is_server;
    BASKET_CONF->MY_SERVER = my_server;
    BASKET_CONF->NUM_SERVERS = num_servers;
    BASKET_CONF->SERVER_ON_NODE = server_on_node || is_server;
    BASKET_CONF->SERVER_LIST_PATH = "./server_list";

    basket::queue<KeyType> *queue;
    if (is_server) {
        queue = new basket::queue<KeyType>();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        queue = new basket::queue<KeyType>();
    }

    std::queue<KeyType> lqueue=std::queue<KeyType>();

    MPI_Comm client_comm;
    MPI_Comm_split(MPI_COMM_WORLD, !is_server, my_rank, &client_comm);
    int client_comm_size;
    MPI_Comm_size(client_comm, &client_comm_size);
    // if(is_server){
    //     std::function<int(int)> func=[](int x){ std::cout<<x<<std::endl;return x; };
    //     int a;
    //     std::function<std::pair<bool,int>(KeyType&,std::array<int, array_size>&,std::string,int)> putFunc(std::bind(&basket::queue<KeyType,std::array<int,
    //                                                                                                                 array_size>>::LocalPutWithCallback<int,int>,queue,std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, std::placeholders::_4));
    //     queue->Bind("CB_Put", func, "APut",putFunc);
    // }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        Timer llocal_queue_timer=Timer();
        std::hash<KeyType> keyHash;
        /*Local std::queue test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            llocal_queue_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            lqueue.push(KeyType(val));
            llocal_queue_timer.pauseTime();
        }

        double llocal_queue_throughput=num_request/llocal_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        Timer llocal_get_queue_timer=Timer();
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            llocal_get_queue_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            auto result = lqueue.front();
            lqueue.pop();
            llocal_get_queue_timer.pauseTime();
        }
        double llocal_get_queue_throughput=num_request/llocal_get_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        if (my_rank == 0) {
            printf("llocal_queue_throughput put: %f\n",llocal_queue_throughput);
            printf("llocal_queue_throughput get: %f\n",llocal_get_queue_throughput);
        }
        MPI_Barrier(client_comm);

        Timer local_queue_timer=Timer();
        uint16_t my_server_key = my_server % num_servers;
        /*Local queue test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            auto key=KeyType(val);
            local_queue_timer.resumeTime();
            queue->Push(key, my_server_key);
            local_queue_timer.pauseTime();
        }
        double local_queue_throughput=num_request/local_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        Timer local_get_queue_timer=Timer();
        /*Local queue test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            auto key=KeyType(val);
            local_get_queue_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            auto result = queue->Pop(my_server_key);
            local_get_queue_timer.pauseTime();
        }

        double local_get_queue_throughput=num_request/local_get_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        double local_put_tp_result, local_get_tp_result;
        if (client_comm_size > 1) {
            MPI_Reduce(&local_queue_throughput, &local_put_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            MPI_Reduce(&local_get_queue_throughput, &local_get_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            local_put_tp_result /= client_comm_size;
            local_get_tp_result /= client_comm_size;
        }
        else {
            local_put_tp_result = local_queue_throughput;
            local_get_tp_result = local_get_queue_throughput;
        }

        if (my_rank==0) {
            printf("local_queue_throughput put: %f\n", local_put_tp_result);
            printf("local_queue_throughput get: %f\n", local_get_tp_result);
        }

        MPI_Barrier(client_comm);

        Timer remote_queue_timer=Timer();
        /*Remote queue test*/
        uint16_t my_server_remote_key = (my_server + 1) % num_servers;
        for(int i=0;i<num_request;i++){
            size_t val = my_server+1;
            auto key=KeyType(val);
            remote_queue_timer.resumeTime();
            queue->Push(key, my_server_remote_key);
            remote_queue_timer.pauseTime();
        }
        double remote_queue_throughput=num_request/remote_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        MPI_Barrier(client_comm);

        Timer remote_get_queue_timer=Timer();
        /*Remote queue test*/
        for(int i=0;i<num_request;i++){
            size_t val = my_server+1;
            auto key=KeyType(val);
            remote_get_queue_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            queue->Pop(my_server_remote_key);
            remote_get_queue_timer.pauseTime();
        }
        double remote_get_queue_throughput=num_request/remote_get_queue_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        double remote_put_tp_result, remote_get_tp_result;
        if (client_comm_size > 1) {
            MPI_Reduce(&remote_queue_throughput, &remote_put_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            remote_put_tp_result /= client_comm_size;
            MPI_Reduce(&remote_get_queue_throughput, &remote_get_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            remote_get_tp_result /= client_comm_size;
        }
        else {
            remote_put_tp_result = remote_queue_throughput;
            remote_get_tp_result = remote_get_queue_throughput;
        }

        if(my_rank == 0) {
            printf("remote queue throughput (put): %f\n",remote_put_tp_result);
            printf("remote queue throughput (get): %f\n",remote_get_tp_result);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    delete(queue);
    MPI_Finalize();
    exit(EXIT_SUCCESS);
}
