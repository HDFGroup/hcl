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

/*-------------------------------------------------------------------------
 *
 * Created: debug.h
 * June 5 2018
 * Hariharan Devarajan <hdevarajan@hdfgroup.org>
 *
 * Purpose: Defines debug macros for Basket.
 *
 *-------------------------------------------------------------------------
 */

#ifndef INCLUDE_BASKET_COMMON_DEBUG_H_
#define INCLUDE_BASKET_COMMON_DEBUG_H_

#include <unistd.h>
#include <execinfo.h>
#include <iostream>
#include <csignal>
#include <tuple>


/**
 * Handles signals and prints stack trace.
 *
 * @param sig
 */
inline void handler(int sig) {
    void *array[10];
    size_t size;
    // get void*'s for all entries on the stack
    size = backtrace(array, 300);
    int rank, comm_size;
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(0);
}
/**
 * various macros to print variables and messages.
 */

#ifdef BASKET_DEBUG
#define DBGVAR(var)                                             \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
    << #var << " = [" << (var) << "]" << std::endl

#define DBGVAR2(var1, var2)                                     \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
    << #var1 << " = [" << (var1) << "]"                         \
    << #var2 << " = [" << (var2) << "]"  << std::endl
#define DBGVAR3(var1, var2, var3)                               \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
    << #var1 << " = [" << (var1) << "]"                         \
    << #var2 << " = [" << (var2) << "]"                         \
    << #var3 << " = [" << (var3) << "]"  << std::endl

#define DBGMSG(msg)                                             \
    std::cout << "DBG: " << __FILE__ << "(" << __LINE__ << ") " \
    << msg << std::endl
#else
#define DBGVAR(var)
#define DBGVAR2(var1, var2)
#define DBGVAR3(var1, var2, var3)
#define DBGMSG(msg)
#endif

/**
 * Time all functions and instrument it
 */

#include <stdarg.h>
#include <mpi.h>
#include <stack>
#include <string>
#include <chrono>
#include <sstream>

class Timer {
  public:
    Timer():elapsed_time(0) {}
    void resumeTime() {
        t1 = std::chrono::high_resolution_clock::now();
    }
    double pauseTime() {
        auto t2 = std::chrono::high_resolution_clock::now();
        elapsed_time +=
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    t2 - t1).count()/1000000.0;
        return elapsed_time;
    }
    double getElapsedTime() {
        return elapsed_time;
    }
  private:
    std::chrono::high_resolution_clock::time_point t1;
    double elapsed_time;
};
/**
 * Implement Auto tracing Mechanism.
 */
using std::cout;
using std::endl;
using std::string;

using namespace std;
class AutoTrace
{
#if  defined(BASKET_TIMER)
    Timer timer;
#endif
    static int rank,item;
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
    string m_line;
#endif
  public:
    template <typename... Args>
    AutoTrace(
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
            std::string string,
#endif
            Args... args)
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
    :m_line(string)
#endif
    {
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
        char thread_name[256];
        pthread_getname_np(pthread_self(), thread_name,256);
        std::stringstream stream;

        if(rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        stream << "\033[31m";
        stream <<++item<<";"<<thread_name<<";"<< rank << ";" <<m_line << ";";
#endif
#if  defined(BASKET_TIMER)
        stream <<";;";
#endif
#ifdef BASKET_TRACE
        auto args_obj = std::make_tuple(args...);
        const ulong args_size = std::tuple_size<decltype(args_obj)>::value;
        stream << "args(";

        if(args_size == 0) stream << "Void";
        else {
            std::apply([&stream](auto&&... args) {((stream << args << ", "), ...);}, args_obj);
        }
        stream << ");";
#endif
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
        stream <<"start"<< endl;
        stream << "\033[00m";
        cout << stream.str();
#endif
#ifdef BASKET_TIMER
        timer.resumeTime();
#endif
    }

    ~AutoTrace()
    {
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
        std::stringstream stream;
        char thread_name[256];
        pthread_getname_np(pthread_self(), thread_name,256);
        stream << "\033[31m";
        stream <<item-- <<";"<<std::string(thread_name)<<";"<< rank << ";" << m_line << ";";
#endif
#if defined(BASKET_TRACE)
        stream  <<";";
#endif
#ifdef BASKET_TIMER
        double end_time=timer.pauseTime();
        stream  <<end_time<<";msecs;";
#endif
#if defined(BASKET_TRACE) || defined(BASKET_TIMER)
        stream  <<"finish"<< endl;
        stream << "\033[00m";
        cout << stream.str();
#endif
    }
};



#endif  // INCLUDE_BASKET_COMMON_DEBUG_H_
