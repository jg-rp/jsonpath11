#include "libjsonpath/path.hpp"

#include <algorithm>      // std::min std::max
#include <cmath>          // std::abs
#include <cstdint>        // std::int64_t
#include <limits>         // std::numeric_limits
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map
#include <variant>        // std::variant std::visit

#include "libjsonpath/exceptions.hpp"
#include "libjsonpath/jsonpath.hpp"
#include "libjsonpath/node.hpp"
#include "libjsonpath/selectors.hpp"
#include "nanobind/nanobind.h"

namespace nb = nanobind;

namespace libjsonpath {

using namespace std::string_literals;
using expression_rv = std::variant<JSONPathNodeList, nb::object>;

// Convert negative indicies to their positive equivalents given
// an "array" length.
size_t normalized_index(size_t length, std::int64_t index, const Token& token) {
  if (index >= 0) {
    return static_cast<size_t>(index);
  }

  if (length > static_cast<size_t>(std::numeric_limits<std::int64_t>::max())) {
    throw IndexError("array index out of range", token);
  }

  std::int64_t positive_index = length + index;
  return (positive_index >= 0) ? static_cast<size_t>(positive_index) : length;
}

// JSONPath expression result truthiness test.
bool is_truthy(const expression_rv& rv) {
  if (std::holds_alternative<JSONPathNodeList>(rv)) {
    auto nodes = std::get<JSONPathNodeList>(rv);
    return !nodes.empty();
  }

  auto value{std::get<nb::object>(rv)};
  return !(nb::isinstance<nb::bool_>(value) && !nb::cast<nb::bool_>(value));
}

// Visit every object with _node.value_ at the root.
void descend(const JSONPathNode& node, std::vector<JSONPathNode>& out_nodes) {
  out_nodes.push_back(node);
  if (nb::isinstance<nb::dict>(node.value)) {
    auto obj{nb::cast<nb::dict>(node.value)};
    for (auto item : obj) {
      nb::str key{item.first};
      nb::object val = nb::cast<nb::object>(item.second);
      location_t location{node.location};
      location.push_back(nb::cast<std::string>(item.first));
      descend({val, location}, out_nodes);
    }
  } else if (nb::isinstance<nb::list>(node.value)) {
    auto obj{nb::cast<nb::list>(node.value)};
    size_t index{0};
    for (auto item : obj) {
      nb::object val = nb::cast<nb::object>(item);
      location_t location{node.location};
      location.push_back(index);
      descend({val, location}, out_nodes);
      index++;
    }
  }
}

// Return a list of values from a node list, or a single value if
// the node list only has one item.
nb::object values_or_singular(const JSONPathNodeList& nodes) {
  if (nodes.size() == 1) {
    return nodes[0].value;
  }

  nb::list values{};
  for (auto node : nodes) {
    values.append(node.value);
  }

  return values;
}

class QueryContext {
public:
  QueryContext(nb::object root_, const function_extension_map& functions_,
               const function_signature_map& signatures_, nb::object nothing_);

  const nb::object root;
  const function_extension_map& functions;
  const function_signature_map& signatures;
  const nb::object nothing;
};

QueryContext::QueryContext(nb::object root_,
                           const function_extension_map& functions_,
                           const function_signature_map& signatures_,
                           nb::object nothing_)
    : root{root_},
      functions{functions_},
      signatures{signatures_},
      nothing{nothing_} {}

// Contextual objects a JSONPath filter will operate on.
struct FilterContext {
  const QueryContext& query;
  nb::object current;
};

class ExpressionVisitor {
private:
  const FilterContext& m_context;

public:
  ExpressionVisitor(const FilterContext& filter_context)
      : m_context{filter_context} {}

  ~ExpressionVisitor() = default;

  expression_rv operator()(const NullLiteral&) const { return nb::none(); }

  expression_rv operator()(const BooleanLiteral& expression) const {
    return nb::bool_(expression.value);
  }

  expression_rv operator()(const IntegerLiteral& expression) const {
    return nb::int_(expression.value);
  }

