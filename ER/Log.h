#pragma once

namespace Logger {
	enum LOGGER__INTERNAL_LOG_LEVEL {
		TRACE = 0,
		WARN = 1,
		FATAL = 2,
	};

	template <LOGGER__INTERNAL_LOG_LEVEL l>
	constexpr const char* const LOGGER__INTERNAL_LOG_LEVEL_STR()
	{
		static_assert(l >= TRACE && l <= FATAL);
		switch (l) {
		case 0:
			return "TRACE";
		case 1:
			return "WARN";
		case 2:
			return "FATAL";
		}
		__assume(0);
	}

	template <LOGGER__INTERNAL_LOG_LEVEL l>
	class LOGGER__INTERNAL_LOG {
	public:
		LOGGER__INTERNAL_LOG(const char* const file, const char* const func, int line, std::ostream& os) : os(os)
		{
			os << "log#" << LOGGER__INTERNAL_LOG_LEVEL_STR<l>() << " " << file << ":" << line << "@" << func << ": ";
		}
		template<typename T>
		LOGGER__INTERNAL_LOG& operator<<(const T& t)
		{
			os << t;
			return *this;
		}
		~LOGGER__INTERNAL_LOG()
		{
			os << std::flush;
		}
	private:
		std::ostream& os;
	};
}

#ifdef ELOG_ENABLED
#define LOG(level) Logger::LOGGER__INTERNAL_LOG<level>(__FILE__, __func__, __LINE__, std::cout)
#else
#define LOG(level) if (0) std::cerr // noop
#endif