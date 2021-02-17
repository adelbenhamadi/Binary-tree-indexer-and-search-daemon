#include "options.h"

namespace mynodes{
	namespace options{

	// adds an option to the list of options
void ProgramOptions::addOption(Option const& option) {
 
  std::map<std::string, Section>::iterator sectionIt = addSection(option.section, "");

  if (!option.shorthand.empty()) {
	if (!_shorthands.try_emplace(option.shorthand, option.fullName()).second) {
	  throw std::logic_error(
		  std::string("shorthand option already defined for option ") + option.displayName());
	}
  }

  Section& section = (*sectionIt).second;
  section.options.try_emplace(option.name, option);
}
}
}