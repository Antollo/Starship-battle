#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

#include <map>
#include <functional>
#include <sstream>
#include <locale>
#include <codecvt>
#include <memory>
#include "Console.h"

using ArgsText = std::wstring;
using ArgsStream = std::basic_stringstream<ArgsText::value_type>;

class CallableBase
{
public:
    virtual void operator()(ArgsStream& argsText) const = 0;
};

template<class... Args>
class Callable : public CallableBase
{
public:
    Callable(std::function<void(Args...)>&& callable) : c(callable) { }
    void operator()(ArgsStream& argsStream) const override
    {
        call(argsStream, std::index_sequence_for<Args...>());
    }
    template<std::size_t... Is>
    void call(ArgsStream& argsStream, std::index_sequence<Is...>) const
    {
        std::tuple<Args...> t;
        // Trick from: https://stackoverflow.com/questions/6245735/pretty-print-stdtuple
        using swallow = int[];
        (void)swallow{0, (void(argsStream >> std::get<Is>(t)), 0)...};
        // End of trick
        c((std::get<Is>(t))...);
    }
private:
    std::function<void(Args...)> c;
};

template<class T, class... Args>
auto makeCallableHelper(T callable, void(T::*)(Args...))
{
    return Callable<Args...>(std::function<void(Args...)>(callable));
}

template<class T, class... Args>
auto makeCallableHelper(T callable, void(T::*)(Args...) const)
{
    return Callable<Args...>(std::function<void(Args...)>(callable));
}

template<class T, class... Args>
auto makeCallablePtrHelper(T callable, void(T::*)(Args...))
{
    return newCallable<Args...>(std::function<void(Args...)>(callable));
}

template<class T, class... Args>
auto makeCallablePtrHelper(T callable, void(T::*)(Args...) const)
{
    return new Callable<Args...>(std::function<void(Args...)>(callable));
}

template<class T>
auto makeCallable(T&& callable)
{
    return makeCallableHelper(callable, &std::remove_reference_t<T>::operator());
}
    
template<class T>
auto makeCallablePtr(T&& callable)
{
    return makeCallablePtrHelper(callable, &std::remove_reference_t<T>::operator());
}

class CommandProcessor
{
public:
    template <class T>
    void bind(const std::wstring& key, T&& callback)
    {
        map[key] = std::unique_ptr<CallableBase>(makeCallablePtr(callback));
    }
    void call(const std::wstring& key, std::wstringstream& args)
    {
        if (map.count(key)) map[key]->operator()(args);
    }
    void call(const std::wstring& str)
    {
        std::wstring key;
        inputsStream.clear();
        inputsStream.str(str);
        inputsStream >> key;
        try
        {
            call(key, inputsStream);
        }
        catch(const std::exception& e)
        {
            (*Object::console) << key << L"-error: " << converter.from_bytes(e.what()) << L"\n"
            << L"Use \"help command " << key << L"\" to get help.\n\n";
        }
    }
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
private:
    std::map<std::wstring, std::unique_ptr<CallableBase>> map;
    std::wstringstream inputsStream;
};

#endif