  expression_rv operator()(const FloatLiteral& expression) const {
    return nb::float_(expression.value);
  }

  expression_rv operator()(const StringLiteral& expression) const {
    return nb::str(expression.value.c_str());
  }

  expression_rv operator()(const Box<LogicalNotExpression>& expression) const {
    return nb::bool_(!is_truthy(std::visit(*this, expression->right)));
  }

  expression_rv operator()(const Box<InfixExpression>& expression) const {
    // Unpack single value node list.
    expression_rv left{std::visit(*this, expression->left)};
    if (std::holds_alternative<JSONPathNodeList>(left)) {
      auto left_ = std::get<JSONPathNodeList>(left);
      if (left_.size() == 1) {
        left = left_[0].value;
      }
    }

    // Unpack single value node list.
    expression_rv right{std::visit(*this, expression->right)};
    if (std::holds_alternative<JSONPathNodeList>(right)) {
      auto right_ = std::get<JSONPathNodeList>(right);
      if (right_.size() == 1) {
        right = right_[0].value;
      }
    }

    if (expression->op == BinaryOperator::logical_and) {
      return nb::bool_(is_truthy(left) && is_truthy(right));
    }

    if (expression->op == BinaryOperator::logical_or) {
      return nb::bool_(is_truthy(left) || is_truthy(right));
    }

    return nb::bool_(compare(left, expression->op, right));
  }

  expression_rv operator()(const Box<RelativeQuery>& expression) const {
    return query_(expression->query, m_context.current,
                  m_context.query.functions, m_context.query.signatures,
                  m_context.query.nothing);
  }

  expression_rv operator()(const Box<RootQuery>& expression) const {
    return query_(expression->query, m_context.query.root,
                  m_context.query.functions, m_context.query.signatures,
                  m_context.query.nothing);
  }

  expression_rv operator()(const Box<FunctionCall>& expression) const {
    auto name{std::string{expression->name}};
    auto it{m_context.query.functions.find(name)};
    if (it == m_context.query.functions.end()) {
      throw NameError(
          "undefined filter function '"s + std::string(expression->name) + "'"s,
          expression->token);
    }

    nb::callable func = it->second;

    auto sig_it{m_context.query.signatures.find(name)};
    if (sig_it == m_context.query.signatures.end()) {
      throw NameError("missing types for filter function '"s +
                          std::string(expression->name) + "'"s,
                      expression->token);
    }
    FunctionExtensionTypes func_sig = sig_it->second;

    nb::list args{};
    size_t index = 0;

    for (auto arg : expression->args) {
      expression_rv arg_rv{std::visit(*this, arg)};
      if (std::holds_alternative<JSONPathNodeList>(arg_rv)) {
        auto nodes{std::get<JSONPathNodeList>(arg_rv)};
        // Is the parameter expected a node list of values?
        // Assumes the function call has already been validated and has
        // the correct number of arguments.
        if (func_sig.args[index] != ExpressionType::nodes) {
          if (nodes.empty()) {
            args.append(m_context.query.nothing);
          } else if (nodes.size() == 1) {
            args.append(nodes[0].value);
          } else {
            args.append(nb::cast(nodes));
          }
        } else {
          args.append(nb::cast(nodes));
        }
      } else {
        args.append(std::get<nb::object>(arg_rv));
      }

      index++;
    }

    auto rv{func(*args)};
    if (func_sig.res == ExpressionType::nodes) {
      // TODO: catch exception.
      return nb::cast<JSONPathNodeList>(rv);
    }
    return rv;
  }

private:
  bool compare(const expression_rv& left, BinaryOperator op,
               const expression_rv& right) const {
    switch (op) {
      case BinaryOperator::eq:
        return equals(left, right);
      case BinaryOperator::ne:
        return !equals(left, right);
      case BinaryOperator::lt:
        return less_than(left, right);
      case BinaryOperator::gt:
        return less_than(right, left);
      case BinaryOperator::ge:
        return less_than(right, left) || equals(left, right);
      case BinaryOperator::le:
        return less_than(left, right) || equals(left, right);
      default:
        return false;
    }
  }

