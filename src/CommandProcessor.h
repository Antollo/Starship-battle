#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

#include <map>
#include <functional>
#include <sstream>
#include <locale>
#include <codecvt>
#include <memory>
#include "Console.h"

//using ArgsText = std::wstring;
//using ArgsStream = std::basic_stringstream<ArgsText::value_type>;

class CallableBase
{
public:
    virtual std::wstring operator()(std::wstringstream& argsText) const = 0;
};

template<class... Args>
class Callable : public CallableBase
{
public:
    Callable(std::function<std::wstring(Args...)>&& newCallable) : callable(newCallable) { }
    std::wstring operator()(std::wstringstream& argsStream) const override
    {
        return call(argsStream, std::index_sequence_for<Args...>());
    }
    template<std::size_t... Is>
    std::wstring call(std::wstringstream& argsStream, std::index_sequence<Is...>) const
    {
        std::tuple<Args...> t;
        // Trick from: https://stackoverflow.com/questions/6245735/pretty-print-stdtuple
        using swallow = int[];
        (void)swallow{0, (void(argsStream >> std::get<Is>(t)), 0)...};
        // End of trick
        return callable((std::get<Is>(t))...);
    }
private:
    std::function<std::wstring(Args...)> callable;
};

/*template<class T, class... Args>
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
}*/

class CommandProcessor
{
public:
    template <class T>
    void bind(const std::wstring& key, T&& callback)
    {
        map[key] = std::unique_ptr<CallableBase>(new Callable(std::function(callback)));
    }
    std::wstring call(const std::wstring& key, std::wstringstream& args)
    {
        if (map.count(key)) 
            return map[key]->operator()(args);
        return L"print " + key + L"-error: Command not found.\n"
        + L"Use 'help' to get help.\n";        ;
    }
    std::wstring call(const std::wstring& str)
    {
        std::wstring key;
        inputsStream.clear();
        inputsStream.str(str);
        inputsStream >> key;
        try
        {
            return call(key, inputsStream);
        }
        catch(const std::exception& e)
        {
            return L"print " + key + L"-error: " + converter.from_bytes(e.what()) + L"\n"
            + L"Use 'help-" + key + L"' to get help.\n";
        }
    }
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
private:
    std::map<std::wstring, std::unique_ptr<CallableBase>> map;
    std::wstringstream inputsStream;
};

#endif
