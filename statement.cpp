#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

    using runtime::Closure;
    using runtime::Context;
    using runtime::IsTrue;
    using runtime::ObjectHolder;

    namespace {
        const string ADD_METHOD = "__add__"s;
        const string INIT_METHOD = "__init__"s;
    }  // namespace

#define BINARY_OPERATION(type, lhs, rhs, op)                                                               \
        if (lhs.TryAs<type>() && rhs.TryAs<type>()) {                                                          \
            return ObjectHolder::Own(type(lhs.TryAs<type>()->GetValue() op rhs.TryAs<type>()->GetValue()));    \
        }

    // -----------------------VariableValue---------------------------

    VariableValue::VariableValue(const std::string& var_name)
        : dotted_ids_(1, var_name) {}

    VariableValue::VariableValue(std::vector<std::string> dotted_ids)
        : dotted_ids_(move(dotted_ids)) {}

    ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
        Closure copy(closure);
        for (size_t i = 0; i < dotted_ids_.size(); ++i) {
            const string& field_name = dotted_ids_.at(i);
            if (copy.count(field_name) == 0) {
                throw runtime_error("Not field"s + field_name);
            }
            if (i == dotted_ids_.size() - 1) {
                return copy.at(field_name);
            }
            auto ptr_obj = copy.at(field_name).TryAs<runtime::ClassInstance>();
            if (!ptr_obj) {
                throw runtime_error("This isn't object"s);
            }
            copy = ptr_obj->Fields();
        }
        return {};
    }

    // -----------------------Assignment---------------------------

    Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
        : var_(move(var))
        , rv_(move(rv)) {}

    ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
        return closure[var_] = rv_->Execute(closure, context);
    }

    // -----------------------FieldAssignment---------------------------

    FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
        std::unique_ptr<Statement> rv)
        : object_(move(object))
        , field_name_(move(field_name))
        , rv_(move(rv)) {}

    ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
        auto ptr_obj = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
        if (ptr_obj) {
            return ptr_obj->Fields()[field_name_] = rv_->Execute(closure, context);
        }
        return {};
    }

    // -----------------------None---------------------------

    ObjectHolder None::Execute([[maybe_unused]] Closure& closure,
        [[maybe_unused]] Context& context) {
        return {};
    }

    // -----------------------Print---------------------------

    unique_ptr<Print> Print::Variable(const std::string& name) {
        auto arg = make_unique<VariableValue>(name);
        return make_unique<Print>(move(arg));
    }

    Print::Print(unique_ptr<Statement> argument) {
        args_.push_back(move(argument));
    }

    Print::Print(vector<unique_ptr<Statement>> args)
        : args_(move(args)) {}

    ObjectHolder Print::Execute(Closure& closure, Context& context) {
        auto& os = context.GetOutputStream();
        for (size_t i = 0; i < args_.size(); ++i) {
            auto ptr_obj = args_.at(i)->Execute(closure, context).Get();
            if (ptr_obj) {
                ptr_obj->Print(os, context);
            }
            else {
                os << "None"s;
            }
            if (i != args_.size() - 1) {
                os << ' ';
            }
        }
        os << '\n';
        return {};
    }

    // -----------------------MethodCall---------------------------

    MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
        std::vector<std::unique_ptr<Statement>> args)
        : object_(move(object))
        , method_(move(method))
        , args_(move(args)) {}

    ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
        auto ptr_obj = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
        if (ptr_obj) {
            vector<ObjectHolder> params;
            for (size_t i = 0; i < args_.size(); ++i) {
                params.push_back(args_.at(i)->Execute(closure, context));
            }
            return ptr_obj->Call(method_, params, context);
        }
        return {};
    }

    // -----------------------NewInstance---------------------------

    NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
        : cls_(class_)
        , args_(move(args)) {}

    NewInstance::NewInstance(const runtime::Class& class_)
        : cls_(class_) {}

    ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
        if (cls_.HasMethod(INIT_METHOD, args_.size())) {
            vector<ObjectHolder> params;
            for (size_t i = 0; i < args_.size(); ++i) {
                params.push_back(args_.at(i)->Execute(closure, context));
            }
            cls_.Call(INIT_METHOD, params, context);
        }
        return ObjectHolder::Share(cls_);
    }

    // -----------------------UnaryOperation---------------------------

    UnaryOperation::UnaryOperation(std::unique_ptr<Statement> argument)
        : argument_(std::move(argument)) {}

    // -----------------------Stringify---------------------------

    ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
        auto ptr_obj = argument_->Execute(closure, context).Get();
        if (ptr_obj) {
            ostringstream os;
            ptr_obj->Print(os, context);
            return ObjectHolder::Own(runtime::String(os.str()));
        }
        return  ObjectHolder::Own(runtime::String("None"s));
    }

    // -----------------------BinaryOperation---------------------------

    BinaryOperation::BinaryOperation(std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs)
        : lhs_(std::move(lhs))
        , rhs_(std::move(rhs)) {}

    // -----------------------Add---------------------------

    ObjectHolder Add::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        BINARY_OPERATION(runtime::Number, lhs, rhs, +);
        BINARY_OPERATION(runtime::String, lhs, rhs, +);
        if (auto ptr = lhs.TryAs<runtime::ClassInstance>(); ptr) {
            return ptr->Call(ADD_METHOD, { rhs }, context);
        }
        throw runtime_error("The operator is not overloaded +"s);
    }

    // -----------------------Sub---------------------------

    ObjectHolder Sub::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        BINARY_OPERATION(runtime::Number, lhs, rhs, -);
        throw runtime_error("The operator is not overloaded -"s);
    }

    // -----------------------Mult---------------------------

    ObjectHolder Mult::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        BINARY_OPERATION(runtime::Number, lhs, rhs, *);
        throw runtime_error("The operator is not overloaded *"s);
    }

    // -----------------------Div---------------------------

    ObjectHolder Div::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        const auto ptr_left = lhs.TryAs<runtime::Number>();
        const auto ptr_right = rhs.TryAs<runtime::Number>();
        if (ptr_left && ptr_right) {
            if (ptr_right->GetValue() != 0) {
                return ObjectHolder::Own(runtime::Number(ptr_left->GetValue() / ptr_right->GetValue()));
            }
            throw runtime_error("The denominator is zero"s);
        }
        throw runtime_error("The operator is not overloaded /"s);
    }

    // -----------------------Or---------------------------

    ObjectHolder Or::Execute(Closure& closure, Context& context) {
        if (!IsTrue(lhs_->Execute(closure, context))) {
            return ObjectHolder::Own(runtime::Bool(IsTrue(rhs_->Execute(closure, context))));
        }
        return ObjectHolder::Own(runtime::Bool(true));
    }

    // -----------------------And---------------------------

    ObjectHolder And::Execute(Closure& closure, Context& context) {
        if (IsTrue(lhs_->Execute(closure, context))) {
            return ObjectHolder::Own(runtime::Bool(IsTrue(rhs_->Execute(closure, context))));
        }
        return ObjectHolder::Own(runtime::Bool(false));
    }

    // -----------------------Not---------------------------

    ObjectHolder Not::Execute(Closure& closure, Context& context) {
        return ObjectHolder::Own(runtime::Bool(!IsTrue(argument_->Execute(closure, context))));
    }

    // -----------------------Compound---------------------------

    void Compound::AddStatement(std::unique_ptr<Statement> stmt) {
        args_.push_back(move(stmt));
    }

    ObjectHolder Compound::Execute(Closure& closure, Context& context) {
        for (size_t i = 0; i < args_.size(); ++i) {
            args_.at(i)->Execute(closure, context);
        }
        return {};
    }

    // -----------------------MethodBody---------------------------

    MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
        : body_(move(body)) {}

    ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
        try {
            body_->Execute(closure, context);
        }
        catch (const ObjectHolder& result) {
            return result;
        }
        return {};
    }

    // -----------------------Return---------------------------

    Return::Return(std::unique_ptr<Statement> statement)
        : statement_(std::move(statement)) {}

    ObjectHolder Return::Execute(Closure& closure, Context& context) {
        throw statement_->Execute(closure, context);
    }

    // -----------------------ClassDefinition---------------------------

    ClassDefinition::ClassDefinition(ObjectHolder cls)
        : cls_(cls) {}

    ObjectHolder ClassDefinition::Execute(Closure& closure, [[maybe_unused]] Context& context) {
        return closure[cls_.TryAs<runtime::Class>()->GetName()] = cls_;
    }

    // -----------------------IfElse---------------------------

    IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
        std::unique_ptr<Statement> else_body)
        : condition_(move(condition))
        , if_body_(move(if_body))
        , else_body_(move(else_body)) {}

    ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
        if (IsTrue(condition_->Execute(closure, context))) {
            return if_body_->Execute(closure, context);
        }
        if (else_body_) {
            return else_body_->Execute(closure, context);
        }
        return {};
    }

    // -----------------------Comparison---------------------------

    Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
        : BinaryOperation(std::move(lhs), std::move(rhs))
        , cmp_(cmp) {}

    ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
        ObjectHolder lhs = lhs_->Execute(closure, context);
        ObjectHolder rhs = rhs_->Execute(closure, context);
        return ObjectHolder::Own(runtime::Bool(cmp_(lhs, rhs, context)));
    }

}  // namespace ast