#pragma once

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


#include "ceto.h"
//#include "ceto_private_boundscheck.donotedit.autogenerated.h"

#include "ceto_private_listcomp.donotedit.autogenerated.h"
;
#include "ceto_private_boundscheck.donotedit.autogenerated.h"
;
#include "ceto_private_convenience.donotedit.autogenerated.h"
;
#include <map>
;
#include "ast.donotedit.autogenerated.h"
;
#include "utility.donotedit.autogenerated.h"
;
struct ClassDefinition : public ceto::shared_object, public std::enable_shared_from_this<ClassDefinition> {

    std::shared_ptr<const Identifier> name_node;

    std::shared_ptr<const Call> class_def_node;

    bool is_unique;

    bool is_struct;

    bool is_forward_declaration;

    decltype(false) is_pure_virtual = false;

    decltype(false) is_concrete = false;

        inline auto repr() const -> auto {
            return ((((((((((this -> class_name() + "(") + (*ceto::mad(this -> name_node)).repr()) + ", ") + (*ceto::mad(this -> class_def_node)).repr()) + std::to_string(this -> is_unique)) + ", ") + std::to_string(this -> is_struct)) + ", ") + std::to_string(this -> is_forward_declaration)) + ")");
        }

         virtual inline auto class_name() const -> std::string {
            return ceto::util::typeid_name((*this));
        }

         virtual ~ClassDefinition() {
            ; // pass
        }

    explicit ClassDefinition(std::shared_ptr<const Identifier> name_node, std::shared_ptr<const Call> class_def_node, bool is_unique, bool is_struct, bool is_forward_declaration) : name_node(std::move(name_node)), class_def_node(std::move(class_def_node)), is_unique(is_unique), is_struct(is_struct), is_forward_declaration(is_forward_declaration) {}

    ClassDefinition() = delete;

};

struct InterfaceDefinition : public ClassDefinition {

    explicit InterfaceDefinition() : ClassDefinition (nullptr, nullptr, false, false, false) {
    }

};

struct VariableDefinition : public ceto::shared_object, public std::enable_shared_from_this<VariableDefinition> {

    std::shared_ptr<const Identifier> defined_node;

    std::shared_ptr<const Node> defining_node;

        inline auto repr() const -> auto {
            return (((((this -> class_name() + "(") + (*ceto::mad(this -> defined_node)).repr()) + ", ") + (*ceto::mad(this -> defining_node)).repr()) + ")");
        }

         virtual inline auto class_name() const -> std::string {
            return ceto::util::typeid_name((*this));
        }

         virtual ~VariableDefinition() {
            ; // pass
        }

    explicit VariableDefinition(std::shared_ptr<const Identifier> defined_node, std::shared_ptr<const Node> defining_node) : defined_node(std::move(defined_node)), defining_node(std::move(defining_node)) {}

    VariableDefinition() = delete;

};

struct LocalVariableDefinition : public VariableDefinition {

using VariableDefinition::VariableDefinition;

};

struct GlobalVariableDefinition : public VariableDefinition {

using VariableDefinition::VariableDefinition;

};

struct FieldDefinition : public VariableDefinition {

using VariableDefinition::VariableDefinition;

};

struct ParameterDefinition : public VariableDefinition {

using VariableDefinition::VariableDefinition;

};

    inline auto creates_new_variable_scope(const std::shared_ptr<const Node>&  e) -> auto {
        if ((std::dynamic_pointer_cast<const Call>(e) != nullptr)) {
            const auto name = (*ceto::mad((*ceto::mad(e)).func)).name();
            if (name) {
                return ceto::util::contains(std::vector {{std::string {"def"}, std::string {"lambda"}, std::string {"class"}, std::string {"struct"}}}, (*ceto::mad_smartptr(name)).value());
            } else if (((std::dynamic_pointer_cast<const ArrayAccess>((*ceto::mad(e)).func) != nullptr) && ((*ceto::mad((*ceto::mad((*ceto::mad(e)).func)).func)).name() == "lambda"))) {
                return true;
            }
        }
        return false;
    }

    inline auto comes_before(const std::shared_ptr<const Node>&  root, const std::shared_ptr<const Node>&  before, const std::shared_ptr<const Node>&  after) -> std::optional<bool> {
        if (root == before) {
            return true;
        } else if ((root == after)) {
            return false;
        }
        for(const auto& arg : (*ceto::mad(root)).args) {
            const auto cb = comes_before(arg, before, after);
            if ((*ceto::mad_smartptr(cb)).has_value()) {
                return cb;
            }
        }
        if ((*ceto::mad(root)).func) {
            const auto cb = comes_before((*ceto::mad(root)).func, before, after);
            if ((*ceto::mad_smartptr(cb)).has_value()) {
                return cb;
            }
        }
        return {};
    }

