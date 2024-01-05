#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "libjsonpath/exceptions.hpp"
#include "libjsonpath/jsonpath.hpp"
#include "libjsonpath/lex.hpp"
#include "libjsonpath/node.hpp"
#include "libjsonpath/parse.hpp"
#include "libjsonpath/path.hpp"
#include "libjsonpath/selectors.hpp"
#include "libjsonpath/tokens.hpp"
#include "libjsonpath/utils.hpp"
#include "nanobind/nanobind.h"
#include "nanobind/stl/bind_map.h"
#include "nanobind/stl/bind_vector.h"
#include "nanobind/stl/map.h"
#include "nanobind/stl/string.h"
#include "nanobind/stl/string_view.h"
#include "nanobind/stl/unordered_map.h"
#include "nanobind/stl/vector.h"

namespace nb = nanobind;

NB_MAKE_OPAQUE(libjsonpath::JSONPathNodeList);
NB_MAKE_OPAQUE(libjsonpath::function_signature_map);
NB_MAKE_OPAQUE(libjsonpath::function_extension_map);

NB_MODULE(_jsonpath24, m) {
  m.doc() = "Python bindings for libjsonpath";
  nb::bind_vector<libjsonpath::JSONPathNodeList>(m, "JSONPathNodeList");
  nb::bind_map<libjsonpath::function_signature_map>(m, "FunctionSignatureMap");
  nb::bind_map<libjsonpath::function_extension_map>(m, "FunctionExtensionMap");

  nb::enum_<libjsonpath::TokenType>(m, "TokenType")
      .value("eof_", libjsonpath::TokenType::eof_)
      .value("and_", libjsonpath::TokenType::and_)
      .value("colon", libjsonpath::TokenType::colon)
      .value("comma", libjsonpath::TokenType::comma)
      .value("current", libjsonpath::TokenType::current)
      .value("ddot", libjsonpath::TokenType::ddot)
      .value("dq_string", libjsonpath::TokenType::dq_string)
      .value("eq", libjsonpath::TokenType::eq)
      .value("error", libjsonpath::TokenType::error)
      .value("false_", libjsonpath::TokenType::false_)
      .value("filter", libjsonpath::TokenType::filter_)
      .value("float_", libjsonpath::TokenType::float_)
      .value("func_", libjsonpath::TokenType::func_)
      .value("ge", libjsonpath::TokenType::ge)
      .value("gt", libjsonpath::TokenType::gt)
      .value("index", libjsonpath::TokenType::index)
      .value("int_", libjsonpath::TokenType::int_)
      .value("lbracket", libjsonpath::TokenType::lbracket)
      .value("le", libjsonpath::TokenType::le)
      .value("lparen", libjsonpath::TokenType::lparen)
      .value("lt", libjsonpath::TokenType::lt)
      .value("name", libjsonpath::TokenType::name_)
      .value("ne", libjsonpath::TokenType::ne)
      .value("not_", libjsonpath::TokenType::not_)
      .value("null_", libjsonpath::TokenType::null_)
      .value("or_", libjsonpath::TokenType::or_)
      .value("rbracket", libjsonpath::TokenType::rbracket)
      .value("root", libjsonpath::TokenType::root)
      .value("rparen", libjsonpath::TokenType::rparen)
      .value("sq_string", libjsonpath::TokenType::sq_string)
      .value("true_", libjsonpath::TokenType::true_)
      .value("wild", libjsonpath::TokenType::wild)
      .export_values();

  nb::class_<libjsonpath::Token>(m, "Token")
      .def_ro("type", &libjsonpath::Token::type)
      .def_ro("value", &libjsonpath::Token::value)
      .def_ro("index", &libjsonpath::Token::index)
      .def_ro("query", &libjsonpath::Token::query)
      .def("__str__", [](const libjsonpath::Token& t) {
        return libjsonpath::token_to_string(t);
      });

  nb::class_<libjsonpath::Lexer>(m, "Lexer")
      .def(nb::init<std::string_view>())
      .def("run", &libjsonpath::Lexer::run)
      .def("tokens", &libjsonpath::Lexer::tokens);

  nb::class_<libjsonpath::Parser>(m, "Parser")
      .def(nb::init<>())
      .def(nb::init<std::unordered_map<std::string,
                                       libjsonpath::FunctionExtensionTypes>>())
      .def("parse",
           nb::overload_cast<const libjsonpath::Tokens&>(
               &libjsonpath::Parser::parse, nb::const_),
           "Parse a JSONPath from a sequence of tokens", nb::rv_policy::move)

      .def("parse",
           nb::overload_cast<std::string_view>(&libjsonpath::Parser::parse,
                                               nb::const_),
           "Parse a JSONPath query string", nb::rv_policy::move);

  nb::enum_<libjsonpath::BinaryOperator>(m, "BinaryOperator")
      .value("none", libjsonpath::BinaryOperator::none)
      .value("logical_and", libjsonpath::BinaryOperator::logical_and)
      .value("logical_or", libjsonpath::BinaryOperator::logical_or)
      .value("eq", libjsonpath::BinaryOperator::eq)
      .value("ge", libjsonpath::BinaryOperator::ge)
      .value("gt", libjsonpath::BinaryOperator::gt)
      .value("le", libjsonpath::BinaryOperator::le)
      .value("lt", libjsonpath::BinaryOperator::lt)
      .value("ne", libjsonpath::BinaryOperator::ne);

  nb::class_<libjsonpath::NullLiteral>(m, "NullLiteral")
      .def_ro("token", &libjsonpath::NullLiteral::token);

  nb::class_<libjsonpath::BooleanLiteral>(m, "BooleanLiteral")
      .def_ro("token", &libjsonpath::BooleanLiteral::token)
      .def_ro("value", &libjsonpath::BooleanLiteral::value);

  nb::class_<libjsonpath::IntegerLiteral>(m, "IntegerLiteral")
      .def_ro("token", &libjsonpath::IntegerLiteral::token)
      .def_ro("value", &libjsonpath::IntegerLiteral::value);

  nb::class_<libjsonpath::FloatLiteral>(m, "FloatLiteral")
      .def_ro("token", &libjsonpath::FloatLiteral::token)
      .def_ro("value", &libjsonpath::FloatLiteral::value);

  nb::class_<libjsonpath::StringLiteral>(m, "StringLiteral")
      .def_ro("token", &libjsonpath::StringLiteral::token)
      .def_ro("value", &libjsonpath::StringLiteral::value);

  nb::class_<libjsonpath::Box<libjsonpath::LogicalNotExpression>>(
      m, "LogicalNotExpression")
      .def_prop_ro(
          "token",
          [](const libjsonpath::Box<libjsonpath::LogicalNotExpression>& e) {
            return e->token;
          })
      .def_prop_ro(
          "right",
          [](const libjsonpath::Box<libjsonpath::LogicalNotExpression>& e) {
            return e->right;
          });

  nb::class_<libjsonpath::Box<libjsonpath::InfixExpression>>(m,
                                                             "InfixExpression")
      .def_prop_ro("token",
                   [](const libjsonpath::Box<libjsonpath::InfixExpression>& e) {
                     return e->token;
                   })
      .def_prop_ro("left",
                   [](const libjsonpath::Box<libjsonpath::InfixExpression>& e) {
                     return e->left;
                   })
      .def_prop_ro("op",
                   [](const libjsonpath::Box<libjsonpath::InfixExpression>& e) {
                     return e->op;
                   })
      .def_prop_ro("right",
                   [](const libjsonpath::Box<libjsonpath::InfixExpression>& e) {
                     return e->right;
                   });

  nb::class_<libjsonpath::Box<libjsonpath::RelativeQuery>>(m, "RelativeQuery")
      .def_prop_ro("token",
                   [](const libjsonpath::Box<libjsonpath::RelativeQuery>& e) {
                     return e->token;
                   })
      .def_prop_ro("query",
                   [](const libjsonpath::Box<libjsonpath::RelativeQuery>& e) {
                     return e->query;
                   });

  nb::class_<libjsonpath::Box<libjsonpath::RootQuery>>(m, "RootQuery")
      .def_prop_ro("token",
                   [](const libjsonpath::Box<libjsonpath::RootQuery>& e) {
                     return e->token;
                   })
      .def_prop_ro("query",
                   [](const libjsonpath::Box<libjsonpath::RootQuery>& e) {
                     return e->query;
                   });

  nb::class_<libjsonpath::Box<libjsonpath::FunctionCall>>(m, "FunctionCall")
      .def_prop_ro("token",
                   [](const libjsonpath::Box<libjsonpath::FunctionCall>& e) {
                     return e->token;
                   })
      .def_prop_ro("name",
                   [](const libjsonpath::Box<libjsonpath::FunctionCall>& e) {
                     return e->name;
                   })
      .def_prop_ro("args",
                   [](const libjsonpath::Box<libjsonpath::FunctionCall>& e) {
                     return e->args;
                   });

  nb::class_<libjsonpath::NameSelector>(m, "NameSelector")
      .def_ro("token", &libjsonpath::NameSelector::token)
      .def_ro("name", &libjsonpath::NameSelector::name)
      .def_ro("shorthand", &libjsonpath::NameSelector::shorthand);

  nb::class_<libjsonpath::IndexSelector>(m, "IndexSelector")
      .def_ro("token", &libjsonpath::IndexSelector::token)
      .def_ro("index", &libjsonpath::IndexSelector::index);

  nb::class_<libjsonpath::WildSelector>(m, "WildSelector")
      .def_ro("token", &libjsonpath::WildSelector::token)
      .def_ro("shorthand", &libjsonpath::WildSelector::shorthand);

  nb::class_<libjsonpath::SliceSelector>(m, "SliceSelector")
      .def_ro("token", &libjsonpath::SliceSelector::token)
      .def_ro("start", &libjsonpath::SliceSelector::start)
      .def_ro("stop", &libjsonpath::SliceSelector::stop)
      .def_ro("step", &libjsonpath::SliceSelector::step);

  nb::class_<libjsonpath::Box<libjsonpath::FilterSelector>>(m, "FilterSelector")
      .def_prop_ro(
          "token",
          [](const libjsonpath::Box<libjsonpath::FilterSelector>& selector) {
            return selector->token;
          })
      .def_prop_ro(
          "expression",
          [](const libjsonpath::Box<libjsonpath::FilterSelector>& selector) {
            return selector->expression;
          });

  nb::class_<libjsonpath::Segment>(m, "Segment")
      .def_ro("token", &libjsonpath::Segment::token)
      .def_ro("selectors", &libjsonpath::Segment::selectors);

  nb::class_<libjsonpath::RecursiveSegment>(m, "RecursiveSegment")
      .def_ro("token", &libjsonpath::RecursiveSegment::token)
      .def_ro("selectors", &libjsonpath::RecursiveSegment::selectors);

  nb::enum_<libjsonpath::ExpressionType>(m, "ExpressionType")
      .value("value", libjsonpath::ExpressionType::value)
      .value("logical", libjsonpath::ExpressionType::logical)
      .value("nodes", libjsonpath::ExpressionType::nodes)
      .export_values();

  nb::class_<libjsonpath::FunctionExtensionTypes>(m, "FunctionExtensionTypes")
      .def(nb::init<std::vector<libjsonpath::ExpressionType>,
                    libjsonpath::ExpressionType>())
      .def_ro("args", &libjsonpath::FunctionExtensionTypes::args)
      .def_ro("res", &libjsonpath::FunctionExtensionTypes::res);

  m.def("parse", nb::overload_cast<std::string_view>(&libjsonpath::parse),
        "Parse a JSONPath query string", nb::rv_policy::move);

  m.def(
      "parse",
      nb::overload_cast<
          std::string_view,
          std::unordered_map<std::string, libjsonpath::FunctionExtensionTypes>>(
          &libjsonpath::parse),
      "Parse a JSONPath query string", nb::rv_policy::move);

  m.def("to_string", &libjsonpath::to_string, "JSONPath segments as a string");
  m.def("singular_query", &libjsonpath::singular_query,
        "Return True if a JSONPath is a singular query");

  nb::class_<libjsonpath::JSONPathNode>(m, "JSONPathNode")
      .def_ro("value", &libjsonpath::JSONPathNode::value)
      .def_ro("location", &libjsonpath::JSONPathNode::location)
      .def("path", &libjsonpath::JSONPathNode::path);

  m.def("query_",
        nb::overload_cast<const libjsonpath::segments_t&, nb::object,
                          libjsonpath::function_extension_map,
                          libjsonpath::function_signature_map, nb::object>(
            &libjsonpath::query_),
        "Query JSON-like data", nb::rv_policy::move);

  m.def("query_",
        nb::overload_cast<std::string_view, nb::object,
                          libjsonpath::function_extension_map,
                          libjsonpath::function_signature_map, nb::object>(
            &libjsonpath::query_),
        "Query JSON-like data", nb::rv_policy::move);

  nb::class_<libjsonpath::Env_>(m, "Env_")
      .def(nb::init<libjsonpath::function_extension_map,
                    libjsonpath::function_signature_map, nb::object>())
      .def("query", &libjsonpath::Env_::query, nb::rv_policy::move)
      .def("from_segments", &libjsonpath::Env_::from_segments,
           nb::rv_policy::move)
      .def("parse", &libjsonpath::Env_::parse, nb::rv_policy::move);
}