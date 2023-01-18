#pragma once
#include <string>

void Log(const std::string& str = std::string());

#ifdef WIN32
	#include <format>

	template <class... Args>
	inline void Log(const std::_Fmt_string<Args...> _Fmt, Args&&... _Args)
	{
		Log(std::vformat(_Fmt._Str, std::make_format_args(_Args...)));
	}

	template <class... Args>
	inline std::string Format(const std::_Fmt_string<Args...> _Fmt, Args&&... _Args)
	{
		return std::vformat(_Fmt._Str, std::make_format_args(_Args...));
	}
#elif __GNUC__ >= 13
	#include <format>

	template <class... Args>
	inline void Log(const std::format_string<Args...> _Fmt, Args&&... _Args)
	{
		Log(std::vformat(_Fmt.get(), std::make_format_args(_Args...)));
	}

	template <class... Args>
	inline std::string Format(const std::format_string<Args...> _Fmt, Args&&... _Args)
	{
		return std::vformat(_Fmt.get(), std::make_format_args(_Args...));
	}
#else
	#include <fmt/core.h>

	template <class... Args>
	inline void Log(const fmt::format_string<Args...> _Fmt, Args&&... _Args)
	{
		Log(fmt::vformat(_Fmt, fmt::make_format_args(_Args...)));
	}

	template <class... Args>
	inline std::string Format(const fmt::format_string<Args...> _Fmt, Args&&... _Args)
	{
		return fmt::vformat(_Fmt, fmt::make_format_args(_Args...));
	}
#endif
