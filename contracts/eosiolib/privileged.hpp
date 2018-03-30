#pragma once
#include "privileged.h"
#include "serialize.hpp"
#include "types.h"

namespace eosio {

   struct blockchain_parameters {
      uint32_t base_per_transaction_net_usage;
      uint32_t base_per_transaction_cpu_usage;
      uint32_t base_per_action_cpu_usage;
      uint32_t base_setcode_cpu_usage;
      uint32_t per_signature_cpu_usage;
      uint32_t max_transaction_lifetime;
      uint32_t max_transaction_exec_time;
      uint16_t max_authority_depth;
      uint16_t max_inline_depth;
      uint32_t max_inline_action_size;
      uint32_t max_generated_transaction_size;
      uint32_t max_generated_transaction_count;

      EOSLIB_SERIALIZE( blockchain_parameters,
                        (base_per_transaction_net_usage)(base_per_transaction_cpu_usage)(base_per_action_cpu_usage)
                        (base_setcode_cpu_usage)(per_signature_cpu_usage)
                        (max_transaction_lifetime)(max_transaction_exec_time)(max_authority_depth)
                        (max_inline_depth)(max_inline_action_size)(max_generated_transaction_size)
                        (max_generated_transaction_count)
      )
   };

   void set_blockchain_parameters(const eosio::blockchain_parameters& params);

   void get_blockchain_parameters(eosio::blockchain_parameters& params);

   struct producer_key {
      account_name producer_name;
      public_key   block_signing_key;

      EOSLIB_SERIALIZE( producer_key, (producer_name)(block_signing_key) )
   };

   struct producer_schedule {
      uint32_t             version = 0; ///< sequentially incrementing version number
      std::vector<producer_key> producers;

      EOSLIB_SERIALIZE( producer_schedule, (version)(producers) )
   };

}
