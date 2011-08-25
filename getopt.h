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


#include <set>
#include <vector>
#include <stdexcept>
#include <string>
#include <climits>
#include <cstdlib>
#ifndef GETOPTPP_H
#define GETOPTPP_H

class Parameter;
class ParserState;

using namespace std;
class OptionsParser {
public:
	OptionsParser();
	virtual ~OptionsParser();

	void addParameter(Parameter * const param);
	void parse(int argc, const char* argv[]) throw(runtime_error);

	void usage(const string &description) const;
	string programName() const;

	const vector<string>& getFiles() const;
protected:
	string argv0;
	set<Parameter*> parameters;
	vector<string> files;

	friend class ParserState;
};

/*
 * Corresponds to the state of the parsing, basically just a wrapper
 * for a const_iterator that handles nicer.
 */
class ParserState {
public:
	const string peek() const;
	const string get() const;
	void advance();
	bool end() const;
protected:
	ParserState(OptionsParser &opts, vector<string>& args);
private:
	friend class OptionsParser;

	OptionsParser &opts;
	const vector<string> &arguments;
	vector<string>::const_iterator iterator;
};

/*
 *
 * Abstract base class of all parameters
 *
 */

class Parameter {
public:
	class ParameterRejected : public runtime_error {
	public:
		ParameterRejected(const string& s) : runtime_error(s) {}
	};

	class UnexpectedArgument : public ParameterRejected {
	public:
		UnexpectedArgument(const string &s) : ParameterRejected(s) {}
	};

	class ExpectedArgument : public ParameterRejected {
		public:
			ExpectedArgument(const string &s) : ParameterRejected(s) {}
	};

	Parameter(char shortOption, const char *longOption, 
		  const char *description);
	virtual ~Parameter();

	virtual const string usageLine() const = 0;

	const char* description() const;
	const char* longOption() const;
	char shortOption() const;
protected:
	virtual bool receive(ParserState& state) throw(ParameterRejected);
	virtual void receiveShort(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) = 0;
	virtual void receiveLong(ParserState &state, unsigned int argument_offset) throw (ParameterRejected) = 0;

	friend class OptionsParser;

	char fshortOption;
	const char *flongOption;
	const char *fdescription;
private:

};

/* Parameter that does not take an argument, and throws an exception
 * if an argument is given */
class SwitchParameter : public Parameter {
public:
	SwitchParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~SwitchParameter();
	virtual const string usageLine() const;

	bool isSet() const;

protected:
	virtual void receiveShort(ParserState &state, unsigned int argument_offset) throw (ParameterRejected);
	virtual void receiveLong(ParserState &state, unsigned int argument_offset) throw (ParameterRejected);
	bool fset;
};

/* Parameter that takes an argument, but does not perform an sort of validation.
 *
 * Throws an exception if no argument is given.
 *
 *  */
class StringParameter: public SwitchParameter {
public:
	StringParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~StringParameter();

	const string & stringValue() const;
	virtual const string usageLine() const;

protected:
	virtual void receiveShort(ParserState &state, unsigned int argument_offset) throw (ParameterRejected);
	virtual void receiveLong(ParserState &state, unsigned int argument_offset) throw (ParameterRejected);
	virtual void receiveArgument(const string &argument) throw (ParameterRejected);

	string argument;
};

/* Plain-Old-Data parameter. Performs input validation.
 *
 * Currently only supports int, long and (on some systems) double, but extending
 * it to other types is as easy as partial template specialization.
 */



template<typename T>
class PODParameter : public StringParameter {
public:
	PODParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~PODParameter();

	/* Retreive the value of the argument. Throws an exception if
	 * the value hasn't been set (test with isSet())
	 */
	T getValue() const;
	operator T() const;
	void setDefault(T value);

protected:
	T validate(const string& s) throw (ParameterRejected);
	void receiveArgument(const string &argument) throw(ParameterRejected);

	T value;
};

template<typename T>
PODParameter<T>::PODParameter(char shortOption, const char *longOption,
		const char* description) : StringParameter(shortOption, longOption, description) {}
template<typename T>
PODParameter<T>::~PODParameter() {}

template<typename T>
PODParameter<T>::operator T() const { return getValue(); }

template<typename T>
void PODParameter<T>::setDefault(T value) {
	fset = true;
	this->value = value;
}

template<typename T>
T PODParameter<T>::getValue() const {
	if(!isSet()) {
		throw runtime_error(
				string("Attempting to retreive the argument of parameter") + longOption() + " but it hasn't been set!");
	}
	return value;

}

template<typename T>
void PODParameter<T>::receiveArgument(const string &argument) throw(ParameterRejected) {
	value = validate(argument);
}



#endif