  bool equals(const expression_rv& left_, const expression_rv& right_) const {
    if (std::holds_alternative<JSONPathNodeList>(left_)) {
      return node_list_equals(std::get<JSONPathNodeList>(left_), right_);
    }

    if (std::holds_alternative<JSONPathNodeList>(right_)) {
      return node_list_equals(std::get<JSONPathNodeList>(right_), left_);
    }

    // Both left and right are py objects.
    nb::object left{std::get<nb::object>(left_)};
    nb::object right{std::get<nb::object>(right_)};
    return left.equal(right);
  }

  bool node_list_equals(const JSONPathNodeList& left,
                        const expression_rv& right_) const {
    if (std::holds_alternative<nb::object>(right_)) {
      nb::object right{std::get<nb::object>(right_)};

      // left is an empty node list and right is NOTHING.
      if (left.empty()) {
        return right.equal(m_context.query.nothing);
      }

      // left is a single element node list, compare the node's value to
      // right.
      if (left.size() == 1) {
        return left[0].value.equal(right);
      }

      return false;
    }

    // left and right are node lists.
    JSONPathNodeList right{std::get<JSONPathNodeList>(right_)};

    // Are both lists are empty?
    if (left.empty() && right.empty()) {
      return true;
    }

    // Do both lists have a single node?
    if (left.size() == 1 && right.size() == 1) {
      return left[0].value.equal(right[0].value);
    }

    return false;
  }

  bool less_than(const expression_rv& left_,
                 const expression_rv& right_) const {
    if (std::holds_alternative<JSONPathNodeList>(left_) ||
        std::holds_alternative<JSONPathNodeList>(right_)) {
      return false;
    }

    nb::object left{std::get<nb::object>(left_)};
    nb::object right{std::get<nb::object>(right_)};

    if (nb::isinstance<nb::bool_>(left) || nb::isinstance<nb::bool_>(right)) {
      return false;
    }

    if (nb::isinstance<nb::str>(left) && nb::isinstance<nb::str>(right)) {
      return left < right;
    }

    if (nb::isinstance<nb::int_>(left) && nb::isinstance<nb::int_>(right)) {
      return left < right;
    }

    if (nb::isinstance<nb::int_>(left) && nb::isinstance<nb::float_>(right)) {
      return left < right;
    }

    if (nb::isinstance<nb::float_>(left) && nb::isinstance<nb::float_>(right)) {
      return left < right;
    }

    if (nb::isinstance<nb::float_>(left) && nb::isinstance<nb::int_>(right)) {
      return left < right;
    }

    return false;
  }
};

class SelectorVisitor {
private:
  const QueryContext& m_query_context;
  const JSONPathNode& m_node;
  std::vector<JSONPathNode>* m_out_nodes;

public:
  SelectorVisitor(const QueryContext& q_ctx, const JSONPathNode& node,
                  std::vector<JSONPathNode>* out_nodes)
      : m_query_context{q_ctx}, m_node{node}, m_out_nodes{out_nodes} {
    nb::print(".. selector visitor");
  }

  ~SelectorVisitor() = default;

  void operator()(const NameSelector& selector) {
    if (nb::isinstance<nb::dict>(m_node.value)) {
      auto obj{nb::cast<nb::dict>(m_node.value)};
      nb::str name{selector.name.c_str()};
      if (obj.contains(name)) {
        nb::object val{obj[name]};
        location_t location{m_node.location};
        location.push_back(selector.name);
        m_out_nodes->push_back(JSONPathNode{val, location});
      }
    }
  }

  void operator()(const IndexSelector& selector) {
    if (nb::isinstance<nb::list>(m_node.value)) {
      auto obj{nb::cast<nb::list>(m_node.value)};
      size_t len{nb::len(obj)};
      auto index{normalized_index(len, selector.index, selector.token)};
      if (index < len) {
        nb::object val{obj[index]};
        location_t location{m_node.location};
        location.push_back(index);
        m_out_nodes->push_back(JSONPathNode{val, location});
      }
    }
  }

