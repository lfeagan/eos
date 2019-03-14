/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#include "fork_test_utilities.hpp"

using namespace eosio::chain;
using namespace eosio::testing;

BOOST_AUTO_TEST_SUITE(protocol_feature_tests)

BOOST_AUTO_TEST_CASE( activate_preactivate_feature ) try {
   tester c( setup_policy::none );
   const auto& pfm = c.control->get_protocol_feature_manager();

   c.produce_block();

   // Cannot set latest bios contract since it requires intrinsics that have not yet been whitelisted.
   BOOST_CHECK_EXCEPTION( c.set_code( config::system_account_name, contracts::eosio_bios_wasm() ),
                          wasm_exception, fc_exception_message_is("env.is_feature_activated unresolveable")
   );

   // But the old bios contract can still be set.
   c.set_code( config::system_account_name, contracts::before_preactivate_eosio_bios_wasm() );
   c.set_abi( config::system_account_name, contracts::before_preactivate_eosio_bios_abi().data() );

   auto t = c.control->pending_block_time();
   c.control->abort_block();
   BOOST_REQUIRE_EXCEPTION( c.control->start_block( t, 0, {digest_type()} ), protocol_feature_exception,
                            fc_exception_message_is( "protocol feature with digest '0000000000000000000000000000000000000000000000000000000000000000' is unrecognized" )
   );

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::preactivate_feature );

   BOOST_REQUIRE( d );

   // Activate PREACTIVATE_FEATURE.
   c.schedule_protocol_features_wo_preactivation({ *d });
   c.produce_block();

   // Now the latest bios contract can be set.
   c.set_code( config::system_account_name, contracts::eosio_bios_wasm() );
   c.set_abi( config::system_account_name, contracts::eosio_bios_abi().data() );

   c.produce_block();

   BOOST_CHECK_EXCEPTION( c.push_action( config::system_account_name, N(reqactivated), config::system_account_name,
                                          mutable_variant_object()("feature_digest",  digest_type()) ),
                           eosio_assert_message_exception,
                           eosio_assert_message_is( "protocol feature is not activated" )
   );

   c.push_action( config::system_account_name, N(reqactivated), config::system_account_name, mutable_variant_object()
      ("feature_digest",  *d )
   );

   c.produce_block();

   // Ensure validator node accepts the blockchain

   tester c2(setup_policy::none, db_read_mode::SPECULATIVE);
   push_blocks( c, c2 );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( double_preactivation ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   c.push_action( config::system_account_name, N(preactivate), config::system_account_name,
                  fc::mutable_variant_object()("feature_digest", *d), 10 );

   std::string expected_error_msg("protocol feature with digest '");
   {
      fc::variant v;
      to_variant( *d, v );
      expected_error_msg += v.get_string();
      expected_error_msg += "' is already pre-activated";
   }

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(preactivate), config::system_account_name,
                                          fc::mutable_variant_object()("feature_digest", *d), 20 ),
                           protocol_feature_exception,
                           fc_exception_message_is( expected_error_msg )
   );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( double_activation ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.preactivate_protocol_features( {*d} );

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.schedule_protocol_features_wo_preactivation( {*d} );

   BOOST_CHECK_EXCEPTION(  c.produce_block();,
                           block_validate_exception,
                           fc_exception_message_starts_with( "attempted duplicate activation within a single block:" )
   );

   c.protocol_features_to_be_activated_wo_preactivation.clear();

   BOOST_CHECK( !c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.produce_block();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

   c.produce_block();

   BOOST_CHECK( c.control->is_builtin_activated( builtin_protocol_feature_t::only_link_to_existing_permission ) );

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_CASE( only_link_to_existing_permission_test ) try {
   tester c( setup_policy::preactivate_feature_and_new_bios );
   const auto& pfm = c.control->get_protocol_feature_manager();

   auto d = pfm.get_builtin_digest( builtin_protocol_feature_t::only_link_to_existing_permission );
   BOOST_REQUIRE( d );

   c.create_accounts( {N(alice), N(bob), N(charlie)} );

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(bob), fc::mutable_variant_object()
                              ("account", "bob")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(charlie), fc::mutable_variant_object()
                              ("account", "charlie")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

   c.push_action( config::system_account_name, N(updateauth), N(alice), fc::mutable_variant_object()
      ("account", "alice")
      ("permission", "test")
      ("parent", "active")
      ("auth", authority(get_public_key("testapi", "test")))
   );

   c.produce_block();

   // Verify the incorrect behavior prior to ONLY_LINK_TO_EXISTING_PERMISSION activation.
   c.push_action( config::system_account_name, N(linkauth), N(bob), fc::mutable_variant_object()
      ("account", "bob")
      ("code", name(config::system_account_name))
      ("type", "")
      ("requirement", "test" )
   );

   c.preactivate_protocol_features( {*d} );
   c.produce_block();

   // Verify the correct behavior after ONLY_LINK_TO_EXISTING_PERMISSION activation.
   BOOST_CHECK_EXCEPTION(  c.push_action( config::system_account_name, N(linkauth), N(charlie), fc::mutable_variant_object()
                              ("account", "charlie")
                              ("code", name(config::system_account_name))
                              ("type", "")
                              ("requirement", "test" )
                           ), permission_query_exception,
                           fc_exception_message_is( "Failed to retrieve permission: test" )
   );

} FC_LOG_AND_RETHROW()


BOOST_AUTO_TEST_SUITE_END()
