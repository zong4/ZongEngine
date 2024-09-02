#pragma once

namespace Hazel {
	/*
	 * NOTE(Emily): At the moment this doesn't support arbitrary-position
	 * 				Parameterless named options
	 * 				(i.e. `git push -f origin' would treat origin as a
	 * 				Parameter to `-f')
	 * 				As it would require an option specifier for the options
	 * 				Whilst retaining raw argument support
	 */
	class CommandLineParser {
	public:
		/// `allow_ms' accepts `/Foo` in addition to `--foo`
		CommandLineParser(int argc, char** argv, bool allow_ms = true);

		std::vector<std::string_view> GetRawArgs();
		/// Gets the value passed to a named option
		/// e.g. `-C foo': `GetOpt("C")' -> `"foo"'
		/// `opt' is taken in the form *without* the leading `-' or `/'
		std::string_view GetOpt(const std::string& name);
		bool HaveOpt(const std::string& name);

	private:
		struct Opt {
			bool raw;
			bool ms;
			std::string_view name;
			std::string_view param;
		};
		std::vector<Opt> m_Opts;
	};
}