struct Scope : public ceto::shared_object, public std::enable_shared_from_this<Scope> {

    decltype(std::map<std::string,std::vector<std::shared_ptr<const Node>>>()) interfaces = std::map<std::string,std::vector<std::shared_ptr<const Node>>>();

    std::vector<std::shared_ptr<const ClassDefinition>> class_definitions = std::vector<std::shared_ptr<const ClassDefinition>>{}; static_assert(ceto::is_non_aggregate_init_and_if_convertible_then_non_narrowing_v<decltype(std::vector<std::shared_ptr<const ClassDefinition>>{}), std::remove_cvref_t<decltype(class_definitions)>>);

    std::vector<std::shared_ptr<const VariableDefinition>> variable_definitions = std::vector<std::shared_ptr<const VariableDefinition>>{}; static_assert(ceto::is_non_aggregate_init_and_if_convertible_then_non_narrowing_v<decltype(std::vector<std::shared_ptr<const VariableDefinition>>{}), std::remove_cvref_t<decltype(variable_definitions)>>);

    decltype(0) indent = 0;

    std::weak_ptr<const Scope> _parent = {};

    decltype(false) in_function_body = false;

    decltype(false) in_function_param_list = false;

    decltype(false) in_class_body = false;

    decltype(false) in_decltype = false;

        inline auto indent_str() const -> auto {
            return std::string(4 * (this -> indent), ' ');
        }

        inline auto add_variable_definition(const std::shared_ptr<const Identifier>&  defined_node, const std::shared_ptr<const Node>&  defining_node) -> void {
            auto parent { (*ceto::mad(defined_node)).parent() } ;
            while (parent) {                if (creates_new_variable_scope(parent)) {
                    const auto name = (*ceto::mad((*ceto::mad(parent)).func)).name();
                    if ((name == "class") || (name == "struct")) {
                        const auto defn = std::make_shared<const FieldDefinition>(defined_node, defining_node);
                        (*ceto::mad(this -> variable_definitions)).push_back(defn);
                    } else if (((name == "def") || (name == "lambda"))) {
                        const auto defn = std::make_shared<const ParameterDefinition>(defined_node, defining_node);
                        (*ceto::mad(this -> variable_definitions)).push_back(defn);
                    } else {
                        const auto defn = std::make_shared<const LocalVariableDefinition>(defined_node, defining_node);
                        (*ceto::mad(this -> variable_definitions)).push_back(defn);
                    }
                    return;
                }
                parent = (*ceto::mad(parent)).parent();
            }
            const auto defn = std::make_shared<const GlobalVariableDefinition>(defined_node, defining_node);
            (*ceto::mad(this -> variable_definitions)).push_back(defn);
        }

        inline auto add_interface_method(const std::string&  interface_name, const std::shared_ptr<const Node>&  interface_method_def_node) -> void {
            (*ceto::mad(ceto::bounds_check(this -> interfaces, interface_name))).push_back(interface_method_def_node);
        }

        inline auto add_class_definition(const std::shared_ptr<const ClassDefinition>&  class_definition) -> void {
            (*ceto::mad(this -> class_definitions)).push_back(class_definition);
        }

