#ifndef HPP_ARGPARSER
#define HPP_ARGPARSER

#include <type_traits>
#include <stdexcept>
#include <vector>
#include <map>
#include <memory>
#include "lexical_cast.hpp"

class ArgParser {
private:
    class IArgument {
    public:
        virtual ~IArgument() {}
        virtual void parse(const std::string & s) = 0;
    };

    template <typename T>
    class Argument : public IArgument {
    public:
        Argument(T & t) : m_t(t) {}
        ~Argument() {}

        void parse(const std::string & s) override {
            m_t = lexical_cast::read<T>(s);
        }

    private:
        T & m_t;
    };

    template <typename T>
    class MultiArgument : public IArgument {
    public:
        MultiArgument(std::vector<T> & t) : m_t(t) {}
        ~MultiArgument() {}

        void parse(const std::string & s) override {
            m_t.push_back(lexical_cast::read<T>(s));
        }

    private:
        std::vector<T> & m_t;
    };

public:
    ArgParser(int argc, char * * argv) {
        for (int i = 1; i < argc; ++i) {
            m_args.push_back(argv[i]);
        }
    }

    ~ArgParser() = default;

    template <typename T>
    void addSwitch(const char switchShort, const std::string switchLong, T & t) {
        const std::string strShort = std::string("-") + switchShort;
        const std::string strLong = std::string("--") + switchLong;

        auto pArgShort = std::make_shared<Argument<T>>(t);
        auto pArgLong = std::make_shared<Argument<T>>(t);
        m_mapArgs[strShort] = std::make_pair(std::is_same<bool, T>::value, pArgShort);
        m_mapArgs[strLong] = std::make_pair(std::is_same<bool, T>::value, pArgLong);
    }

    template <typename T>
    void addMultiSwitch(const char switchShort, const std::string switchLong, std::vector<T> & t) {
        const std::string strShort = std::string("-") + switchShort;
        const std::string strLong = std::string("--") + switchLong;

        auto pArgShort = std::make_shared<MultiArgument<T>>(t);
        auto pArgLong = std::make_shared<MultiArgument<T>>(t);
        m_mapArgs[strShort] = std::make_pair(false, pArgShort);
        m_mapArgs[strLong] = std::make_pair(false, pArgLong);
    }

    bool parse() const {
        try {
            std::vector<std::string>::size_type i = 0;

            while (i < m_args.size()) {
                const auto& p = m_mapArgs.at(m_args[i]);
                const std::string s = p.first ? "1" : m_args.at(i + 1);

                p.second->parse(s);
                i += (p.first ? 1 : 2);
            }
        } catch (const std::out_of_range & e) {
            return false;
        }

        return true;
    }

private:
    std::vector<std::string> m_args;
    std::map<std::string, std::pair<bool, std::shared_ptr<IArgument>>> m_mapArgs;
};

#endif /* HPP_ARGPARSER */
