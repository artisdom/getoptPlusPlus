 /* (C) 2011 Viktor Lofgren
  *
  *  This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */



#include "getoptpp.h"
#include <stdexcept>

namespace vlofgren {

/*
 *
 * Class OptionsParser
 *
 *
 */


OptionsParser::OptionsParser(const char* programDesc) : fprogramDesc(programDesc) {}
OptionsParser::~OptionsParser() {}

ParameterSet& OptionsParser::getParameters() {
	return parameters;
}

void OptionsParser::parse(int argc, const char* argv[]) throw(runtime_error)
{
	argv0 = argv[0];

	if(argc == 1) return;

	vector<string> v(&argv[1], &argv[argc]);

	ParserState state(*this, v);

	for(; !state.end(); state.advance()) {

		set<Parameter*>::iterator i;

		for(i = parameters.parameters.begin();
				i != parameters.parameters.end(); i++)
		{
			if((*i)->receive(state)) break;
		}

		if(i == parameters.parameters.end()) {
			string file = state.get();
			if(file == "--") {
				state.advance();
				break;
			}
			else if(file.at(0) == '-')
				throw Parameter::ParameterRejected(string("Bad parameter: ") + file);
			else files.push_back(state.get());
		}
	}

	if(!state.end()) for(; !state.end(); state.advance()) {
		files.push_back(state.get());
	}

}

void OptionsParser::usage() const {
	cerr << "Usage: " << programName() << " arguments" << endl;

	cerr << fprogramDesc << endl << endl;

	cerr << "Parameters: " << endl;

	set<Parameter*>::iterator i;
	for(i = parameters.parameters.begin();
			i != parameters.parameters.end(); i++)
	{
		cerr.width(30);
		cerr << left << "    " + (*i)->usageLine();

		cerr.width(40);
		cerr << left << (*i)->description() << endl;

	}

}

const vector<string>& OptionsParser::getFiles() const {
	return files;
}

const string& OptionsParser::programName() const {
	return argv0;
}

/*
 * Parameter set
 *
 *
 */

ParameterSet::ParameterSet(const ParameterSet& ps) {
	throw new runtime_error("ParameterSet not copyable");
}

ParameterSet::~ParameterSet() {
	for(set<Parameter*>::iterator i = parameters.begin();
			i != parameters.end(); i++)
	{
		delete *i;
	}

}

/* The typical use case for command line arguments makes linear searching completely
 * acceptable here.
 */

Parameter& ParameterSet::operator[](char c) const {
	for(set<Parameter*>::const_iterator i = parameters.begin(); i!= parameters.end(); i++) {
		if((*i)->shortOption() == c) return *(*i);
	}
	throw out_of_range("ParameterSet["+c+string("]"));
}


Parameter& ParameterSet::operator[](const string& param) const {
	for(set<Parameter*>::const_iterator i = parameters.begin(); i!= parameters.end(); i++) {
		if((*i)->longOption() == param) return *(*i);
	}
	throw out_of_range("ParameterSet["+param+"]");
}



/*
 *
 * Class ParserState
 *
 *
 */


ParserState::ParserState(OptionsParser &opts, vector<string>& args) :
	opts(opts), arguments(args), iterator(args.begin())
{
	
}

const string ParserState::peek() const {
	vector<string>::const_iterator next = iterator+1;
	if(next != arguments.end()) return *next;
	else return "";
	
}

const string ParserState::get() const {
	if(!end()) return *iterator;
	else return "";
}

void ParserState::advance() {
	iterator++;
}

bool ParserState::end() const {
	return iterator == arguments.end();
}


/*
 *
 * Class Parameter
 *
 *
 */



Parameter::Parameter(char shortOption, const char *longOption, const char *description) :
	fshortOption(shortOption), flongOption(longOption), fdescription(description)
{
	
}

Parameter::~Parameter() {}

const string& Parameter::description() const { return fdescription; }
const string& Parameter::longOption() const { return flongOption; }
char Parameter::shortOption() const { return fshortOption; }

/*
 *
 * Class Switchable
 *
 *
 */

bool Switchable::isSet() const { return fset; }
Switchable::~Switchable() {};
Switchable::Switchable() : fset(false) {}

void MultiSwitchable::set() throw (Switchable::SwitchingError) { fset = true; }
MultiSwitchable::~MultiSwitchable() {}


void UniquelySwitchable::set() throw (Switchable::SwitchingError) {
	if(UniquelySwitchable::isSet()) throw Switchable::SwitchingError();
	fset = true;
}
UniquelySwitchable::~UniquelySwitchable() {}


PresettableUniquelySwitchable::~PresettableUniquelySwitchable() {}
bool PresettableUniquelySwitchable::isSet() const {
	return UniquelySwitchable::isSet() || fpreset.isSet();
}
void PresettableUniquelySwitchable::set() throw (Switchable::SwitchingError)
{
	UniquelySwitchable::set();
}
void PresettableUniquelySwitchable::preset() {
	fpreset.set();
}

/*
 *
 * Class SwitchParameter
 *
 *
 */


SwitchParameter::SwitchParameter(char shortOption, const char *longOption,
			const char* description) : CommonParameter<MultiSwitchable>(shortOption, longOption, description) {}
SwitchParameter::~SwitchParameter() {}

void SwitchParameter::receiveSwitch() throw(Parameter::ParameterRejected) {
	set();
}

void SwitchParameter::receiveArgument(const string &arg) throw(Parameter::ParameterRejected) {
	throw UnexpectedArgument();
}

/*
 *
 * PODParameter specializations
 *
 *
 *
 */


template<>
PODParameter<string>::PODParameter(char shortOption, const char *longOption,
		const char* description) : CommonParameter<PresettableUniquelySwitchable>(shortOption, longOption, description) {
	preset();
}


template<>
int PODParameter<int>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	// This is sadly necessary for strto*-functions to operate on
	// const char*. The function doesn't write to the memory, though,
	// so it's quite safe.

	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	long l = strtol(cstr, &cstr, 10);
	if(*cstr != '\0') throw ParameterRejected("Expected int");

	if(l > INT_MAX || l < INT_MIN) {
		throw ParameterRejected("Expected int");
	}

	return l;
}

template<>
long PODParameter<long>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	long l = strtol(cstr, &cstr, 10);
	if(*cstr != '\0') throw ParameterRejected("Expected long");

	return l;
}

template<>
double PODParameter<double>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	double d = strtod(cstr, &cstr);
	if(*cstr != '\0') throw ParameterRejected("Expected double");

	return d;
}

template<>
string PODParameter<string>::validate(const string &s) throw(Parameter::ParameterRejected)
{
	return s;
}


} //namespace
