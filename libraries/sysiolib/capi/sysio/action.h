/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup action_c Action C API
 * @ingroup c_api
 * @brief Defines API for querying action and sending action
 *
 *
 * A SYS.IO action has the following abstract structure:
 *
 * ```
 *   struct action {
 *     capi_name  account_name; // the contract defining the primary code to execute for code/type
 *     capi_name  action_name; // the action to be taken
 *     permission_level authorization; // the accounts and permission levels provided
 *     bytes data; // opaque data processed by code
 *   };
 * ```
 *
 * This API enables your contract to inspect the fields on the current action and act accordingly.
 *
 * Example:
 * @code
 * // Assume this action is used for the following examples:
 * // {
 * //  "code": "eos",
 * //  "type": "transfer",
 * //  "authorization": [{ "account": "inita", "permission": "active" }],
 * //  "data": {
 * //    "from": "inita",
 * //    "to": "initb",
 * //    "amount": 1000
 * //  }
 * // }
 *
 * char buffer[128];
 * uint32_t total = read_action(buffer, 5); // buffer contains the content of the action up to 5 bytes
 * print(total); // Output: 5
 *
 * uint32_t msgsize = action_size();
 * print(msgsize); // Output: size of the above action's data field
 *

 * require_recipient("initc"_n); // initc account will be notified for this action
 *
 * require_auth("inita"_n); // Do nothing since inita exists in the auth list
 * require_auth("initb"_n); // Throws an exception
 *
 * print(current_time()); // Output: timestamp (in microseconds since 1970) of current block
 *
 * @endcode
 *
 *
 * @{
 */

/**
 *  Copy up to length bytes of current action data to the specified location
 *
 *  @brief Copy current action data to the specified location
 *  @param msg - a pointer where up to length bytes of the current action data will be copied
 *  @param len - len of the current action data to be copied, 0 to report required size
 *  @return the number of bytes copied to msg, or number of bytes that can be copied if len==0 passed
 *  @pre `msg` is a valid pointer to a range of memory at least `len` bytes long
 *  @post `msg` is filled with packed action data
 */
__attribute__((sysio_wasm_import))
uint32_t read_action_data( void* msg, uint32_t len );

/**
 * Get the length of the current action's data field. This method is useful for dynamically sized actions
 *
 * @brief Get the length of current action's data field
 * @return the length of the current action's data field
 */
__attribute__((sysio_wasm_import))
uint32_t action_data_size( void );

/**
 *  Add the specified account to set of accounts to be notified
 *
 *  @brief Add the specified account to set of accounts to be notified
 *  @param name - name of the account to be verified
 */
__attribute__((sysio_wasm_import))
void require_recipient( capi_name name );

/**
 *  Verifies that name exists in the set of provided auths on a action. Throws if not found.
 *
 *  @brief Verify specified account exists in the set of provided auths
 *  @param name - name of the account to be verified
 */
__attribute__((sysio_wasm_import))
void require_auth( capi_name name );

 /**
 *  Verifies that name has auth.
 *
 *  @brief Verifies that name has auth.
 *  @param name - name of the account to be verified
 */
__attribute__((sysio_wasm_import))
bool has_auth( capi_name name );

/**
 *  Verifies that name exists in the set of provided auths on a action. Throws if not found.
 *
 *  @brief Verify specified account exists in the set of provided auths
 *  @param name - name of the account to be verified
 *  @param permission - permission level to be verified
 */
__attribute__((sysio_wasm_import))
void require_auth2( capi_name name, capi_name permission );

/**
 *  Verifies that @ref name is an existing account.
 *
 *  @brief Verifies that @ref name is an existing account.
 *  @param name - name of the account to check
 */
__attribute__((sysio_wasm_import))
bool is_account( capi_name name );

/**
 *  Send an inline action in the context of this action's parent transaction
 *
 *  @param serialized_action - serialized action
 *  @param size - size of serialized action in bytes
 *  @pre `serialized_action` is a valid pointer to an array at least `size` bytes long
 */
__attribute__((sysio_wasm_import))
void send_inline(char *serialized_action, size_t size);

/**
 * /function
 *  Send an inline context free action in the context of this action's parent transaction
 *
 *  @param serialized_action - serialized action
 *  @param size - size of serialized action in bytes
 *  @pre `serialized_action` is a valid pointer to an array at least `size` bytes long
 */
__attribute__((sysio_wasm_import))
void send_context_free_inline(char *serialized_action, size_t size);

/**
 *  Returns the time in microseconds from 1970 of the publication_time
 *  @brief Get the publication time
 *  @return the time in microseconds from 1970 of the publication_time
 */
__attribute__((sysio_wasm_import))
uint64_t  publication_time( void );

/**
 *  Get the current receiver of the action
 *  @brief Get the current receiver of the action
 *  @return the account which specifies the current receiver of the action
 */
__attribute__((sysio_wasm_import))
capi_name current_receiver( void );

/**
 *  Get the hash of the code currently published on the given account
 *  @param account Name of the account to hash the code of
 *  @param struct_version Version specifying the desired format of the returned struct
 *  @param result_buffer Buffer wherein the result should be written
 *  @param buffer_size Size in bytes of result_buffer
 */
 __attribute__((sysio_wasm_import))
 uint32_t get_code_hash( uint64_t account, uint32_t struct_version, char* result_buffer, size_t buffer_size );

/**
 * Set the action return value which will be included in the action_receipt
 * @brief Set the action return value
 * @param return_value - serialized return value
 * @param size - size of serialized return value in bytes
 * @pre `return_value` is a valid pointer to an array at least `size` bytes long
 */
__attribute__((sysio_wasm_import))
void set_action_return_value(void *return_value, size_t size);

#ifdef __cplusplus
}
#endif
/// @} action
