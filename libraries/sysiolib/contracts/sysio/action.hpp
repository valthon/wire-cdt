/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <cstdlib>
#include <type_traits>

#include "../../core/sysio/serialize.hpp"
#include "../../core/sysio/datastream.hpp"
#include "../../core/sysio/name.hpp"
#include "../../core/sysio/fixed_bytes.hpp"
#include "../../core/sysio/ignore.hpp"
#include "../../core/sysio/time.hpp"

namespace sysio {

   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((sysio_wasm_import))
         uint32_t read_action_data( void* msg, uint32_t len );

         __attribute__((sysio_wasm_import))
         uint32_t action_data_size();

         __attribute__((sysio_wasm_import))
         void require_recipient( uint64_t name );

         __attribute__((sysio_wasm_import))
         void require_auth( uint64_t name );

         __attribute__((sysio_wasm_import))
         bool has_auth( uint64_t name );

         __attribute__((sysio_wasm_import))
         void require_auth2( uint64_t name, uint64_t permission );

         __attribute__((sysio_wasm_import))
         bool is_account( uint64_t name );

         __attribute__((sysio_wasm_import))
         void send_inline(char *serialized_action, size_t size);

         __attribute__((sysio_wasm_import))
         void send_context_free_inline(char *serialized_action, size_t size);

         __attribute__((sysio_wasm_import))
         uint64_t  publication_time();

         __attribute__((sysio_wasm_import))
         uint64_t current_receiver();

