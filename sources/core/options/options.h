#pragma once

#include "parameter.h"

#include <string>
#include <iostream>
#include <map>
#include <unordered_map>


namespace mynodes {

    namespace options {

        namespace tools {
            // strip the "--" from a string
            std::string stripPrefix(std::string const& name) {
                size_t pos = name.find("--");
                if (pos == 0) {
                    // strip initial "--"
                    return name.substr(2);
                }
                return name;
            }

            // strip the "-" from a string
            std::string stripShorthand(std::string const& name) {
                size_t pos = name.find('-');
                if (pos == 0) {
                    // strip initial "-"
                    return name.substr(1);
                }
                return name;
            }

            // split an option name at the ".", if it exists
            std::pair<std::string, std::string> splitName(std::string name) {
                std::string section;
                name = stripPrefix(name);
                // split at "."
                size_t pos = name.find('.');
                if (pos == std::string::npos) {
                    // global option
                    section = "";
                }
                else {
                    // section-specific option
                    section = name.substr(0, pos);
                    name = name.substr(pos + 1);
                }

                return std::make_pair(section, name);
            }

            std::vector<std::string> wordwrap(std::string const& value, size_t size) {
                std::vector<std::string> result;
                std::string next = value;

                if (size > 0) {
                    while (next.size() > size) {
                        size_t m = next.find_last_of("., ", size - 1);

                        if (m == std::string::npos || m < size / 2) {
                            m = size;
                        }
                        else {
                            m += 1;
                        }

                        result.emplace_back(next.substr(0, m));
                        next = next.substr(m);
                    }
                }

                result.emplace_back(next);

                return result;
            }

            // right-pad a string
            std::string pad(std::string const& value, size_t length) {
                size_t const valueLength = value.size();
                if (valueLength > length) {
                    return value.substr(0, length);
                }
                if (valueLength == length) {
                    return value;
                }
                return value + std::string(length - valueLength, ' ');
            }

            std::string trim(std::string const& value) {
                size_t const pos = value.find_first_not_of(" \t\n\r");
                if (pos == std::string::npos) {
                    return "";
                }
                return value.substr(pos);
            }

        }

        struct Option {
            Option(const std::string& op, const std::string& des, Parameter* param)
                : section(), name(), description(des), shorthand(), parameter(param) {
                auto parts = tools::splitName(op);
                section = parts.first;
                name = parts.second;

            }
            std::string displayName() const { return "--" + verboseName(); }
            std::string verboseName() const {
                return section.empty() ? name : section + '.' + name;
            }

            std::string section;
            std::string name;
            std::string description;
            std::string shorthand;
            std::shared_ptr<Parameter> parameter;
        };
        struct Section {

            Section(std::string const& name, std::string const& description,
                std::string const& alias, bool hidden, bool obsolete)
                : name(name),
                description(description),
                alias(alias),
                hidden(hidden),
                obsolete(obsolete) {}

            // get display name for the section
            std::string displayName() const { return alias.empty() ? name : alias; }
            Option& getOption(std::string const& n) {
                auto it = options.find(n);
                if (it == options.end()) {
                    throw std::logic_error(std::string("option '") + n + "' not found");
                }
                return it->second;
            }


            std::string name;
            std::string description;
            std::string alias;
            bool hidden;
            bool obsolete;


            // map options for this section
            std::map<std::string, Option> options;
        };

        class ProgramOptions {
        public:
            ProgramOptions(ProgramOptions const&) = delete;
            ProgramOptions& operator=(ProgramOptions const&) = delete;

            auto addSection(Section const& section) {

                auto emplacedPair = _sections.try_emplace(section.name, section);
                if (!emplacedPair.second) {
                    // section already present. check if we need to update it
                    Section& section = emplacedPair.first->second;
                    if (!section.description.empty() && section.description.empty()) {
                        // copy over description
                        section.description = section.description;
                    }
                }
                return emplacedPair.first;
            }


            // adds a new option to the program options
            Option& addOption(std::string const& name, std::string const& description, Parameter* parameter) {
                addOption(Option(name, description, parameter));
                return getOption(name);
            }
            Option& getOption(std::string const& n) {
                auto parts = tools::splitName(n);
                auto it = _sections.find(parts.first);
                if (it == _sections.end()) {
                    throw std::logic_error(std::string("Section " + parts.first + " not found!"));
                }
                return it->second.getOption(parts.second);

            }
        private:
            void addOption(Option const& option);

        private:
            // name of binary (i.e. argv[0])
            std::string _progname;
            // usage hint, e.g. "usage: #progname# [<options>] ..."
            std::string _usage;

            // context string that's shown when errors are printed
            std::string _context;

            std::map<std::string, Section> _sections;
            // shorthands for options, translating from short options to long option names
            // e.g. "-c" to "--configuration"
            std::unordered_map<std::string, std::string> _shorthands;
            // map with old options and their new equivalents, used for printing more
            // meaningful error messages when an invalid (but once valid) option was used
            std::unordered_map<std::string, std::string> _oldOptions;

            // whether or not the program options setup is still mutable
            bool _sealed;
            // allow or disallow overriding already set options
            bool _overrideOptions;
            // directory of this binary
            char const* _binaryPath;

        };

    }
}