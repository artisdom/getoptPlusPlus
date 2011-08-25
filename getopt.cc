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



#include "getopt.h"

#include <iostream>

OptionsParser::OptionsParser() {}
OptionsParser::~OptionsParser() {}

void OptionsParser::addParameter(Parameter * const param) {
	parameters.insert(param);
}

void OptionsParser::parse(int argc, const char* argv[]) throw(runtime_error)
{
	argv0 = argv[0];

	if(argc == 1) return;

	vector<string> v(&argv[1], &argv[argc]);

	ParserState state(*this, v);

	for(; !state.end(); state.advance()) {

		set<Parameter*>::iterator i;

		for(i = parameters.begin();
				i != parameters.end(); i++)
		{
			if((*i)->receive(state)) break;
		}

		if(i == parameters.end()) {
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

void OptionsParser::usage(const string& description) const { // XXX:STUB
	cerr << "Usage: " << programName() << " arguments" << endl;

	cerr << description << endl << endl;

	cerr << "Parameters: " << endl;

	set<Parameter*>::iterator i;
	for(i = parameters.begin();
			i != parameters.end(); i++)
	{
		cerr << "\t" <<(*i)->usageLine() + "\t" + (*i)->description() << endl;

	}

}

const vector<string>& OptionsParser::getFiles() const {
	return files;
}

string OptionsParser::programName() const {
	return argv0;
}



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


Parameter::Parameter(char shortOption, const char *longOption, const char *description) :
	fshortOption(shortOption), flongOption(longOption), fdescription(description)
{
	
}

Parameter::~Parameter() {}

const char* Parameter::description() const { return fdescription; }
const char* Parameter::longOption() const { return flongOption; }
char Parameter::shortOption() const { return fshortOption; }

bool Parameter::receive(ParserState& state) throw(ParameterRejected) {

	const string arg = state.get();

	try {
		if(arg.at(0) != '-') return false;
		
		if(arg.at(1) == '-') { /* Long form parameter */
			unsigned int eq = arg.find_first_of("=");
			string option = arg.substr(2, ((eq == string::npos) ? arg.length() : eq)-2);

			if(option != longOption()) return false;

			if(eq != string::npos) eq++;
			this->receiveLong(state, eq);
			return true;
		}

		if(arg.at(1) == shortOption()) {
			/* Matched argument on the form -f or -fsomething */
			if(arg.length() == 2) { /* -f */
				this->receiveShort(state, string::npos);
				return true;
			} else { /* -fsomething */
				this->receiveShort(state, 2);
				return true;
			}
		}
	} catch(out_of_range& o) {
		return false;
	}

	return false;
}



SwitchParameter::SwitchParameter(char shortOption, const char *longOption,
			const char* description) : Parameter(shortOption, longOption, description), fset(false){}
SwitchParameter::~SwitchParameter() {}


bool SwitchParameter::isSet() const { return fset; }

const string SwitchParameter::usageLine() const {
	return string("-") + shortOption() + "\t| --" + longOption();
}

void SwitchParameter::receiveShort(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) {
	if(argument_offset != string::npos) throw UnexpectedArgument(string("-") + shortOption());
	fset = true;
}
void SwitchParameter::receiveLong(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) {
	if(argument_offset != string::npos) throw UnexpectedArgument(string("--") +longOption());
	fset = true;
}

StringParameter::StringParameter(char shortOption, const char *longOption,
			const char* description) : SwitchParameter(shortOption, longOption, description) {}
StringParameter::~StringParameter() {}

const string StringParameter::usageLine() const {
	return string("-") + shortOption() + "arg\t| --" + longOption() + "=arg";
}


void StringParameter::receiveShort(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) {
	if(argument_offset == string::npos) throw ExpectedArgument(string("-") + shortOption());
	try {
		this->receiveArgument(state.get().substr(argument_offset));
	} catch(ParameterRejected& r) {
		throw ParameterRejected(string("-") + shortOption() + ":" + r.what());
	}
	fset = true;
}

void StringParameter::receiveLong(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) {
	if(argument_offset == string::npos) throw ExpectedArgument(string("--") + longOption());
	try {
		this->receiveArgument(state.get().substr(argument_offset));
	} catch(ParameterRejected &r) {
		throw ParameterRejected(string("--") + longOption() + ":" + r.what());
	}
	fset = true;
}

void StringParameter::receiveArgument(const string &argument) throw (ParameterRejected) {
	this->argument = argument;

}

const string & StringParameter::stringValue() const { return argument; }



template<>
int PODParameter<int>::validate(const string &s) throw(ParameterRejected) {
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
long PODParameter<long>::validate(const string &s) throw(ParameterRejected) {
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	long l = strtol(cstr, &cstr, 10);
	if(*cstr != '\0') throw ParameterRejected("Expected long");

	return l;
}

template<>
double PODParameter<double>::validate(const string &s) throw(ParameterRejected) {
	char* cstr = const_cast<char*>(s.c_str());
	if(*cstr == '\0') throw ParameterRejected("No argument given");

	double d = strtod(cstr, &cstr);
	if(*cstr != '\0') throw ParameterRejected("Expected double");

	return d;
}
