#ifndef LIBJSONPATH_NODE_H
#define LIBJSONPATH_NODE_H

#include <string>
#include <variant>
#include <vector>

#include "nanobind/nanobind.h"

namespace nb = nanobind;

namespace libjsonpath {

using location_t = std::vector<std::variant<size_t, std::string>>;

// A JSON-like object and its location within a JSON document.
class JSONPathNode {
public:
  nb::object value;
  location_t location;

  JSONPathNode(nb::object &value_, location_t location_);

  // Return the canonical string representation of the path to this node.
  std::string path();
};

using JSONPathNodeList = std::vector<JSONPathNode>;

}  // namespace libjsonpath

#endif