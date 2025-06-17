#pragma once

#include <source_location>

#ifndef NDEBUG
#define DEBUG_BREAKPOINT_DUMMY do{}while(false)
#else
#define DEBUG_BREAKPOINT_DUMMY
#endif

namespace debug
{
    __forceinline void WaitForDebugger()
    {
#ifndef NDEBUG
        if (!(GetKeyState(VK_CONTROL) & 0x8000))
            return;

        while (!IsDebuggerPresent())
            Sleep(10);

        Sleep(2000);
#endif
    }

#ifdef COMMONLIB_STUFF
    template<class T>
    std::string FormToString(T* form)
    {
        if (!form)
        {
            return fmt::format("{}: none", RE::FormTypeToString(T::FORMTYPE));
        }
        return fmt::format("{}: \"{}\"|{:08X}", RE::FormTypeToString(form->GetFormType()), form->GetName(), form->GetFormID());
    }
#endif

    /*
    template<>
    std::string FormToString<RE::ActiveEffect>(RE::ActiveEffect* form)
    {
        if (!form)
        {
            return fmt::format("ActiveMagicEffect: none");
        }
        return "ActiveMagicEffect: " + FormToString(form->GetBaseObject());
    }
    */

    /*
    template<class T>
    std::string MagicEffect(T* effect)
    {
        return effect ? fmt::format("{} attached to {} casted by {}", Form<EffectSetting>(effect->GetBaseObject()), effect->GetTargetActor(), effect->GetCasterActor()) : "null effect";
    }
    */

#ifdef NOPE
	template <class... Args>
	struct alert
	{
        alert() = delete;
		explicit alert(spdlog::format_string_t<Args...> format, Args&&... args,
	                   std::source_location location = std::source_location::current())
		{
			spdlog::log(
				spdlog::source_loc{
                    location.file_name(),
					static_cast<int>(location.line()),
                    location.function_name() },
				spdlog::level::warn,
                format,
				std::forward<Args>(args)...);

            std::string message = fmt::format(format, std::forward<Args>(args)...);
            std::string messageBoxMessage = fmt::format("{}({}): [warn] {}", location.file_name(), location.line(), std::move(message));
            MessageBoxA(nullptr, messageBoxMessage.c_str(), "Alert!", MB_OK | MB_ICONWARNING);
		}
	};
	template <class... Args>
	alert(spdlog::format_string_t<Args...>, Args&&...) -> alert<Args...>;
#endif

#ifdef COMMONLIB_STUFF
    class Exception : public std::exception
    {
    public:
        template <class... Args>
        Exception(const fmt::format_string<Args...> format, Args&&... args, std::source_location location = std::source_location::current())
        {
            std::string message = fmt::format(format, std::forward<Args>(args)...);
            SKSE::log::debug(std::string_view{ message }, location);
            message_ = fmt::format("{}({}): [debug] {}", location.file_name(), location.line(), message);
        }

        char const* what() const override { return message_.c_str(); }

    private:
        std::string message_;
    };
#endif

#ifdef COMMONLIB_STUFF
    class CreationKitException : public Exception
    {
    public:
        template <class... Args>
        CreationKitException(const fmt::format_string<Args...> format, Args&&... args, std::source_location location = std::source_location::current())
        {

        }
    };
#endif
}