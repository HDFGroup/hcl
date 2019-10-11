//
// Created by hariharan on 8/16/19.
//

#ifndef BASKET_RPC_FACTORY_H
#define BASKET_RPC_FACTORY_H

#include <unordered_map>
#include <memory>
#include "rpc_lib.h"

class RPCFactory{
private:
    std::unordered_map<uint16_t,std::shared_ptr<RPC>> rpcs;
public:
    RPCFactory():rpcs(){}
    std::shared_ptr<RPC> GetRPC(uint16_t server_port){
        auto iter = rpcs.find(server_port);
        if(iter!=rpcs.end()) return iter->second;
        auto temp = BASKET_CONF->RPC_PORT;
        BASKET_CONF->RPC_PORT=server_port;
        auto rpc = std::make_shared<RPC>();
        rpcs.emplace(server_port,rpc);
        BASKET_CONF->RPC_PORT=temp;
        return rpc;
    }
};
#endif //BASKET_RPC_FACTORY_H