         __attribute__((sysio_wasm_import))
         uint32_t get_code_hash( uint64_t account, uint32_t struct_version, char* result_buffer, size_t buffer_size );
      }
   };

   struct code_hash_result {
       unsigned_int struct_version;
       uint64_t code_sequence;
       checksum256 code_hash;
       uint8_t vm_type;
       uint8_t vm_version;

       CDT_REFLECT(struct_version, code_sequence, code_hash, vm_type, vm_version);
       SYSLIB_SERIALIZE(code_hash_result, (struct_version)(code_sequence)(code_hash)(vm_type)(vm_version));
   };

   /**
    *  @defgroup action Action
    *  @ingroup contracts
    *  @brief Defines type-safe C++ wrappers for querying action and sending action
    *  @note There are some methods from the @ref action that can be used directly from C++
    */

   /**
    *  @ingroup action
    *  @return Unpacked action data casted as T.
    *
    *  Example:
    *
    *  @code
    *  struct dummy_action {
    *    char a; //1
    *    unsigned long long b; //8
    *    int  c; //4
    *
    *    SYSLIB_SERIALIZE( dummy_action, (a)(b)(c) )
    *  };
    *  dummy_action msg = unpack_action_data<dummy_action>();
    *  @endcode
    */
   template<typename T>
   T unpack_action_data() {
      constexpr size_t max_stack_buffer_size = 512;
      size_t size = internal_use_do_not_use::action_data_size();
      char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
      internal_use_do_not_use::read_action_data( buffer, size );
      return unpack<T>( buffer, size );
   }

   /**
    *  Add the specified account to set of accounts to be notified
    *
    *  @ingroup action
    *  @brief Add the specified account to set of accounts to be notified
    *  @param notify_account - name of the account to be verified
    */
   inline void require_recipient( name notify_account ){
      internal_use_do_not_use::require_recipient( notify_account.value );
   }

   /**
    *  All of the listed accounts will be added to the set of accounts to be notified
    *
    *  This helper method enables you to add multiple accounts to accounts to be notified list with a single
    *  call rather than having to call the similar C API multiple times.
    *
    *  @ingroup action
    *  @param notify_account account to be notified
    *  @param remaining_accounts accounts to be notified
    *  @note action.code is also considered as part of the set of notified accounts
    *
    *  Example:
    *
    *  @code
    *  require_recipient("Account1"_n, "Account2"_n, "Account3"_n); // throws exception if any of them not in set.
    *  @endcode
    */
   template<typename... accounts>
   void require_recipient( name notify_account, accounts... remaining_accounts ){
      internal_use_do_not_use::require_recipient( notify_account.value );
      require_recipient( remaining_accounts... );
   }

   /**
    *  Verifies that @ref name exists in the set of provided auths on a action. Fails if not found.
    *
    *  @ingroup action
    *  @param name - name of the account to be verified
    */
   inline void require_auth( name n ) {
      internal_use_do_not_use::require_auth( n.value );
   }

   /**
   *  Returns the time in microseconds from 1970 of the publication_time
   *
   *  @ingroup action
   *  @return the time in microseconds from 1970 of the publication_time
   */
   inline time_point  publication_time() {
     return time_point( microseconds ( internal_use_do_not_use::publication_time() ) );
   }

   /**
   *  Get the current receiver of the action
   *  @return the account which specifies the current receiver of the action
   */
   inline name current_receiver() {
     return name{internal_use_do_not_use::current_receiver()};
   }

   /**
   *  Get the hash of the code currently published on the given account
   *  @param account Name of the account to hash the code of
   *  @param full_result Optional: If a full result struct is desired, a pointer to the struct to populate
   *  @return The SHA256 hash of the specified account's code
   */
   inline checksum256 get_code_hash( name account, code_hash_result* full_result = nullptr ) {
       if (full_result == nullptr)
           full_result = (code_hash_result*)alloca(sizeof(code_hash_result));
       constexpr size_t max_stack_buffer_size = 50;

       // Packed size of this struct will virtually always be less than the struct size; always less after padding
       auto struct_buffer_size = sizeof(code_hash_result);
       char* struct_buffer = (char*)alloca(struct_buffer_size);

       using VersionType = decltype(code_hash_result::struct_version);
       const VersionType STRUCT_VERSION = 0;
       auto response_size =
           internal_use_do_not_use::get_code_hash(account.value, STRUCT_VERSION, struct_buffer, struct_buffer_size);
       // Safety check: in this case, response size should never exceed our buffer, but just in case...
       bool buffer_on_heap = false;
       if (response_size > struct_buffer_size) {
           // Slow path: allocate an adequate buffer and try again
           // No need to deallocate struct_buffer since it was alloca'd
           if (response_size > max_stack_buffer_size) {
               struct_buffer = (char*)malloc(response_size);
               buffer_on_heap = true;
           } else {
               struct_buffer = (char*)alloca(response_size);
           }
           internal_use_do_not_use::get_code_hash(account.value, STRUCT_VERSION, struct_buffer, struct_buffer_size);
       }

       check(unpack<VersionType>(struct_buffer, struct_buffer_size) == STRUCT_VERSION,
             "Hypervisor returned unexpected code hash struct version");
       unpack(*full_result, struct_buffer, struct_buffer_size);

       // If struct_buffer is heap allocated, we must free it
       if (buffer_on_heap)
           free(struct_buffer);

       return full_result->code_hash;
   }

   /**
    *  Copy up to length bytes of current action data to the specified location
    *
    *  @ingroup action
    *  @param msg - a pointer where up to length bytes of the current action data will be copied
    *  @param len - len of the current action data to be copied, 0 to report required size
    *  @return the number of bytes copied to msg, or number of bytes that can be copied if len==0 passed
    *  @pre `msg` is a valid pointer to a range of memory at least `len` bytes long
    *  @post `msg` is filled with packed action data
    */
   inline uint32_t read_action_data( void* msg, uint32_t len ) {
     return internal_use_do_not_use::read_action_data(msg, len);
   }

   /**
    * Get the length of the current action's data field. This method is useful for dynamically sized actions
    *
    * @return the length of the current action's data field
    */
   inline uint32_t action_data_size() {
     return internal_use_do_not_use::action_data_size();
   }
   /**
    * Packed representation of a permission level (Authorization)
    *
    * @ingroup action
    */
   struct permission_level {
      /**
       * Construct a new permission level object with actor name and permission name
       *
       * @param a - Name of the account who owns this authorization
       * @param p - Name of the permission
       */
      permission_level( name a, name p ):actor(a),permission(p){}

      /**
       * Default Constructor
       *
       */
      permission_level(){}

      /**
       * Name of the account who owns this permission
       */
      name    actor;
      /**
       * Name of the permission
       */
      name    permission;

      /**
       * Check equality of two permissions
       *
       * @param a - first permission to compare
       * @param b - second permission to compare
       * @return true if equal
       * @return false if unequal
       */
      friend constexpr bool operator == ( const permission_level& a, const permission_level& b ) {
         return std::tie( a.actor, a.permission ) == std::tie( b.actor, b.permission );
      }

      /**
       * Lexicographically compares two permissions
       *
       * @param a - first permission to compare
       * @param b - second permission to compare
       * @return true if a < b
       * @return false if a >= b
       */
      friend constexpr bool operator < ( const permission_level& a, const permission_level& b ) {
         return std::tie( a.actor, a.permission ) < std::tie( b.actor, b.permission );
      }

      SYSLIB_SERIALIZE( permission_level, (actor)(permission) )
   };

   /**
    *  Require the specified authorization for this action. If this action doesn't contain the specified auth, it will fail.
    *
    *  @ingroup action
    *  @param level - Authorization to be required
    */
   inline void require_auth( const permission_level& level ) {
      internal_use_do_not_use::require_auth2( level.actor.value, level.permission.value );
   }

   /**
    *  Verifies that @ref n has auth.
    *
    *  @ingroup action
    *  @param n - name of the account to be verified
    */
   inline bool has_auth( name n ) {
      return internal_use_do_not_use::has_auth( n.value );
   }

   /**
    *  Verifies that @ref n is an existing account.
    *
    *  @ingroup action
    *  @param n - name of the account to check
    */
   inline bool is_account( name n ) {
      return internal_use_do_not_use::is_account( n.value );
   }

   /**
    *  This is the packed representation of an action along with
    *  meta-data about the authorization levels.
    *
    *  @ingroup action
    */
   struct action {
      /**
       *  Name of the account the action is intended for
       */
      name                       account;

      /**
       *  Name of the action
       */
      name                       name;

      /**
       *  List of permissions that authorize this action
       */
      std::vector<permission_level>   authorization;

      /**
       *  Payload data
       */
      std::vector<char>               data;

      /**
       *  Default Constructor
       */
      action() = default;

      /**
       * Construct a new action object with the given permission, action receiver, action name, action struct
       *
       * @tparam T  - Type of action struct, must be serializable by `pack(...)`
       * @param auth - The permissions that authorizes this action
       * @param a -  The name of the account this action is intended for (action receiver)
       * @param n - The name of the action
       * @param value - The action struct that will be serialized via pack into data
       */
      template<typename T>
      action( const permission_level& auth, struct name a, struct name n, T&& value )
      :account(a), name(n), authorization(1,auth), data(pack(std::forward<T>(value))) {}

      /**
       * Construct a new action object with the given list of permissions, action receiver, action name, action struct
       *
       * @tparam T  - Type of action struct, must be serializable by `pack(...)`
       * @param auths - The list of permissions that authorize this action
       * @param a -  The name of the account this action is intended for (action receiver)
       * @param n - The name of the action
       * @param value - The action struct that will be serialized via pack into data
       */
      template<typename T>
      action( std::vector<permission_level> auths, struct name a, struct name n, T&& value )
      :account(a), name(n), authorization(std::move(auths)), data(pack(std::forward<T>(value))) {}

      /// @cond INTERNAL

      SYSLIB_SERIALIZE( action, (account)(name)(authorization)(data) )

      /// @endcond

      /**
       * Send the action as inline action
       */
      void send() const {
         auto serialize = pack(*this);
         internal_use_do_not_use::send_inline(serialize.data(), serialize.size());
      }

      /**
       * Send the action as inline context free action
       *
       * @pre This action should not contain any authorizations
       */
      void send_context_free() const {
         sysio::check( authorization.size() == 0, "context free actions cannot have authorizations");
         auto serialize = pack(*this);
         internal_use_do_not_use::send_context_free_inline(serialize.data(), serialize.size());
      }

      /**
       * Retrieve the unpacked data as T
       *
       * @tparam T expected type of data
       * @return the action data
       */
      template<typename T>
      T data_as() {
         return unpack<T>( &data[0], data.size() );
      }

   };



   namespace detail {

      /// @cond INTERNAL

      template <typename T>
      struct unwrap { typedef T type; };

      template <typename T>
      struct unwrap<ignore<T>> { typedef T type; };

      template <typename R, typename Act, typename... Args>
      auto get_args(R(Act::*p)(Args...)) {
         return std::tuple<std::decay_t<typename unwrap<Args>::type>...>{};
      }

      template <typename R, typename Act, typename... Args>
      auto get_args_nounwrap(R(Act::*p)(Args...)) {
         return std::tuple<std::decay_t<Args>...>{};
      }

      template <auto Action>
      using deduced = decltype(get_args(Action));

      template <auto Action>
      using deduced_nounwrap = decltype(get_args_nounwrap(Action));

      template <typename T>
      struct convert { typedef T type; };

      template <>
      struct convert<const char*> { typedef std::string type; };

      template <>
      struct convert<char*> { typedef std::string type; };

      template <typename T, typename U>
      struct is_same { static constexpr bool value = std::is_convertible<T,U>::value; };

      template <typename U>
      struct is_same<bool,U> { static constexpr bool value = std::is_integral<U>::value; };

      template <typename T>
      struct is_same<T,bool> { static constexpr bool value = std::is_integral<T>::value; };

      template <size_t N, size_t I, auto Arg, auto... Args>
      struct get_nth_impl { static constexpr auto value  = get_nth_impl<N,I+1,Args...>::value; };

      template <size_t N, auto Arg, auto... Args>
      struct get_nth_impl<N, N, Arg, Args...> { static constexpr auto value = Arg; };

      template <size_t N, auto... Args>
      struct get_nth { static constexpr auto value  = get_nth_impl<N,0,Args...>::value; };

      template <auto Action, size_t I, typename T, typename... Rest>
      struct check_types {
         static_assert(detail::is_same<typename convert<T>::type, typename convert<typename std::tuple_element<I, deduced<Action>>::type>::type>::value);
         using type = check_types<Action, I+1, Rest...>;
         static constexpr bool value = true;
      };
      template <auto Action, size_t I, typename T>
      struct check_types<Action, I, T> {
         static_assert(detail::is_same<typename convert<T>::type, typename convert<typename std::tuple_element<I, deduced<Action>>::type>::type>::value);
         static constexpr bool value = true;
      };

      template <auto Action, typename... Ts>
      constexpr bool type_check() {
         static_assert(sizeof...(Ts) == std::tuple_size<deduced<Action>>::value);
         if constexpr (sizeof...(Ts) != 0)
            return check_types<Action, 0, Ts...>::value;
         return true;
      }

      /// @endcond
   }

   /**
    * Wrapper for an action object.
    *
    * @brief Used to wrap an a particular action to simplify the process of other contracts sending inline actions to "wrapped" action.
    * Example:
    * @code
    * // defined by contract writer of the actions
    * using transfer_act = action_wrapper<"transfer"_n, &token::transfer>;
    * // usage by different contract writer
    * transfer_act{"sysio.token"_n, {st.issuer, "active"_n}}.send(st.issuer, to, quantity, memo);
    * // or
    * transfer_act trans_action{ "sysio.token"_n, {st.issuer, "active"_n}};
    * trans_action.send(st.issuer, to, quantity, memo);
    * @endcode
    */
   template <sysio::name::raw Name, auto Action>
   struct action_wrapper {
      template <typename Code>
      constexpr action_wrapper(Code&& code, std::vector<sysio::permission_level>&& perms)
         : code_name(std::forward<Code>(code)), permissions(std::move(perms)) {}

      template <typename Code>
      constexpr action_wrapper(Code&& code, const std::vector<sysio::permission_level>& perms)
         : code_name(std::forward<Code>(code)), permissions(perms) {}

      template <typename Code>
      constexpr action_wrapper(Code&& code, sysio::permission_level&& perm)
         : code_name(std::forward<Code>(code)), permissions({1, std::move(perm)}) {}

      template <typename Code>
      constexpr action_wrapper(Code&& code, const sysio::permission_level& perm)
         : code_name(std::forward<Code>(code)), permissions({1, perm}) {}

      template <typename Code>
      constexpr action_wrapper(Code&& code)
         : code_name(std::forward<Code>(code)) {}

      static constexpr sysio::name action_name = sysio::name(Name);
      sysio::name code_name;
      std::vector<sysio::permission_level> permissions;

      static constexpr auto get_mem_ptr() {
         return Action;
      }

      template <typename... Args>
      action to_action(Args&&... args)const {
         static_assert(detail::type_check<Action, Args...>());
         return action(permissions, code_name, action_name, detail::deduced<Action>{std::forward<Args>(args)...});
      }
      template <typename... Args>
      void send(Args&&... args)const {
         to_action(std::forward<Args>(args)...).send();
      }

      template <typename... Args>
      void send_context_free(Args&&... args)const {
         to_action(std::forward<Args>(args)...).send_context_free();
      }

   };

   template <sysio::name::raw Name, auto... Actions>
   struct variant_action_wrapper {
      template <typename Code>
      constexpr variant_action_wrapper(Code&& code, std::vector<sysio::permission_level>&& perms)
         : code_name(std::forward<Code>(code)), permissions(std::move(perms)) {}

      template <typename Code>
      constexpr variant_action_wrapper(Code&& code, const std::vector<sysio::permission_level>& perms)
         : code_name(std::forward<Code>(code)), permissions(perms) {}

      template <typename Code>
      constexpr variant_action_wrapper(Code&& code, sysio::permission_level&& perm)
         : code_name(std::forward<Code>(code)), permissions({1, std::move(perm)}) {}

      template <typename Code>
      constexpr variant_action_wrapper(Code&& code, const sysio::permission_level& perm)
         : code_name(std::forward<Code>(code)), permissions({1, perm}) {}

      static constexpr sysio::name action_name = sysio::name(Name);
      sysio::name code_name;
      std::vector<sysio::permission_level> permissions;

      template <size_t Variant>
      static constexpr auto get_mem_ptr() {
         return detail::get_nth<Variant, Actions...>::value;
      }

      template <size_t Variant, typename... Args>
      action to_action(Args&&... args)const {
         static_assert(detail::type_check<detail::get_nth<Variant, Actions...>::value, Args...>());
         unsigned_int var = Variant;
         return action(permissions, code_name, action_name, std::tuple_cat(std::make_tuple(var), detail::deduced<detail::get_nth<Variant, Actions...>::value>{std::forward<Args>(args)...}));
      }


      template <size_t Variant, typename... Args>
      void send(Args&&... args)const {
         to_action<Variant>(std::forward<Args>(args)...).send();
      }

      template <size_t Variant, typename... Args>
      void send_context_free(Args&&... args) const {
         to_action<Variant>(std::forward<Args>(args)...).send_context_free();
      }

   };

   template<typename... Args>
   void dispatch_inline( name code, name act,
                         std::vector<permission_level> perms,
                         std::tuple<Args...> args ) {
      action( perms, code, act, std::move(args) ).send();
   }

   template<typename, name::raw>
   struct inline_dispatcher;


   template<typename T, name::raw Name, typename... Args>
   struct inline_dispatcher<void(T::*)(Args...), Name> {
      static void call(name code, const permission_level& perm, std::tuple<Args...> args) {
         dispatch_inline(code, name(Name), std::vector<permission_level>(1, perm), std::move(args));
      }
      static void call(name code, std::vector<permission_level> perms, std::tuple<Args...> args) {
         dispatch_inline(code, name(Name), std::move(perms), std::move(args));
      }
   };

} // namespace sysio

