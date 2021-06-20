#pragma once

#include "runtime.h"

#include <functional>
#include <utility>

namespace ast {

    // -----------------------Statement---------------------------

    using Statement = runtime::Executable;

    // -----------------------ValueStatement---------------------------

    template <typename T>
    class ValueStatement : public Statement {
    public:
        explicit                                               ValueStatement(T v);

        runtime::ObjectHolder                                  Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        T                                                      value_;
    };

    template <typename T>
    ValueStatement<T>::ValueStatement(T v)
        : value_(std::move(v)) {}

    template <typename T>
    runtime::ObjectHolder ValueStatement<T>::Execute([[maybe_unused]] runtime::Closure& closure,
        [[maybe_unused]] runtime::Context& context) {
        return runtime::ObjectHolder::Share(value_);
    }

    // -----------------------NumericConst---------------------------

    using NumericConst = ValueStatement<runtime::Number>;

    // -----------------------StringConst---------------------------

    using StringConst = ValueStatement<runtime::String>;

    // -----------------------BoolConst---------------------------

    using BoolConst = ValueStatement<runtime::Bool>;

    // -----------------------VariableValue---------------------------

    class VariableValue : public Statement {
    public:
        explicit                                                 VariableValue(const std::string& var_name);

        explicit                                                 VariableValue(std::vector<std::string> dotted_ids);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::vector<std::string>                                 dotted_ids_;
    };

    // -----------------------Assignment---------------------------

    class Assignment : public Statement {
    public:
        Assignment(std::string var,
            std::unique_ptr<Statement> rv);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::string                                              var_;
        std::unique_ptr<Statement>                               rv_;
    };

    // -----------------------FieldAssignment---------------------------

    class FieldAssignment : public Statement {
    public:
        FieldAssignment(VariableValue object,
            std::string field_name,
            std::unique_ptr<Statement> rv);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        VariableValue                                            object_;
        std::string                                              field_name_;
        std::unique_ptr<Statement>                               rv_;
    };

    // -----------------------None---------------------------

    class None : public Statement {
    public:
        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Print---------------------------

    class Print : public Statement {
    public:
        explicit                                                 Print(std::unique_ptr<Statement> argument);

        explicit                                                 Print(std::vector<std::unique_ptr<Statement>> args);

        static std::unique_ptr<Print>                            Variable(const std::string& name);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::vector<std::unique_ptr<Statement>>                  args_;
    };

    // -----------------------MethodCall---------------------------

    class MethodCall : public Statement {
    public:
        MethodCall(std::unique_ptr<Statement> object,
            std::string method,
            std::vector<std::unique_ptr<Statement>> args);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::unique_ptr<Statement>                               object_;
        std::string                                              method_;
        std::vector<std::unique_ptr<Statement>>                  args_;
    };

    // -----------------------NewInstance---------------------------

    class NewInstance : public Statement {
    public:
        explicit                                                 NewInstance(const runtime::Class& class_);

        NewInstance(const runtime::Class& class_,
            std::vector<std::unique_ptr<Statement>> args);

        runtime::ObjectHolder                                    Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        runtime::ClassInstance                                   cls_;
        std::vector<std::unique_ptr<Statement>>                  args_;
    };

    // -----------------------UnaryOperation---------------------------

    class UnaryOperation : public Statement {
    public:
        explicit                                                 UnaryOperation(std::unique_ptr<Statement> argument);

    protected:
        std::unique_ptr<Statement>                               argument_;
    };

    // -----------------------Stringify---------------------------

    class Stringify : public UnaryOperation {
    public:
        using UnaryOperation::UnaryOperation;

        runtime::ObjectHolder                                     Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------BinaryOperation---------------------------

    class BinaryOperation : public Statement {
    public:
        BinaryOperation(std::unique_ptr<Statement> lhs,
            std::unique_ptr<Statement> rhs);

    protected:
        std::unique_ptr<Statement> lhs_, rhs_;
    };

    // -----------------------Add---------------------------

    class Add : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Sub---------------------------

    class Sub : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Mult---------------------------

    class Mult : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Div---------------------------

    class Div : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Or---------------------------

    class Or : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------And---------------------------

    class And : public BinaryOperation {
    public:
        using BinaryOperation::BinaryOperation;

        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Not---------------------------

    class Not : public UnaryOperation {
    public:
        using UnaryOperation::UnaryOperation;
        runtime::ObjectHolder                                      Execute(runtime::Closure& closure, runtime::Context& context) override;
    };

    // -----------------------Compound---------------------------

    class Compound : public Statement {
    public:
        template <typename... Args>
        explicit                                                    Compound(Args&&... args);

        void                                                        AddStatement(std::unique_ptr<Statement> stmt);

        runtime::ObjectHolder                                       Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::vector<std::unique_ptr<Statement>>                     args_;
    };

    template <typename... Args>
    Compound::Compound(Args&&... args) {
        if constexpr (sizeof...(args) != 0) {
            (..., (args_.push_back(std::forward<Args>(args))));
        }
    }

    // -----------------------MethodBody---------------------------

    class MethodBody : public Statement {
    public:
        explicit                                                    MethodBody(std::unique_ptr<Statement>&& body);

        runtime::ObjectHolder                                       Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::unique_ptr<Statement>                                  body_;
    };

    // -----------------------Return---------------------------

    class Return : public Statement {
    public:
        explicit Return(std::unique_ptr<Statement> statement);

        runtime::ObjectHolder                                       Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::unique_ptr<Statement>                                  statement_;
    };

    // -----------------------ClassDefinition---------------------------

    class ClassDefinition : public Statement {
    public:
        explicit                                                     ClassDefinition(runtime::ObjectHolder cls);

        runtime::ObjectHolder                                        Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        runtime::ObjectHolder                                        cls_;
    };

    // -----------------------IfElse---------------------------

    class IfElse : public Statement {
    public:
        IfElse(std::unique_ptr<Statement> condition,
            std::unique_ptr<Statement> if_body,
            std::unique_ptr<Statement> else_body);

        runtime::ObjectHolder                                        Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::unique_ptr<Statement>                                   condition_, if_body_, else_body_;
    };

    // -----------------------Comparison---------------------------

    class Comparison : public BinaryOperation {
    public:
        using Comparator = std::function<bool(const runtime::ObjectHolder&,
            const runtime::ObjectHolder&, runtime::Context&)>;

        Comparison(Comparator cmp, std::unique_ptr<Statement> lhs, std::unique_ptr<Statement> rhs);

        runtime::ObjectHolder                                        Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        Comparator cmp_;
    };

}  // namespace ast