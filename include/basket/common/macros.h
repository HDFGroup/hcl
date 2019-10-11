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
#ifndef INCLUDE_BASKET_COMMON_MACROS_H_
#define INCLUDE_BASKET_COMMON_MACROS_H_

#include <basket/common/configuration_manager.h>
#include <basket/common/singleton.h>
# define EXPAND_ARGS(...) __VA_ARGS__
#define BASKET_CONF basket::Singleton<basket::ConfigurationManager>::GetInstance()

#define THALLIUM_DEFINE(name, args,args_t...) void Thallium##name(const tl::request &thallium_req, args_t) { thallium_req.respond(name args ); }

#define THALLIUM_DEFINE1(name) void Thallium##name(const tl::request &thallium_req) { thallium_req.respond(name()); }

#ifdef BASKET_ENABLE_RPCLIB
#define RPC_CALL_WRAPPER_RPCLIB1(funcname, serverVar,ret) \
 case RPCLIB: {								\
    return rpc->call<RPCLIB_MSGPACK::object_handle>( serverVar , func_prefix + std::string(funcname) ).template as< ret >(); \
    break;\
  }
#define RPC_CALL_WRAPPER_RPCLIB(funcname, serverVar,ret,args...)			\
 case RPCLIB: {								\
  return rpc->call<RPCLIB_MSGPACK::object_handle>( serverVar , func_prefix + std::string(funcname) ,args).template as< ret >(); \
    break;\
  }
#else
#define RPC_CALL_WRAPPER_RPCLIB1(funcname, serverVar,ret)
#define RPC_CALL_WRAPPER_RPCLIB(funcname, serverVar,ret,args...) 
#endif
#ifdef BASKET_ENABLE_RPCLIB
#define RPC_CALL_WRAPPER_RPCLIB1_CB(funcname, serverVar,ret) \
 case RPCLIB: {								\
    return rpc->call<RPCLIB_MSGPACK::object_handle>( serverVar , funcname , std::forward< CB_Args >( cb_args )...).template as< ret >();\
    break;\
  }
#define RPC_CALL_WRAPPER_RPCLIB_CB(funcname, serverVar,ret, ...)			\
 case RPCLIB: {								\
  return rpc->call<RPCLIB_MSGPACK::object_handle>( serverVar , funcname , __VA_ARGS__ , std::forward< CB_Args >( cb_args )...).template as< ret >();\
    break;\
  }
#else
#define RPC_CALL_WRAPPER_RPCLIB1_CB(funcname, serverVar,ret)
#define RPC_CALL_WRAPPER_RPCLIB_CB(funcname, serverVar,ret,args...)
#endif

#ifdef  BASKET_ENABLE_THALLIUM_TCP
#define RPC_CALL_WRAPPER_THALLIUM_TCP() case THALLIUM_TCP:
#else
#define RPC_CALL_WRAPPER_THALLIUM_TCP() 
#endif
#ifdef BASKET_ENABLE_THALLIUM_ROCE
#define RPC_CALL_WRAPPER_THALLIUM_ROCE() case THALLIUM_ROCE:
#else
#define RPC_CALL_WRAPPER_THALLIUM_ROCE() 
#endif
#if defined(BASKET_ENABLE_THALLIUM_TCP) || defined(BASKET_ENABLE_THALLIUM_ROCE)
#define RPC_CALL_WRAPPER_THALLIUM1(funcname, serverVar,ret)\
{\
 return rpc->call<tl::packed_response>( serverVar , func_prefix + funcname ).template as< ret >(); \
 break;\
 }
#define RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar,ret,args...)	\
{\
 return rpc->call<tl::packed_response>( serverVar , func_prefix + funcname ,args ).template as< ret >(); \
 break;\
 }
#else
#define RPC_CALL_WRAPPER_THALLIUM1(funcname, serverVar,ret)
#define RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar,ret,args...) 
#endif


#define RPC_CALL_WRAPPER1(funcname, serverVar,ret) [& ]()-> ret { \
switch (BASKET_CONF->RPC_IMPLEMENTATION) {\
RPC_CALL_WRAPPER_RPCLIB1(funcname, serverVar,ret) \
RPC_CALL_WRAPPER_THALLIUM_TCP()\
RPC_CALL_WRAPPER_THALLIUM_ROCE()\
RPC_CALL_WRAPPER_THALLIUM1(funcname, serverVar,ret)\
 }\
}();
#define RPC_CALL_WRAPPER(funcname, serverVar,ret, args...) [& ]()-> ret { \
switch (BASKET_CONF->RPC_IMPLEMENTATION) {\
  RPC_CALL_WRAPPER_RPCLIB(funcname, serverVar,ret,args)	\
RPC_CALL_WRAPPER_THALLIUM_TCP()\
RPC_CALL_WRAPPER_THALLIUM_ROCE()\
    RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar,ret,args)	\
}\
  }();
#define RPC_CALL_WRAPPER1_CB(funcname, serverVar,ret) [&]()-> ret { \
switch (BASKET_CONF->RPC_IMPLEMENTATION) {\
RPC_CALL_WRAPPER_RPCLIB1(funcname, serverVar,ret) \
RPC_CALL_WRAPPER_THALLIUM_TCP()\
RPC_CALL_WRAPPER_THALLIUM_ROCE()\
RPC_CALL_WRAPPER_THALLIUM1(funcname, serverVar,ret)\
 }\
}();

#define RPC_CALL_WRAPPER_CB(funcname, serverVar,ret, ...) [&]()-> ret { \
switch (BASKET_CONF->RPC_IMPLEMENTATION) {\
  RPC_CALL_WRAPPER_RPCLIB_CB(funcname, serverVar,ret, __VA_ARGS__)	\
RPC_CALL_WRAPPER_THALLIUM_TCP()\
RPC_CALL_WRAPPER_THALLIUM_ROCE()\
    RPC_CALL_WRAPPER_THALLIUM(funcname, serverVar,ret,__VA_ARGS__)	\
}\
  }();

#endif  // INCLUDE_BASKET_COMMON_MACROS_H_