#define INLINE_ACTION_SENDER3( CONTRACT_CLASS, FUNCTION_NAME, ACTION_NAME  )\
::sysio::inline_dispatcher<decltype(&CONTRACT_CLASS::FUNCTION_NAME), ACTION_NAME>::call

#define INLINE_ACTION_SENDER2( CONTRACT_CLASS, NAME )\
INLINE_ACTION_SENDER3( CONTRACT_CLASS, NAME, ::sysio::name(#NAME) )

#define INLINE_ACTION_SENDER(...) BLUEGRASS_META_OVERLOAD(INLINE_ACTION_SENDER,__VA_ARGS__)(__VA_ARGS__)

/**
 * Send an inline-action from inside a contract.
 *
 * @brief A macro to simplify calling inline actions
 * @details The send inline action macro is intended to simplify the process of calling inline actions. When calling new actions from existing actions
 * SYSIO supports two communication models, inline and deferred. Inline actions are executed as part of the current transaction. This macro
 * creates an @ref action using the supplied parameters and automatically calls action.send() on this newly created action.
 *
 * Example:
 * @code
 * SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
 * @endcode
 *
 * The example above is taken from sysio.token.
 * This example:
 *       uses the passed in, dereferenced `this` pointer, to call this.get_self() i.e. the sysio.token contract;
 *       calls the sysio.token::transfer() action;
 *       uses the active permission of the "issuer" account;
 *       uses parameters st.issuer, to, quantity and memo.
 * This macro creates an action struct used to 'send()' (call) transfer(account_name from, account_name to, asset quantity, string memo)
 *
 * @param CONTRACT - The contract to call, which contains the action being sent, maps to the @ref account
 * @param NAME - The name of the action to be called, maps to a @ref name
 * @param ... - The authorising permission, maps to an @ref authorization , followed by the parameters of the action, maps to a @ref data.
 */

#define SEND_INLINE_ACTION( CONTRACT, NAME, ... )\
INLINE_ACTION_SENDER(std::decay_t<decltype(CONTRACT)>, NAME)( (CONTRACT).get_self(),__VA_ARGS__)
