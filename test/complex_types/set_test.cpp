/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <utility>
#include <mpi.h>
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <chrono>
#include <set>
#include <hcl/common/data_structures.h>
#include <hcl/set/set.h>

const int KEY_SIZE=1000;

struct KeyType {
    std::array<int,KEY_SIZE> a;

    KeyType() : a() {}

    KeyType(std::array<int,KEY_SIZE> a_) : a(a_) {}
    KeyType(int val) {
        for(int i=0;i<a.size();++i){
            a[i]=val;
        }
    }
#ifdef HCL_ENABLE_RPCLIB
    MSGPACK_DEFINE (a);
#endif

    /* equal operator for comparing two Matrix. */
    bool operator==(const KeyType &o) const {
        if(o.a.size() != a.size()) return false;
        for(int i=0;i<a.size();++i){
            if(o.a[i] != a[i]) return false;
        }
        return true;
    }

    KeyType &operator=(const KeyType &other) {
        a = other.a;
        return *this;
    }

    bool operator<(const KeyType &o) const {
        if(o.a.size() < a.size()) return false;
        if(o.a.size() > a.size()) return true;
        for(int i=0;i<a.size();++i){
            if(o.a[i] < a[i]) return false;
            if(o.a[i] > a[i]) return true;
        }
        return false;
    }

    bool operator>(const KeyType &o) const {
        return !(a < o.a);
    }
    bool operator>=(const KeyType &o) const {
        return (a > o.a) || (a == o.a);
    }
    bool operator<=(const KeyType &o) const {
        return (a < o.a) || (a == o.a);
    }
    bool Contains(const KeyType &o) const {
        return a == o.a;
    }

};

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
template<typename A>
void serialize(A &ar, KeyType &a) {
    ar & a.a;
}
#endif

struct ValueType {
    std::vector<int> a;

    ValueType() : a() {}

    ValueType(std::vector<int> a_) : a(a_) {}
    ValueType(size_t num_elements, int val):a(num_elements,val) {
    }

#ifdef HCL_ENABLE_RPCLIB
    MSGPACK_DEFINE (a);
#endif

    /* equal operator for comparing two Matrix. */
    bool operator==(const ValueType &o) const {
        if(o.a.size() != a.size()) return false;
        for(int i=0;i<a.size();++i){
            if(o.a[i] != a[i]) return false;
        }
        return true;
    }

    ValueType &operator=(const ValueType &other) {
        a = other.a;
        return *this;
    }

    bool operator<(const ValueType &o) const {
        if(o.a.size() < a.size()) return false;
        if(o.a.size() > a.size()) return true;
        for(int i=0;i<a.size();++i){
            if(o.a[i] < a[i]) return false;
            if(o.a[i] > a[i]) return true;
        }
        return false;
    }

    bool operator>(const ValueType &o) const {
        return !(a < o.a);
    }

    bool Contains(const ValueType &o) const {
        return a == o.a;
    }

