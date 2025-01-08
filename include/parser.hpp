#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <type_traits>
#include <cassert>

namespace cmd_line_parser {

struct parser {
    parser(int argc, char** argv)
        : m_argc(argc)
        , m_argv(argv)
        , m_required(0) {}

    struct cmd {
        std::string shorthand, value, descr;
        bool is_required, is_boolean;
    };

    bool parse() {
        assert(m_argc > 0);

        bool have_version = false;
        int num_required = 0;
        std::unordered_set<std::string> parsed_shorthands;
        parsed_shorthands.reserve(m_argc);

        for (int i = 1; i < m_argc; ++i) {
            std::string parsed(m_argv[i]);
            // shortcuts, bypassing required checks
            if (parsed == "-h" || parsed == "--help")
                return abort(false);
            else if (parsed == "--version")
                have_version = true;
            else {
                if (m_argc - 1 < m_required) return abort();
            }
            int id = 0;
            if (const auto it = m_shorthands.find(parsed); it == m_shorthands.end()) {
                std::cerr << "== error: shorthand '" + parsed + "' not found" << std::endl;
                return abort();
            } else {
                if (const auto it = parsed_shorthands.find(parsed); it != parsed_shorthands.end()) {
                    std::cerr << "== error: shorthand '" + parsed + "' already parsed" << std::endl;
                    return abort();
                }
                parsed_shorthands.emplace(parsed);
                id = (*it).second;
            }
            assert(static_cast<size_t>(id) < m_names.size());
            auto const& name = m_names[id];
            auto& cmd = m_cmds[name];
            if (cmd.is_required) num_required += 1;
            if (cmd.is_boolean) {
                parsed = "true";
            } else {
                ++i;
                if (i == m_argc) { return abort(); }
                parsed = m_argv[i];
            }
            cmd.value = parsed;
        }

        if (!have_version && num_required != m_required) return abort();

        return true;
    }

    void help(const bool err=true) const {
        std::ostream *os = err ? &std::cerr : &std::cout;
        *os << "Usage: " << m_argv[0] << "\t-h,--help";
        const auto print = [this,os](bool with_description) {
            for (size_t i = 0; i != m_names.size(); ++i) {
                auto const& cmd = m_cmds.at(m_names[i]);
                if (!with_description)
                    *os << " ";
                std::cerr << cmd.shorthand;
                if (!cmd.is_boolean) *os << " " << m_names[i];
                if (with_description) {
                    *os << "\n\t" << (cmd.is_required ? "REQUIRED: " : "") << cmd.descr
                        << "\n\n";
                }
            }
        };
        print(false);
        *os << "\n\nOPTIONS:\n\n";
        print(true);
        *os << "-h,--help\n\tPrint this help text and silently exits." << std::endl;
    }

    bool add(std::string const& name, std::string const& descr, std::string const& shorthand,
             bool is_required, bool is_boolean = false) {
        bool ret = m_cmds
                       .emplace(name, cmd{shorthand, is_boolean ? "false" : "", descr, is_required,
                                          is_boolean})
                       .second;
        if (ret) {
            if (is_required) m_required += 1;
            m_names.push_back(name);
            m_shorthands.emplace(shorthand, m_names.size() - 1);
        }
        return ret;
    }

    template <typename T>
    T get(std::string const& name) const {
        auto it = m_cmds.find(name);
        if (it == m_cmds.end()) throw std::runtime_error("error: '" + name + "' not found");
        auto const& value = (*it).second.value;
        return parse<T>(value);
    }

    bool parsed(std::string const& name) const {
        auto it = m_cmds.find(name);
        if (it == m_cmds.end()) return false;
        auto const& cmd = (*it).second;
        if (cmd.is_boolean) {
            if (cmd.value == "true") return true;
            if (cmd.value == "false") return false;
            assert(false);  // should never happen
        }
        return cmd.value != "";
    }

    template <typename T>
    T parse(std::string const& value) const {
        if constexpr (std::is_same<T, std::string>::value) {
            return value;
        } else if constexpr (std::is_same<T, char>::value || std::is_same<T, signed char>::value ||
                             std::is_same<T, unsigned char>::value) {
            return value.front();
        } else if constexpr (std::is_same<T, int>::value || std::is_same<T, short int>::value) {
            return std::strtol(value.c_str(), nullptr, 10);
        } else if constexpr (std::is_same<T, unsigned int>::value ||
                             std::is_same<T, unsigned short int>::value) {
            return std::strtoul(value.c_str(), nullptr, 10);
        } else if constexpr (std::is_same<T, long int>::value ||
                             std::is_same<T, long long int>::value) {
            return std::strtoll(value.c_str(), nullptr, 10);
        } else if constexpr (std::is_same<T, unsigned long int>::value ||
                             std::is_same<T, unsigned long long int>::value) {
            return std::strtoull(value.c_str(), nullptr, 10);
        } else if constexpr (std::is_same<T, float>::value || std::is_same<T, double>::value ||
                             std::is_same<T, long double>::value) {
            return std::strtod(value.c_str(), nullptr);
        } else if constexpr (std::is_same<T, bool>::value) {
            std::istringstream stream(value);
            bool ret;
            if (value == "true" || value == "false") {
                stream >> std::boolalpha >> ret;
            } else {
                stream >> std::noboolalpha >> ret;
            }
            return ret;
        }
        assert(false);  // should never happen
        throw std::runtime_error("unsupported type");
    }

private:
    int m_argc;
    char** m_argv;
    int m_required;
    std::unordered_map<std::string, cmd> m_cmds;
    std::unordered_map<std::string, int> m_shorthands;
    std::vector<std::string> m_names;

    bool abort(const bool err=true) const {
        help(err);
        return false;
    }
};

}  // namespace cmd_line_parser
