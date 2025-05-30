/*
 * Verifies that a class/function can be used from the std namespace
 */

#include <sysio/sysio.hpp>
#include <sysio/print.hpp>
#include <variant>

using std::variant;
using namespace sysio;

class[[sysio::contract("hello")]] hello : public contract
{
public:
   using contract::contract;

   [[sysio::action]] void hi(name user) {
      require_auth(user);
      print("Hello, ", user);
   }

   struct [[sysio::table]] greeting {
      uint64_t id;
      variant<int32_t, int64_t> t;
      uint64_t primary_key() const { return id; }
   };
   typedef multi_index<"greeting"_n, greeting> greeting_index;
};
