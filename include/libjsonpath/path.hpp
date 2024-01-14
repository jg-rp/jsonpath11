#ifndef LIBJSONPATH_PATH_H
#define LIBJSONPATH_PATH_H

#include <string_view>

#include "libjsonpath/node.hpp"
#include "libjsonpath/parse.hpp"
#include "nanobind/nanobind.h"

namespace nb = nanobind;

namespace libjsonpath {

using function_extension_map = std::unordered_map<std::string, nb::callable>;

// Apply the JSONPath query represented by _segments_ to JSON-like data _obj_.
JSONPathNodeList query_(const segments_t& segments, nb::object obj,
                        function_extension_map functions,
                        function_signature_map signatures, nb::object nothing);

// Parse the JSONPath query expression _path_ and use it to query JSON-like
// data in _obj_.
JSONPathNodeList query_(std::string_view path, nb::object obj,
                        function_extension_map functions,
                        function_signature_map signatures, nb::object nothing);

class Env_ {
private:
  function_extension_map m_functions{};
  function_signature_map m_signatures{};
  nb::object m_nothing{};
  Parser m_parser{};

public:
  Env_(function_extension_map functions, function_signature_map signatures,
       nb::object nothing)
      : m_functions{functions},
        m_signatures{signatures},
        m_nothing{nothing},
        m_parser{signatures} {}

  JSONPathNodeList query(std::string_view path, nb::object obj);
  JSONPathNodeList from_segments(const segments_t& segments, nb::object obj);
  segments_t parse(std::string_view path);
};

}  // namespace libjsonpath

#endif