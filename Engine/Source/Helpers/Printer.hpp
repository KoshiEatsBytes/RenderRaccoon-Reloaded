#pragma once

#include <iostream>
#include <tuple>
#include <concepts>
#include <utility>
#include <typeinfo>

namespace RR
{

    // Matches any type that can be added to std::ostream via << operator
    // Used to constrain the log function to prevent a wall of templated instantiation errors
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
        constexpr const char* cGreen  = "\033[32m";
    }

    // ------------------------------------------------------------------------
    // Variadic log helper
    // ------------------------------------------------------------------------
    template <typename... Payload>
    class Info
    {
    public:
        // Constructor to store arguments in a tuple
        template <typename... Args>
        explicit Info(Args&&... args)
            : localArgs(std::forward<Args>(args)...) {}

        void success() const requires (Streamable<Payload> && ...)
        {
            emitTo(std::cout, detail::cGreen, "[SUCCESS]");
        }

        void log() const requires (Streamable<Payload> && ...)
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
        // Shared emit path for log/warn/error
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
    inline auto success(Args&&... args)
    {
        Info<std::decay_t<Args>...> i(std::forward<Args>(args)...);
        i.success();
        return i;
    }

    template <Streamable... Args>
    inline auto log(Args&&... args)
    {
        Info<std::decay_t<Args>...> i(std::forward<Args>(args)...);
        i.log();
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
        std::cout << detail::cCyan << R"(
    __________                   .___          __________
    \______   \ ____   ____    __| _/__________\______   \_____    ____  ____  ____   ____   ____
     |       _// __ \ /    \  / __ |/ __ \_  __ \       _/\__  \ _/ ___\/ ___\/  _ \ /  _ \ /    \
     |    |   \  ___/|   |  \/ /_/ \  ___/|  | \/    |   \ / __ \\  \__\  \__(  <_> |  <_> )   |  \
     |____|_  /\___  >___|  /\____ |\___  >__|  |____|_  /(____  /\___  >___  >____/ \____/|___|  /
            \/     \/     \/      \/    \/             \/      \/     \/    \/                  \/

    )" << detail::cReset << std::endl;
    }
}