
#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <cassert>
#include <compare> // for <=>
#include <thread>
#include <optional>

//#include <concepts>
//#include <ranges>
//#include <numeric>


#include "ceto.h"


#include <map>
#include <typeinfo>
#include <numeric>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
;
#include "ast.h"
;
namespace py = pybind11;

PYBIND11_MODULE(_abstractsyntaxtree, m) {
;
[]( auto &&  m) {
        auto node { ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(py::class_<Node,std::shared_ptr<Node>>(m, "Node"))->def_readwrite("func", (&Node::func)))->def_readwrite("args", (&Node::args)))->def_readwrite("declared_type", (&Node::declared_type)))->def_readwrite("scope", (&Node::scope)))->def_readwrite("source", (&Node::source)))->def("__repr__", (&Node::repr)))->def_property_readonly("name", (&Node::name)))->def_property("parent", (&Node::parent), (&Node::set_parent)))->def_readwrite("from_include", (&Node::from_include)) } ;
        ceto::mado(ceto::mado(py::class_<UnOp,std::shared_ptr<UnOp>>(m, "UnOp", node))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("op", (&UnOp::op));
        ceto::mado(ceto::mado(py::class_<LeftAssociativeUnOp,std::shared_ptr<LeftAssociativeUnOp>>(m, "LeftAssociativeUnOp", node))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("op", (&LeftAssociativeUnOp::op));
        auto binop { py::class_<BinOp,std::shared_ptr<BinOp>>(m, "BinOp", node) } ;
        ceto::mado(ceto::mado(ceto::mado(ceto::mado(binop)->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("op", (&BinOp::op)))->def_property_readonly("lhs", (&BinOp::lhs)))->def_property_readonly("rhs", (&BinOp::rhs));
        auto typeop { py::class_<TypeOp,std::shared_ptr<TypeOp>>(m, "TypeOp", binop) } ;
        ceto::mado(typeop)->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(ceto::mado(py::class_<SyntaxTypeOp,std::shared_ptr<SyntaxTypeOp>>(m, "SyntaxTypeOp", typeop))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("synthetic_lambda_return_lambda", (&SyntaxTypeOp::synthetic_lambda_return_lambda));
        ceto::mado(py::class_<AttributeAccess,std::shared_ptr<AttributeAccess>>(m, "AttributeAccess", binop))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<ArrowOp,std::shared_ptr<ArrowOp>>(m, "ArrowOp", binop))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<ScopeResolution,std::shared_ptr<ScopeResolution>>(m, "ScopeResolution", binop))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        auto assign { py::class_<Assign,std::shared_ptr<Assign>>(m, "Assign", binop) } ;
        ceto::mado(assign)->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<NamedParameter,std::shared_ptr<NamedParameter>>(m, "NamedParameter", assign))->def(py::init<const std::string &,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("op"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(ceto::mado(py::class_<Call,std::shared_ptr<Call>>(m, "Call", node))->def(py::init<std::shared_ptr<const Node>,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("func"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("is_one_liner_if", (&Call::is_one_liner_if));
        ceto::mado(py::class_<ArrayAccess,std::shared_ptr<ArrayAccess>>(m, "ArrayAccess", node))->def(py::init<std::shared_ptr<const Node>,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("func"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<BracedCall,std::shared_ptr<BracedCall>>(m, "BracedCall", node))->def(py::init<std::shared_ptr<const Node>,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("func"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<Template,std::shared_ptr<Template>>(m, "Template", node))->def(py::init<std::shared_ptr<const Node>,std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("func"), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<Identifier,std::shared_ptr<Identifier>>(m, "Identifier", node))->def(py::init<const std::string &,std::tuple<std::string,int>>(), py::arg("name"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(ceto::mado(ceto::mado(ceto::mado(ceto::mado(py::class_<StringLiteral,std::shared_ptr<StringLiteral>>(m, "StringLiteral", node))->def(py::init<const std::string &,std::shared_ptr<const Identifier>,std::shared_ptr<const Identifier>,std::tuple<std::string,int>>(), py::arg("str"), py::arg("prefix"), py::arg("suffix"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readonly("str", (&StringLiteral::str)))->def_readwrite("prefix", (&StringLiteral::prefix)))->def_readwrite("suffix", (&StringLiteral::suffix)))->def("escaped", (&StringLiteral::escaped));
        ceto::mado(ceto::mado(ceto::mado(py::class_<IntegerLiteral,std::shared_ptr<IntegerLiteral>>(m, "IntegerLiteral", node))->def(py::init<const std::string &,std::shared_ptr<const Identifier>,std::tuple<std::string,int>>(), py::arg("integer_string"), py::arg("suffix"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readonly("integer_string", (&IntegerLiteral::integer_string)))->def_readonly("suffix", (&IntegerLiteral::suffix));
        ceto::mado(ceto::mado(ceto::mado(py::class_<FloatLiteral,std::shared_ptr<FloatLiteral>>(m, "FloatLiteral", node))->def(py::init<const std::string &,std::shared_ptr<const Identifier>,std::tuple<std::string,int>>(), py::arg("float_string"), py::arg("suffix"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readonly("float_string", (&FloatLiteral::float_string)))->def_readonly("suffix", (&FloatLiteral::suffix));
        auto list_like { py::class_<ListLike_,std::shared_ptr<ListLike_>>(m, "ListLike_", node) } ;
        ceto::mado(py::class_<ListLiteral,std::shared_ptr<ListLiteral>>(m, "ListLiteral", list_like))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<TupleLiteral,std::shared_ptr<TupleLiteral>>(m, "TupleLiteral", list_like))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<BracedLiteral,std::shared_ptr<BracedLiteral>>(m, "BracedLiteral", list_like))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        auto block { py::class_<Block,std::shared_ptr<Block>>(m, "Block", list_like) } ;
        ceto::mado(block)->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(ceto::mado(py::class_<Module,std::shared_ptr<Module>>(m, "Module", block))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0)))->def_readwrite("has_main_function", (&Module::has_main_function));
        ceto::mado(py::class_<RedundantParens,std::shared_ptr<RedundantParens>>(m, "RedundantParens", node))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(py::class_<InfixWrapper_,std::shared_ptr<InfixWrapper_>>(m, "InfixWrapper_", node))->def(py::init<std::vector<std::shared_ptr<const Node>>,std::tuple<std::string,int>>(), py::arg("args"), py::arg("source") = std::make_tuple(std::string {""}, 0));
        ceto::mado(m)->def("set_string_replace_function", (&set_string_replace_function), "unfortunate kludge until we fix the baffling escape sequence probs in the selfhost implementation");
        ceto::mado(m)->def("macro_trampoline", (&macro_trampoline), "macro trampoline");
        return;
        }(m);
};
