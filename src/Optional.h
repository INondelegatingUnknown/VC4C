/*
 * Author: doe300
 *
 * See the file "LICENSE" for the full license governing this code.
 */

#ifndef VC4C_OPTIONAL_H
#define VC4C_OPTIONAL_H

#include "CompilationError.h"

#include <functional>
#include <type_traits>
#if __has_include(<optional>) && __cplusplus >= 201703L
#include <optional>
template <typename T>
using OptionalBase = std::optional<T>;
using OptionalException = std::bad_optional_access;
#elif __has_include(<experimental/optional>)
#include <experimental/optional>
template <typename T>
using OptionalBase = std::experimental::optional<T>;
using OptionalException = std::experimental::bad_optional_access;
#endif

namespace vc4c
{
    /**
     * Tag type marking the constructor or conversion function which generates the "empty" value
     */
    struct optional_tombstone_tag
    {
    };

    /**
     * Traits type to enable tombstone values for arbitrary types
     */
    template <typename T>
    struct tombstone_traits
    {
        /**
         * Needs to be set to true to enable usage of tombstone values
         */
        static constexpr bool is_specialized = false;

        /**
         * The tombstone value to be used
         */
        // static const T tombstone;

        /**
         * Whether the given value is a tombstone value
         */
        static constexpr bool isTombstone(const T& val)
        {
            return false;
        }
    };

    namespace detail
    {
        /**
         * Optional type which uses a not-used bit-pattern of the original type to signal a "non-existing" value
         *
         * See also: https://github.com/foonathan/tiny
         */
        template <typename T, typename Traits = tombstone_traits<T>>
        class compact_optional
        {
            // This check is here, since destructors of tombstone value are otherwise too costly
            static_assert(std::is_trivially_destructible<T>::value, "");

        public:
            compact_optional() noexcept : storedValue(Traits::tombstone) {}
            compact_optional(const compact_optional& other) = default;
            compact_optional(compact_optional&& other) noexcept = default;
            compact_optional(const T& val) : storedValue(val) {}
            compact_optional(T&& val) noexcept : storedValue(std::move(val)) {}
            ~compact_optional() noexcept = default;

            // TODO need destructor which skips for EMPTY?? Or make sure EMPTY value is valid value?

            compact_optional& operator=(const compact_optional& other) = default;
            compact_optional& operator=(compact_optional&& other) noexcept = default;

            compact_optional& operator=(const T& val)
            {
                storedValue = val;
                return *this;
            }

            compact_optional& operator=(T&& val) noexcept
            {
                storedValue = std::move(val);
                return *this;
            }

            const T* operator->() const
            {
                return &storedValue;
            }

            T* operator->()
            {
                return &storedValue;
            }

            const T& operator*() const&
            {
                return storedValue;
            }

            T& operator*() &
            {
                return storedValue;
            }

            const T&& operator*() const&&
            {
                return std::move(storedValue);
            }

            T&& operator*() &&
            {
                return std::move(storedValue);
            }

            explicit operator bool() const noexcept
            {
                return !Traits::isTombstone(storedValue);
            }

            bool has_value() const
            {
                return !Traits::isTombstone(storedValue);
            }

            T& value() &
            {
                if(Traits::isTombstone(storedValue))
                    throw OptionalException{};
                return storedValue;
            }

            const T& value() const&
            {
                if(Traits::isTombstone(storedValue))
                    throw OptionalException{};
                return storedValue;
            }

            T&& value() &&
            {
                if(Traits::isTombstone(storedValue))
                    throw OptionalException{};
                return std::move(storedValue);
            }

            const T&& value() const&&
            {
                if(Traits::isTombstone(storedValue))
                    throw OptionalException{};
                return std::move(storedValue);
            }

        private:
            T storedValue;
        };

        template <typename T>
        bool operator==(const compact_optional<T>& o1, const compact_optional<T>& o2)
        {
            if(o1.has_value())
                return o2.has_value() && *o1 == *o2;
            return !o2.has_value();
        }

        template <typename T>
        bool operator!=(const compact_optional<T>& o1, const compact_optional<T>& o2)
        {
            return !(o1 == o2);
        }

        template <typename T>
        bool operator==(const compact_optional<T>& o1, const T& val)
        {
            return o1.has_value() && *o1 == val;
        }

        template <typename T>
        bool operator!=(const compact_optional<T>& o1, const T& val)
        {
            return !o1.has_value() || *o1 != val;
        }

        template <typename T>
        bool operator==(const T& val, const compact_optional<T>& o1)
        {
            return o1.has_value() && *o1 == val;
        }

        template <typename T>
        bool operator!=(const T& val, const compact_optional<T>& o1)
        {
            return !o1.has_value() || *o1 != val;
        }

        template <typename T>
        using optional_dispatch =
            typename std::conditional<tombstone_traits<T>::is_specialized, compact_optional<T>, OptionalBase<T>>::type;
    } /* namespace detail */

