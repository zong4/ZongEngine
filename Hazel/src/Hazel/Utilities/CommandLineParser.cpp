#include "hzpch.h"
#include "CommandLineParser.h"

namespace Hazel {
	CommandLineParser::CommandLineParser(int argc, char** argv, bool allow_ms) {
		m_Opts.reserve(argc - 1);
		bool lastWasNamed = false;

		for(int i = 1; i < argc; ++i) {
			Opt newOpt;
			bool longopt = false;
			char* opt = argv[i];
			char* colon;

			if(opt[0] == '-') {
				++opt;
				opt += (longopt = (opt[0] == '-'));
				if(longopt && (colon = strrchr(opt, '='))) {
					*colon = 0;
					newOpt.param = colon + 1;
				}
				else lastWasNamed = true;
			}
			else if(allow_ms && opt[0] == '/') {
				newOpt.ms = true;
				opt += (longopt = true);
				if((colon = strrchr(opt, ':'))) {
					*colon = 0;
					newOpt.param = colon + 1;
				}
			}
			else {
				if(lastWasNamed) {
					m_Opts.back().param = opt;
					lastWasNamed = false;
				}
				else newOpt.raw = true;
			}

			newOpt.name = opt;

			m_Opts.emplace_back(newOpt);
		}
	}

	std::vector<std::string_view> CommandLineParser::GetRawArgs() {
		std::vector<std::string_view> result;

		for(auto& opt : m_Opts) {
			if(opt.raw) result.emplace_back(opt.name);
		}

		return std::move(result);
	}

	std::string_view CommandLineParser::GetOpt(const std::string& name) {
		// TODO(Emily): Respect and translate MS casing rules
		//  			i.e. `/Foo' as opposed to `/foo'.
		for(auto& opt : m_Opts) {
			if(opt.name == name && !opt.raw) return opt.param;
		}

		return "";
	}

	bool CommandLineParser::HaveOpt(const std::string& name) {
		return !GetOpt(name).empty();
	}
}
