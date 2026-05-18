#pragma once

#include <iostream>
#include <tuple>
#include <concepts>
#include <utility>
#include <typeinfo>

namespace RR
{

    // Matches any type that can be added to std::ostream via << operator
    // Used to constrair the print function to prevent a wall of templated instantiation errors
    template <typename T>
    concept Streamable = requires(std::ostream& os, const T& v) {
        { os << v } -> std::convertible_to<std::ostream&>;
    };

    namespace detail
    {
        // ANSI escape codes for colored terminal output
        constexpr const char* cReset  = "\033[0m";
        constexpr const char* cCyan   = "\033[36m";
        constexpr const char* cYellow = "\033[33m";
        constexpr const char* cRed    = "\033[31m";
    }

    // ------------------------------------------------------------------------
    // Variadic print/log helper
    // ------------------------------------------------------------------------
    template <typename... Payload>
    class Info
    {
    public:
        // Constructor to store arguments in a tuple
        template <typename... Args>
        explicit Info(Args&&... args)
            : localArgs(std::forward<Args>(args)...) {}

        void print() const requires (Streamable<Payload> && ...)
        {
            emitTo(std::cout, detail::cCyan, "[LOG]");
        }

        void warn() const requires (Streamable<Payload> && ...)
        {
            emitTo(std::cerr, detail::cYellow, "[WARN]");
        }

        void error() const requires (Streamable<Payload> && ...)
        {
            emitTo(std::cerr, detail::cRed, "[ERROR]");
        }

        // Prints type name and size for each stored argument
        void info() const
        {
            std::apply([](const auto&... args) {
                ((std::cout << detail::cCyan
                            << "[INFO] Type: "  << typeid(args).name()
                            << ", size: " << sizeof(args) << " bytes"
                            << detail::cReset << '\n'), ...);
                std::cout << std::flush;
            }, localArgs);
        }

    private:
        // Shared emit path for print/warn/error
        void emitTo(std::ostream& out, const char* color, const char* prefix) const
            requires (Streamable<Payload> && ...)
        {
            std::apply([&](const auto&... args) {
                if (color)  out << color;
                if (prefix) out << prefix << ' ';
                ((out << args), ...);
                if (color)  out << detail::cReset;
                out << '\n' << std::flush;
            }, localArgs);
        }

        std::tuple<Payload...> localArgs;
    };

    template <typename... Args>
    Info(Args&&...) -> Info<std::decay_t<Args>...>;

    // ------------------------------------------------------------------------
    // Global functions
    // ------------------------------------------------------------------------

    template <Streamable... Args>
    inline auto print(Args&&... args)
    {
        Info<std::decay_t<Args>...> i(std::forward<Args>(args)...);
        i.print();
        return i;
    }

    template <Streamable... Args>
    inline auto warn(Args&&... args)
    {
        Info<std::decay_t<Args>...> i(std::forward<Args>(args)...);
        i.warn();
        return i;
    }

    template <Streamable... Args>
    inline auto error(Args&&... args)
    {
        Info<std::decay_t<Args>...> i(std::forward<Args>(args)...);
        i.error();
        return i;
    }

    // Standalone type/size dump that works on any complete type
    template <typename... Args>
    inline void getInfo(const Args&... /*deduced, not used*/)
    {
        ((std::cout << detail::cCyan
                    << "[INFO] Type: "  << typeid(Args).name()
                    << ", size: " << sizeof(Args) << " bytes"
                    << detail::cReset << '\n'), ...);
        std::cout << std::flush;
    }

    // BECAUSE WE LIKE COOLNESS!
    inline void printLogo()
    {
        constexpr const char* kCyan  = "\033[36m";
        constexpr const char* kReset = "\033[0m";

        std::cout << kCyan << R"(
    __________                   .___          __________
    \______   \ ____   ____    __| _/__________\______   \_____    ____  ____  ____   ____   ____
     |       _// __ \ /    \  / __ |/ __ \_  __ \       _/\__  \ _/ ___\/ ___\/  _ \ /  _ \ /    \
     |    |   \  ___/|   |  \/ /_/ \  ___/|  | \/    |   \ / __ \\  \__\  \__(  <_> |  <_> )   |  \
     |____|_  /\___  >___|  /\____ |\___  >__|  |____|_  /(____  /\___  >___  >____/ \____/|___|  /
            \/     \/     \/      \/    \/             \/      \/     \/    \/                  \/

    )" << kReset << std::endl;
    }
}