    /*
     * Implementation of an optional value.
     *
     * This class extends the functionality of std::(experimental::)optional
     */
    template <typename T>
    class Optional : public detail::optional_dispatch<T>
    {
        using Base = detail::optional_dispatch<T>;

    public:
        constexpr Optional() : Base() {}
        constexpr Optional(const T& value) : Base(value) {}
        constexpr Optional(T&& value) : Base(std::forward<T>(value)) {}
        constexpr Optional(const Optional<T>& other) : Base(other) {}
        constexpr Optional(Optional<T>&& other) noexcept : Base(std::forward<Base>(other)) {}
        ~Optional() noexcept = default;

        Optional& operator=(const Optional&) = default;
        Optional& operator=(Optional&&) noexcept = default;

        T value_or(const T& defaultValue) const
        {
            return has_value() ? **this : std::forward<const T>(defaultValue);
        }

        std::string to_string() const
        {
            return has_value() ? (*this)->to_string() : "-";
        }

        friend constexpr bool operator==(const Optional& o1, const Optional& o2) noexcept
        {
            return
                (!o1.has_value() && !o2.has_value()) ||
                (o1.has_value() && o2.has_value() && (*o1 == *o2));
        }

        friend constexpr bool operator==(const Optional& o, const T& val) noexcept
        {
            return o.has_value() && (*o == val);
        }

        friend constexpr bool operator==(const T& val, const Optional& o) noexcept
        {
            return o.has_value() && (*o == val);
        }

        friend constexpr bool operator!=(const Optional& o1, const Optional& o2) noexcept
        {
            return
                (o1.has_value() && !o2.has_value()) ||
                (!o1.has_value() && o2.has_value()) ||
                (o1.has_value() && o2.has_value() && (*o1 != *o2));
        }

        friend constexpr bool operator!=(const Optional& o, const T& val) noexcept
        {
            return !o.has_value() || (*o != val);
        }

        friend constexpr bool operator!=(const T& val, const Optional& o) noexcept
        {
            return !o.has_value() || (*o != val);
        }

        bool has_value() const noexcept
        {
            return static_cast<bool>(*this);
        }

        // These operators are defined as member operators to not interfere with any other operator, since any Type is
        // implicitly convertible to Optional<Type>
        [[deprecated]] friend bool operator&(const Optional& o, const std::function<bool(const T&)>& func)
        {
            return o.has_value() && func(*o);
        }

        template <typename Func>
        friend std::enable_if_t<std::is_member_function_pointer<Func>::value &&
                std::is_convertible<std::result_of_t<Func(const T&)>, bool>::value,
            bool>
        operator&(const Optional& o, Func&& func)
        {
            return o.has_value() && ((*o).*func)();
        }

        template <typename Func>
        friend std::enable_if_t<!std::is_member_function_pointer<Func>::value &&
                std::is_convertible<std::result_of_t<Func(const T&)>, bool>::value,
            bool>
        operator&(const Optional& o, Func&& func)
        {
            return o.has_value() && func(*o);
        }

        template <typename R>
        [[deprecated]] friend Optional<R> operator&(const Optional& o, const std::function<Optional<R>(const T&)>& func)
        {
            return o.has_value() ? func(*o) : Optional<R>{};
        }

        template <typename R>
        [[deprecated]] friend R* operator&(const Optional& o, const std::function<R*(const T&)>& func)
        {
            return o.has_value() ? func(*o) : nullptr;
        }

        template <typename Func>
        friend std::enable_if_t<std::is_pointer<std::result_of_t<Func(const T&)>>::value, std::result_of_t<Func(const T&)>>
        operator&(const Optional& o, Func&& func)
        {
            return o.has_value() ? func(*o) : nullptr;
        }

        template <typename S = T>
        friend typename std::enable_if<std::is_class<S>::value, bool>::type
        operator&(const Optional& o, bool (S::*func)() const)
        {
            return o.has_value() && ((*o).*func)();
        }

        template <typename R, typename S = T>
        friend typename std::enable_if<std::is_class<S>::value, Optional<R>>::type
        operator&(const Optional& o, Optional<R> (S::*func)() const)
        {
            return o.has_value() ? ((*o).*func)() : Optional<R>{};
        }

        template <typename R, typename S = T>
        friend typename std::enable_if<std::is_class<S>::value, R*>::type
        operator&(const Optional& o, R* (S::*func)() const)
        {
            return o.has_value() ? ((*o).*func)() : nullptr;
        }

        const Optional<T>& operator|(const Optional<T>& other) const&
        {
            return has_value() ? *this : other;
        }

        Optional<T> operator|(Optional<T>&& other) &&
        {
            return has_value() ? *this : other;
        }
    };

    template <typename R>
    Optional<R> make_optional(const R& value)
    {
        return Optional<R>(value);
    }

    /*
     * Helper basis type to assert a type not being copyable
     */
    struct NonCopyable
    {
        constexpr NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(NonCopyable&&) noexcept = default;
        NonCopyable& operator=(NonCopyable&&) noexcept = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
} /* namespace vc4c */

#endif /* VC4C_OPTIONAL_H */