        inline auto lookup_class(const std::shared_ptr<const Node>&  class_node) const -> std::shared_ptr<const ClassDefinition> {
            if (!(std::dynamic_pointer_cast<const Identifier>(class_node) != nullptr)) {
                return nullptr;
            }
            for(const auto& c : (this -> class_definitions)) {
                if ((*ceto::mad((*ceto::mad(c)).name_node)).name() == (*ceto::mad(class_node)).name()) {
                    return c;
                }
            }
            if ((*ceto::mad(this -> interfaces)).contains((*ceto::mad_smartptr((*ceto::mad(class_node)).name())).value())) {
                return std::make_shared<const InterfaceDefinition>();
            }
            if (const auto s = (*ceto::mad(this -> _parent)).lock()) {
                return (*ceto::mad(s)).lookup_class(class_node);
            }
            return nullptr;
        }

        inline auto find_defs(const std::shared_ptr<const Node>&  var_node, const decltype(true) find_all = true) const -> std::vector<std::shared_ptr<const VariableDefinition>> {
            if (!(std::dynamic_pointer_cast<const Identifier>(var_node) != nullptr)) {
                return {};
            }
            std::vector<std::shared_ptr<const VariableDefinition>> results = std::vector<std::shared_ptr<const VariableDefinition>>{}; static_assert(ceto::is_non_aggregate_init_and_if_convertible_then_non_narrowing_v<decltype(std::vector<std::shared_ptr<const VariableDefinition>>{}), std::remove_cvref_t<decltype(results)>>);
            for(const auto& d : (this -> variable_definitions)) {
                if (((*ceto::mad((*ceto::mad(d)).defined_node)).name() == (*ceto::mad(var_node)).name()) && ((*ceto::mad(d)).defined_node != var_node)) {
                    auto parent_block { (*ceto::mad((*ceto::mad(d)).defined_node)).parent() } ;
                    while (true) {                        if ((std::dynamic_pointer_cast<const Module>(parent_block) != nullptr)) {
                            break;
                        }
                        parent_block = (*ceto::mad(parent_block)).parent();
                    }
                    const auto defined_before = comes_before(parent_block, (*ceto::mad(d)).defined_node, var_node);
                    if (defined_before && (*ceto::mad_smartptr(defined_before)).value()) {
                        if (!find_all) {
                            return std::vector {d};
                        }
                        (*ceto::mad(results)).push_back(d);
                        if (const auto assign = std::dynamic_pointer_cast<const Assign>((*ceto::mad(d)).defining_node)) {
                            if (const auto ident = std::dynamic_pointer_cast<const Identifier>((*ceto::mad(assign)).rhs())) {
                                const auto more = this -> find_defs(ident, find_all);
                                (*ceto::mad(results)).insert((*ceto::mad(results)).end(), (*ceto::mad(more)).begin(), (*ceto::mad(more)).end());
                            }
                        }
                    }
                }
            }
            if (const auto s = (*ceto::mad(this -> _parent)).lock()) {
                const auto more = (*ceto::mad(s)).find_defs(var_node, find_all);
                (*ceto::mad(results)).insert((*ceto::mad(results)).end(), (*ceto::mad(more)).begin(), (*ceto::mad(more)).end());
            }
            return results;
        }

        inline auto find_def(const std::shared_ptr<const Node>&  var_node) const -> auto {
            const auto find_all = false;
            const auto found = this -> find_defs(var_node, find_all);
            return [&]() {if ((*ceto::mad(found)).size() > 0) {
                return ceto::bounds_check(found, 0);
            } else {
                const std::shared_ptr<const VariableDefinition> none_result = nullptr; static_assert(ceto::is_non_aggregate_init_and_if_convertible_then_non_narrowing_v<decltype(nullptr), std::remove_cvref_t<decltype(none_result)>>);
                return none_result;
            }}()
;
        }

        inline auto enter_scope() const -> std::shared_ptr<const Scope> {
            const auto self = ceto::shared_from(this);
            auto s { std::make_shared<Scope>() } ;
            (*ceto::mad(s))._parent = self;
            (*ceto::mad(s)).in_function_body = (this -> in_function_body);
            (*ceto::mad(s)).in_decltype = (this -> in_decltype);
            (*ceto::mad(s)).indent = ((this -> indent) + 1);
            return s;
        }

        inline auto parent() const -> auto {
            return (*ceto::mad(this -> _parent)).lock();
        }

};