  void operator()(const WildSelector&) {
    if (nb::isinstance<nb::dict>(m_node.value)) {
      nb::print(".. a dict");
      auto obj{nb::cast<nb::dict>(m_node.value)};
      for (auto item : obj) {
        nb::str key{item.first};
        nb::object val = nb::cast<nb::object>(item.second);
        location_t location{m_node.location};
        location.push_back(nb::cast<std::string>(item.first));
        nb::print(".. push to location");
        m_out_nodes->push_back(JSONPathNode{val, location});
      }
    } else if (nb::isinstance<nb::list>(m_node.value)) {
      auto obj{nb::cast<nb::list>(m_node.value)};
      size_t index{0};
      for (auto item : obj) {
        nb::object val = nb::cast<nb::object>(item);
        location_t location{m_node.location};
        location.push_back(index);
        m_out_nodes->push_back(JSONPathNode{val, location});
        index++;
      }
    }
  }

  void operator()(const SliceSelector& selector) {
    if (nb::isinstance<nb::list>(m_node.value)) {
      auto obj{nb::cast<nb::list>(m_node.value)};
      for (auto i : slice_indicies(selector, obj.size())) {
        auto item{obj[i]};
        nb::object val = nb::cast<nb::object>(item);
        auto norm_index{normalized_index(nb::len(obj), i, selector.token)};
        location_t location{m_node.location};
        location.push_back(norm_index);
        m_out_nodes->push_back(JSONPathNode{val, location});
      }
    }
  }

  void operator()(const Box<FilterSelector>& selector) {
    if (nb::isinstance<nb::dict>(m_node.value)) {
      auto obj{nb::cast<nb::dict>(m_node.value)};
      for (auto item : obj) {
        nb::object val = nb::cast<nb::object>(item.second);
        FilterContext filter_context{m_query_context, val};
        ExpressionVisitor visitor{filter_context};

        if (is_truthy(std::visit(visitor, selector->expression))) {
          location_t location{m_node.location};
          location.push_back(nb::cast<std::string>(item.first));
          m_out_nodes->push_back({val, location});
        }
      }
    } else if (nb::isinstance<nb::list>(m_node.value)) {
      auto obj{nb::cast<nb::list>(m_node.value)};
      size_t index{0};
      for (auto item : obj) {
        nb::object val = nb::cast<nb::object>(item);
        FilterContext filter_context{m_query_context, val};
        ExpressionVisitor visitor{filter_context};

        if (is_truthy(std::visit(visitor, selector->expression))) {
          location_t location{m_node.location};
          location.push_back(index);
          m_out_nodes->push_back({val, location});
        }

        index++;
      }
    }
  }

private:
  std::vector<int64_t> slice_indicies(const SliceSelector& selector,
                                      size_t size) {
    if (!size) {
      return {};
    }

    std::int64_t length = static_cast<std::int64_t>(size);
    std::int64_t start{0};
    std::int64_t stop{length};
    std::int64_t step{selector.step.value_or(1)};

    if (step == 0) {
      return {};
    }

    // Handle negative start values.
    if (!selector.start) {
      start = step < 0 ? length - 1 : 0UL;
    } else if (selector.start.value() < 0) {
      start = std::max(length + selector.start.value(), std::int64_t{0});
    } else {
      start = std::min(selector.start.value(), length - 1);
    }

    // Handle negative stop values
    if (!selector.stop) {
      stop = step < 0 ? -1 : size;
    } else if (selector.stop.value() < 0) {
      stop = std::max(length + selector.stop.value(), std::int64_t{-1});
    } else {
      stop = std::min(selector.stop.value(), length);
    }

    // TODO: return start, stp and step
    // TODO: then loop in the caller
    std::vector<std::int64_t> indicies{};
    if (step > 0) {
      for (int64_t i = start; i < stop; i += step) {
        indicies.push_back(i);
      }
    } else {
      for (int64_t i = start; i > stop; i += step) {
        indicies.push_back(i);
      }
    }
    return indicies;
  }
};

class SegmentVisitor {
private:
  const QueryContext& m_context;
  const std::vector<JSONPathNode>& m_nodes;
  std::vector<JSONPathNode>* m_out_nodes;

public:
  SegmentVisitor(const QueryContext& q_ctx,
                 const std::vector<JSONPathNode>& nodes,
                 std::vector<JSONPathNode>* out_nodes)
      : m_context{q_ctx}, m_nodes{nodes}, m_out_nodes{out_nodes} {
    nb::print(".. segment visitor constructor");
  }

