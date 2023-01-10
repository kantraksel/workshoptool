#pragma once
#include <format>

void Log(const std::string& str = std::string());

template <class... Args>
inline void Log(const std::_Fmt_string<Args...> _Fmt, Args&&... _Args)
{
	Log(std::vformat(_Fmt._Str, std::make_format_args(_Args...)));
}
