#include "libjsonpath/node.hpp"

namespace nb = nanobind;

namespace libjsonpath {

using namespace std::string_literals;

struct LocationVisitor {
  std::string operator()(const size_t& index) const {
    return std::to_string(index);
  }

  std::string operator()(const std::string& name) const {
    return "'"s + name + "'"s;
  }
};

JSONPathNode::JSONPathNode(nb::object& value_, location_t location_)
    : value{value_}, location{location_} {}

std::string JSONPathNode::path() {
  LocationVisitor visitor{};
  auto rv = "$"s;
  for (auto item : location) {
    rv.push_back('[');
    rv.append(std::visit(visitor, item));
    rv.push_back(']');
  }
  return rv;
}

}  // namespace libjsonpath
