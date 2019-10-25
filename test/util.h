//
// Created by manih on 7/7/2019.
//

#ifndef HCL_UTIL_H
#define HCL_UTIL_H

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <mpi.h>
#include <boost/interprocess/containers/string.hpp>

namespace bip=boost::interprocess;

struct KeyType{
    size_t a;
    KeyType():a(0){}
    KeyType(size_t a_):a(a_){}
#if defined(HCL_ENABLE_RPCLIB)
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
    bool Contains(const KeyType &o) const {
        return a==o.a;
    }

#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
  template<typename A>
    void serialize(A& ar) const {
        ar & a;
    }
#endif
};

// 1,4,16,1000,4000,16000,250000,1000000,4000000,16000000

struct MappedType{
    // char a[62500];
    // double a[62500];
    // std::array<double, 62500> a;
    std::string a;

    MappedType() {
        a = "";
        a.resize(1000000, 'c');
        // for (long i = 0; i < 62500; i++) {
        //     a[i] = 1.1;
        // }
    }

    MappedType(std::string a_) {
        a = a_;
        // for (long i = 0; i < 62500; i++) {
        //     a[i] = a_[i];
        // }
    }

#if defined(HCL_ENABLE_RPCLIB)
    MSGPACK_DEFINE(a);
#endif
    /* equal operator for comparing two Matrix. */
    bool operator==(const MappedType &o) const {
        if (a == o.a) {
            return false;
        }
        // for (long i = 0; i < 62500; i++) {
            // if (a[i] != o.a[i]) {
            //     return false;
            // }
        // }
        return true;
    }
    MappedType& operator=( const MappedType& other ) {
        a = other.a;
        // for (long i = 0; i < 62500; i++) {
        //     a[i] = other.a[i];
        // }
        return *this;
    }
    // bool operator<(const MappedType &o) const {
    //     for (long i = 0; i < 62500; i++) {
    //         if (a[i] < o.a[i]) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }
    // bool Contains(const MappedType &o) const {
    //     for (long i = 0; i < 62500; i++) {
    //         if (a[i] != o.a[i]) {
    //             return false;
    //         }
    //     }
    //     return true;
    // }
#if defined(HCL_ENABLE_THALLIUM_TCP) || defined(HCL_ENABLE_THALLIUM_ROCE)
  template<typename A>
  void serialize(A& ar) const {
      ar & a;
      // for (long i = 0; i < 62500; i++) {
      //     ar & a[i];
      // }
  }
#endif
};

const int MAX = 26;
std::string printRandomString(int n)
{
    char alphabet[MAX] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
                           'h', 'i', 'j', 'k', 'l', 'm', 'n',
                           'o', 'p', 'q', 'r', 's', 't', 'u',
                           'v', 'w', 'x', 'y', 'z' };

    std::string res = "";
    for (int i = 0; i < n; i++)
        res = res + alphabet[rand() % MAX];

    return res;
}

namespace std {
    template<>
    struct hash<KeyType> {
        int operator()(const KeyType &k) const {
            return k.a;
        }
    };
}

void bt_sighandler(int sig, struct sigcontext ctx) {

    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;

    if (sig == SIGSEGV)
        printf("Got signal %d, faulty address is %p, "
               "from %p\n", sig, ctx.cr2, ctx.rip);
    else
        printf("Got signal %d\n", sig);

    trace_size = backtrace(trace, 16);
    /* overwrite sigaction with caller's address */
    trace[1] = (void *)ctx.rip;
    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("[bt] Execution path:\n");
    for (i=1; i<trace_size; ++i)
    {
        printf("[bt] #%d %s\n", i, messages[i]);

        /* find first occurence of '(' or ' ' in message[i] and assume
         * everything before that is the file name. (Don't go beyond 0 though
         * (string terminator)*/
        size_t p = 0;
        while(messages[i][p] != '(' && messages[i][p] != ' '
              && messages[i][p] != 0)
            ++p;

        char syscom[256];
        sprintf(syscom,"addr2line %p -e %.*s", trace[i], p, messages[i]);
        //last parameter is the file name of the symbol
        system(syscom);
    }

    exit(0);
}

void SetSignal(){
    struct sigaction sa;

    sa.sa_handler = reinterpret_cast<__sighandler_t>(bt_sighandler);
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

struct MpiData {
    int comm_size;
    int rank;
    std::string processor_name;
};

MpiData initMpiData(int *argc, char*** argv) {

    MpiData result = {};

    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "Didn't receive appropriate MPI threading specification\n");
        exit(EXIT_FAILURE);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &result.comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &result.rank);

    int len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &len);
    result.processor_name = std::string(processor_name);

    return result;
}
#endif //HCL_UTIL_H
