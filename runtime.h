#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace runtime {

    // ----------------------Context-----------------------
    class Context {
    public:
        virtual std::ostream& GetOutputStream() = 0;

    protected:
        ~Context() = default;
    };

    // ----------------------Object-----------------------
    class Object {
    public:
        virtual                                      ~Object() = default;

        virtual void                                 Print(std::ostream& os, Context& context) = 0;
    };

    // ----------------------ObjectHolder-----------------------
    class ObjectHolder {
    public:
        ObjectHolder() = default;

        template <typename T>
        [[nodiscard]] static ObjectHolder             Own(T&& object);

        [[nodiscard]] static ObjectHolder             Share(Object& object);

        [[nodiscard]] static ObjectHolder             None();

        Object& operator*() const;

        Object* operator->() const;

        [[nodiscard]] Object* Get() const;

        template <typename T>
        [[nodiscard]] T* TryAs() const;

        explicit                                      operator bool() const;

    private:
        explicit                                      ObjectHolder(std::shared_ptr<Object> data);

        void                                          AssertIsValid() const;

        std::shared_ptr<Object>                       data_;
    };

    template <typename T>
    ObjectHolder ObjectHolder::Own(T&& object) {
        return ObjectHolder(std::make_shared<T>(std::forward<T>(object)));
    }

    template <typename T>
    T* ObjectHolder::TryAs() const {
        return dynamic_cast<T*>(this->Get());
    }

    // ----------------------ValueObject-----------------------
    template <typename T>
    class ValueObject : public Object {
    public:
        ValueObject(T v);

        void                                          Print(std::ostream& os, Context& context) override;

        [[nodiscard]] const T& GetValue() const;

    private:
        T                                             value_;
    };

    template <typename T>
    ValueObject<T>::ValueObject(T v)
        : value_(v) {}

    template <typename T>
    void ValueObject<T>::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << value_;
    }

    template <typename T>
    const T& ValueObject<T>::GetValue() const {
        return value_;
    }

    // ----------------------Closure-----------------------

    using Closure = std::unordered_map<std::string, ObjectHolder>;

    // ----------------------IsTrue-----------------------

    bool IsTrue(const ObjectHolder& object);

    // ----------------------Executable-----------------------
    class Executable {
    public:
        virtual                                        ~Executable() = default;
        virtual ObjectHolder                           Execute(Closure& closure, Context& context) = 0;
    };

    // ----------------------String-----------------------

    using String = ValueObject<std::string>;

    // ----------------------Number-----------------------

    using Number = ValueObject<int>;

    // ----------------------Bool-----------------------
    class Bool : public ValueObject<bool> {
    public:
        using ValueObject<bool>::ValueObject;

        void                                           Print(std::ostream& os, Context& context) override;
    };

    // ----------------------Method-----------------------
    struct Method {
        std::string                                    name;
        std::vector<std::string>                       formal_params;
        std::unique_ptr<Executable>                    body;
    };

    // ----------------------Class-----------------------
    class Class : public Object {
    public:
        explicit                                       Class(std::string name,
            std::vector<Method> methods,
            const Class* parent);

        [[nodiscard]] const Method* GetMethod(const std::string& name) const;

        const std::string& GetName() const;

        void                                           Print(std::ostream& os, Context& context) override;

    private:
        std::string                                    name_;
        std::vector<Method>                            methods_;
        const Class* parent_;
    };

    // ----------------------ClassInstance-----------------------
    class ClassInstance : public Object {
    public:
        explicit                                       ClassInstance(const Class& cls);

        void                                           Print(std::ostream& os, Context& context) override;

        ObjectHolder                                   Call(const std::string& method,
            const std::vector<ObjectHolder>& actual_args,
            Context& context);

        [[nodiscard]] bool                             HasMethod(const std::string& method, size_t argument_count) const;

        [[nodiscard]] Closure& Fields();

        [[nodiscard]] const Closure& Fields() const;

    private:
        const Class& cls_;
        Closure                                        closure_;
    };

    // ----------------------Predicate-----------------------
    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);
    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context);

    // ----------------------DummyContext-----------------------
    struct DummyContext : Context {
        std::ostream& GetOutputStream() override;

        std::ostringstream                             output;
    };

    // ----------------------SimpleContext-----------------------
    class SimpleContext : public Context {
    public:
        explicit                                       SimpleContext(std::ostream& output);

        std::ostream& GetOutputStream() override;

    private:
        std::ostream& output_;
    };

}  // namespace runtime