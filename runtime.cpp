#include "runtime.h"

#include <cassert>

using namespace std;

namespace runtime {

    // ----------------------ObjectHolder-----------------------

    ObjectHolder::ObjectHolder(shared_ptr<Object> data)
        : data_(move(data)) {
    }

    void ObjectHolder::AssertIsValid() const {
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object& object) {
        return ObjectHolder(shared_ptr<Object>(&object, [](auto*) {}));
    }

    ObjectHolder ObjectHolder::None() {
        return ObjectHolder();
    }

    Object& ObjectHolder::operator*() const {
        AssertIsValid();
        return *Get();
    }

    Object* ObjectHolder::operator->() const {
        AssertIsValid();
        return Get();
    }

    Object* ObjectHolder::Get() const {
        return data_.get();
    }

    ObjectHolder::operator bool() const {
        return Get() != nullptr;
    }

    // ----------------------IsTrue-----------------------

    bool IsTrue(const ObjectHolder& object) {
        if (auto ptr_num = object.TryAs<Number>(); ptr_num) {
            return ptr_num->GetValue() != 0;
        }
        if (auto ptr_str = object.TryAs<String>(); ptr_str) {
            return !ptr_str->GetValue().empty();
        }
        if (auto ptr_bool = object.TryAs<Bool>(); ptr_bool) {
            return ptr_bool->GetValue();
        }
        return false;
    }

    // ----------------------Bool-----------------------

    void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
        os << (GetValue() ? "True"sv : "False"sv);
    }

    // ----------------------Class-----------------------

    Class::Class(string name, vector<Method> methods, const Class* parent)
        : name_(move(name))
        , methods_(move(methods))
        , parent_(parent) {}

    const Method* Class::GetMethod(const std::string& name) const {
        for (size_t i = 0; i < methods_.size(); ++i) {
            if (methods_.at(i).name == name) {
                return &methods_.at(i);
            }
        }
        if (parent_) {
            return parent_->GetMethod(name);
        }
        return nullptr;
    }

    const std::string& Class::GetName() const {
        return name_;
    }

    void Class::Print(ostream& os, [[maybe_unused]] Context& context) {
        os << "Class "sv << GetName();
    }

    // ----------------------ClassInstance-----------------------

    ClassInstance::ClassInstance(const Class& cls)
        : cls_(cls) {}

    void ClassInstance::Print(ostream& os, Context& context) {
        const string method_name = "__str__"s;
        if (HasMethod(method_name, 0)) {
            Call(method_name, {}, context).Get()->Print(os, context);
        }
        else {
            os << this;
        }
    }

    ObjectHolder ClassInstance::Call(const std::string& method,
        const std::vector<ObjectHolder>& actual_args,
        Context& context) {
        if (HasMethod(method, actual_args.size())) {
            Closure closure;
            for (size_t i = 0; i < actual_args.size(); ++i) {
                closure.emplace(cls_.GetMethod(method)->formal_params.at(i), actual_args.at(i));
            }
            closure.emplace("self"s, ObjectHolder::Share(*this));
            return cls_.GetMethod(method)->body->Execute(closure, context);
        }
        throw runtime_error("No method "s + method);
    }

    bool ClassInstance::HasMethod(const string& method, size_t argument_count) const {
        const Method* ptr_method = cls_.GetMethod(method);
        return ptr_method && ptr_method->formal_params.size() == argument_count;
    }

    Closure& ClassInstance::Fields() {
        return closure_;
    }

    const Closure& ClassInstance::Fields() const {
        return closure_;
    }

    // ----------------------Predicate-----------------------

#define COMPARE(type, lhs, rhs, sign)                                                    \
            if (lhs.TryAs<type>() && rhs.TryAs<type>())  {                                   \
                return lhs.TryAs<type>()->GetValue() sign rhs.TryAs<type>()->GetValue();     \
            }                                                                           


#define COMPARE_FUNC(type, lhs, rhs, context, func)                                      \
            if (auto ptr = lhs.TryAs<type>(); ptr && ptr->HasMethod(func, 1))  {             \
                    return IsTrue(ptr->Call(func, {rhs}, context));                          \
            }                                                                                 

    bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs && !rhs) {
            return true;
        }
        COMPARE(Number, lhs, rhs, == );
        COMPARE(String, lhs, rhs, == );
        COMPARE(Bool, lhs, rhs, == );
        COMPARE_FUNC(ClassInstance, lhs, rhs, context, "__eq__"s);
        throw runtime_error("Cannot compare objects for equality"s);
    }

    bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        if (!lhs && !rhs) {
            runtime_error("Cannot compare objects for less"s);
        }
        COMPARE(Number, lhs, rhs, < );
        COMPARE(String, lhs, rhs, < );
        COMPARE(Bool, lhs, rhs, < );
        COMPARE_FUNC(ClassInstance, lhs, rhs, context, "__lt__"s);
        throw runtime_error("Cannot compare objects for less"s);
    }

    bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !LessOrEqual(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return Less(lhs, rhs, context) || Equal(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
        return !Less(lhs, rhs, context);
    }

    // ----------------------DummyContext-----------------------

    std::ostream& DummyContext::GetOutputStream() {
        return output;
    }

    // ----------------------SimpleContext-----------------------

    SimpleContext::SimpleContext(std::ostream& output)
        : output_(output) {}

    std::ostream& SimpleContext::GetOutputStream() {
        return output_;
    }

}  // namespace runtime