    size_t size(){
        return a.size();
    }

};

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
template<typename A>
void serialize(A &ar, ValueType &a) {
    ar & a.a;
}
#endif
namespace std {
    template<>
    struct hash<KeyType> {
        size_t operator()(const KeyType &k) const {
            size_t hash_val = hash<int>()(k.a[0]);
            for(int i=1;i<k.a.size();++i){
                hash_val ^= hash<int>()(k.a[0]);
            }
            return hash_val;
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
    int ranks_per_server=comm_size,num_request=100;
    long size_of_request=1000;
    bool debug=false;
    bool server_on_node=false;
    std::string server_lists = "./simple_types/server_list";
    if (argc > 1) ranks_per_server = atoi(argv[1]);
    if (argc > 2) num_request = atoi(argv[2]);
    if (argc > 3) size_of_request = (long) atol(argv[3]);
    if (argc > 4) server_on_node = (bool) atoi(argv[4]);
    if (argc > 5) debug = (bool) atoi(argv[5]);
    if (argc > 6) server_lists = argv[6];

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

    typedef int SimpleType;
    typedef std::vector<SimpleType> MyType;
    auto num_elements = size_of_request / sizeof(SimpleType);
    auto my_vals = MyType(num_elements, my_rank);
    size_t size_of_elem = sizeof(SimpleType);


    HCL_CONF->IS_SERVER = is_server;
    HCL_CONF->MY_SERVER = my_server;
    HCL_CONF->NUM_SERVERS = num_servers;
    HCL_CONF->SERVER_ON_NODE = server_on_node || is_server;
    HCL_CONF->SERVER_LIST_PATH = server_lists;

    hcl::set<KeyType> *set;
    if (is_server) {
        set = new hcl::set<KeyType>();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        set = new hcl::set<KeyType>();
    }

    std::set<KeyType> lset=std::set<KeyType>();

    MPI_Comm client_comm;
    MPI_Comm_split(MPI_COMM_WORLD, !is_server, my_rank, &client_comm);
    int client_comm_size;
    MPI_Comm_size(client_comm, &client_comm_size);
    // if(is_server){
    //     std::function<int(int)> func=[](int x){ std::cout<<x<<std::endl;return x; };
    //     int a;
    //     std::function<std::pair<bool,int>(KeyType&,std::array<int, array_size>&,std::string,int)> putFunc(std::bind(&hcl::set<KeyType,std::array<int,
    //                                                                                                                 array_size>>::LocalPutWithCallback<int,int>,set,std::placeholders::_1, std::placeholders::_2,std::placeholders::_3, std::placeholders::_4));
    //     set->Bind("CB_Put", func, "APut",putFunc);
    // }
    MPI_Barrier(MPI_COMM_WORLD);
    if (!is_server) {
        Timer llocal_set_timer=Timer();
        std::hash<KeyType> keyHash;
        /*Local std::set test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            llocal_set_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            lset.insert(KeyType(val));
            llocal_set_timer.pauseTime();
        }

        double llocal_set_throughput=num_request/llocal_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        Timer llocal_get_set_timer=Timer();
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            llocal_get_set_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            auto result = lset.find(KeyType(val));
            if (result != lset.end()){}
            llocal_get_set_timer.pauseTime();
        }
        double llocal_get_set_throughput=num_request/llocal_get_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        if (my_rank == 0) {
            printf("llocal_set_throughput put: %f\n",llocal_set_throughput);
            printf("llocal_set_throughput get: %f\n",llocal_get_set_throughput);
        }
        MPI_Barrier(client_comm);

        Timer local_set_timer=Timer();
        uint16_t my_server_key = my_server % num_servers;
        /*Local set test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            auto key=KeyType(val);
            local_set_timer.resumeTime();
            set->Put(key);
            local_set_timer.pauseTime();
        }
        double local_set_throughput=num_request/local_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        Timer local_get_set_timer=Timer();
        /*Local set test*/
        for(int i=0;i<num_request;i++){
            size_t val=my_server;
            auto key=KeyType(val);
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            local_get_set_timer.resumeTime();
            auto result = set->Get(key);
            local_get_set_timer.pauseTime();
        }

        double local_get_set_throughput=num_request/local_get_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        double local_put_tp_result, local_get_tp_result;
        if (client_comm_size > 1) {
            MPI_Reduce(&local_set_throughput, &local_put_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            MPI_Reduce(&local_get_set_throughput, &local_get_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            local_put_tp_result /= client_comm_size;
            local_get_tp_result /= client_comm_size;
        }
        else {
            local_put_tp_result = local_set_throughput;
            local_get_tp_result = local_get_set_throughput;
        }

        if (my_rank==0) {
            printf("local_set_throughput put: %f\n", local_put_tp_result);
            printf("local_set_throughput get: %f\n", local_get_tp_result);
        }

        MPI_Barrier(client_comm);

        Timer remote_set_timer=Timer();
        /*Remote set test*/
        uint16_t my_server_remote_key = (my_server + 1) % num_servers;
        for(int i=0;i<num_request;i++){
            size_t val = my_server+1;
            auto key=KeyType(val);
            remote_set_timer.resumeTime();
            set->Put(key);
            remote_set_timer.pauseTime();
        }
        double remote_set_throughput=num_request/remote_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        MPI_Barrier(client_comm);

        Timer remote_get_set_timer=Timer();
        /*Remote set test*/
        for(int i=0;i<num_request;i++){
            size_t val = my_server+1;
            auto key=KeyType(val);
            remote_get_set_timer.resumeTime();
            size_t key_hash = keyHash(KeyType(val))%num_servers;
            if (key_hash == my_server && is_server){}
            set->Get(key);
            remote_get_set_timer.pauseTime();
        }
        double remote_get_set_throughput=num_request/remote_get_set_timer.getElapsedTime()*1000*size_of_elem*my_vals.size()/1024/1024;

        double remote_put_tp_result, remote_get_tp_result;
        if (client_comm_size > 1) {
            MPI_Reduce(&remote_set_throughput, &remote_put_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            remote_put_tp_result /= client_comm_size;
            MPI_Reduce(&remote_get_set_throughput, &remote_get_tp_result, 1,
                       MPI_DOUBLE, MPI_SUM, 0, client_comm);
            remote_get_tp_result /= client_comm_size;
        }
        else {
            remote_put_tp_result = remote_set_throughput;
            remote_get_tp_result = remote_get_set_throughput;
        }

        if(my_rank == 0) {
            printf("remote set throughput (put): %f\n",remote_put_tp_result);
            printf("remote set throughput (get): %f\n",remote_get_tp_result);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    delete(set);
    MPI_Finalize();
    exit(EXIT_SUCCESS);
}
