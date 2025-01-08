#include <sysio/sysio.hpp>

using namespace sysio;

template<uint64_t Tag>
struct TaggedNumber {
    uint64_t value;
};

class [[sysio::contract]] tagged_number_test : public contract {
  public:
      using contract::contract;
      
      [[sysio::action]]
      void test(TaggedNumber<"a.tag"_n.value>) {
      }
};