  ~SegmentVisitor() = default;

  void operator()(const Segment& segment) {
    for (auto node : m_nodes) {
      SelectorVisitor visitor{m_context, node, m_out_nodes};
      for (auto selector : segment.selectors) {
        std::visit(visitor, selector);
      }
    }
  }

  void operator()(const RecursiveSegment& segment) {
    for (auto node : m_nodes) {
      std::vector<JSONPathNode> descendants{};
      descend(node, descendants);
      for (auto descendant : descendants) {
        SelectorVisitor visitor{m_context, descendant, m_out_nodes};
        for (auto selector : segment.selectors) {
          std::visit(visitor, selector);
        }
      }
    }
  }
};

JSONPathNodeList resolve_segment(
    const QueryContext& q_ctx, const JSONPathNodeList& nodes,
    const std::variant<libjsonpath::Segment, libjsonpath::RecursiveSegment>&
        segment) {
  nb::print(".. resolve segment");
  JSONPathNodeList out_nodes{};
  SegmentVisitor visitor{q_ctx, nodes, &out_nodes};
  std::visit(visitor, segment);
  return out_nodes;
}

// TODO: Don't pass context around, make all these functions methods of a
// class.

JSONPathNodeList query_(const segments_t& segments, nb::object obj,
                        function_extension_map functions,
                        function_signature_map signatures, nb::object nothing) {
  nb::print(".. make query context");
  QueryContext q_ctx{obj, functions, signatures, nothing};
  // Bootstrap the node list with root object and an empty location.
  nb::print(".. bootstrap nodes");
  JSONPathNodeList nodes{{obj, {}}};
  for (auto segment : segments) {
    nodes = resolve_segment(q_ctx, nodes, segment);
  }
  return nodes;
}

JSONPathNodeList query_(std::string_view path, nb::object obj,
                        function_extension_map functions,
                        function_signature_map signatures, nb::object nothing) {
  segments_t segments{parse(path, signatures)};
  QueryContext q_ctx{obj, functions, signatures, nothing};
  // Bootstrap the node list with root object and an empty location.
  JSONPathNodeList nodes{{obj, {}}};
  for (auto segment : segments) {
    nodes = resolve_segment(q_ctx, nodes, segment);
  }
  return nodes;
}

JSONPathNodeList Env_::query(std::string_view path, nb::object obj) {
  segments_t segments{m_parser.parse(path)};
  QueryContext q_ctx{obj, m_functions, m_signatures, m_nothing};
  // Bootstrap the node list with root object and an empty location.
  JSONPathNodeList nodes{{obj, {}}};
  for (auto segment : segments) {
    nodes = resolve_segment(q_ctx, nodes, segment);
  }
  return nodes;
}

JSONPathNodeList Env_::from_segments(const segments_t& segments,
                                     nb::object obj) {
  QueryContext q_ctx{obj, m_functions, m_signatures, m_nothing};
  // Bootstrap the node list with root object and an empty location.
  JSONPathNodeList nodes{{obj, {}}};
  for (auto segment : segments) {
    nodes = resolve_segment(q_ctx, nodes, segment);
  }
  return nodes;
}

segments_t Env_::parse(std::string_view path) { return m_parser.parse(path); }

}  // namespace libjsonpath