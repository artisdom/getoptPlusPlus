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
#include <sstream>
#include <iostream>

#ifndef GETOPTPP_H
#define GETOPTPP_H

namespace vlofgren {

class Parameter;
class ParserState;

using namespace std;


class OptionsParser {
public:
	OptionsParser(const char *programDesc);
	virtual ~OptionsParser();

	void addParameter(Parameter * const param);
	void parse(int argc, const char* argv[]) throw(runtime_error);

	void usage() const;
	const string& programName() const;

	const vector<string>& getFiles() const;
protected:
	string argv0;
	string fprogramDesc;

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
		ParameterRejected() : runtime_error("") {}
	};

	class UnexpectedArgument : public ParameterRejected {
	public:
		UnexpectedArgument(const string &s) : ParameterRejected(s) {}
		UnexpectedArgument() {}
	};

	class ExpectedArgument : public ParameterRejected {
	public:
		ExpectedArgument(const string &s) : ParameterRejected(s) {}
		ExpectedArgument() {}
	};

	Parameter(char shortOption, const char *longOption, 
		  const char *description);

	virtual ~Parameter();

	virtual string usageLine() const = 0;

	const string& description() const;
	const string& longOption() const;
	char shortOption() const;
protected:
	virtual bool receive(ParserState& state) throw(ParameterRejected) = 0;

	friend class OptionsParser;

	char fshortOption;
	const string flongOption;
	const string fdescription;
private:

};

/*
 *
 * Abstract base class of all parameters
 *
 */

class Switchable;

template<typename SwitchingBehavior=Switchable>
class CommonParameter : public Parameter, protected SwitchingBehavior {
public:
	virtual bool isSet() const;

	CommonParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~CommonParameter();

	virtual string usageLine() const;

protected:
	virtual bool receive(ParserState& state) throw(ParameterRejected);

	virtual void receiveSwitch() throw (ParameterRejected) = 0;
	virtual void receiveArgument(const string& argument) throw (ParameterRejected) = 0;
};


class Switchable {
public:
	class SwitchingError : public Parameter::ParameterRejected {};

	virtual bool isSet() const;
	virtual void set() throw (SwitchingError);

	virtual ~Switchable();
	Switchable();
private:
	bool fset;
};

class UniquelySwitchable : public Switchable {
public:

	virtual ~UniquelySwitchable();
	virtual void set() throw (SwitchingError);
};

class PresettableUniquelySwitchable : public UniquelySwitchable {
public:
	virtual bool isSet() const;
	virtual void set() throw (Switchable::SwitchingError);
	virtual void preset();

	virtual ~PresettableUniquelySwitchable();
private:
	Switchable fpreset;
};

/* Parameter that does not take an argument, and throws an exception
 * if an argument is given */

class SwitchParameter : public CommonParameter<Switchable> {
public:
	SwitchParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~SwitchParameter();

protected:
	virtual void receiveSwitch() throw (Parameter::ParameterRejected);
	virtual void receiveArgument(const string& argument) throw (Parameter::ParameterRejected);
};

/* Parameter that takes an argument, but does not perform an sort of validation.
 *
 * Throws an exception if no argument is given.
 *
 *  */
class StringParameter: public CommonParameter<UniquelySwitchable> {
public:
	StringParameter(char shortOption, const char *longOption,
			const char* description);
	virtual ~StringParameter();

	const string & stringValue() const;
	virtual string usageLine() const;

protected:
	virtual void receiveSwitch() throw (Parameter::ParameterRejected);
	virtual void receiveArgument(const string& argument) throw (Parameter::ParameterRejected);

	string argument;
};

/* Plain-Old-Data parameter. Performs input validation.
 *
 * Currently only supports int, long and (on some systems) double, but extending
 * it to other types is as easy as partial template specialization.
 */

template<typename T>
class PODParameter : public CommonParameter<PresettableUniquelySwitchable> {
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

	std::string usageLine() const;
protected:
	T validate(const string& s) throw (ParameterRejected);
	void receiveArgument(const string &argument) throw(ParameterRejected);
	void receiveSwitch() throw (Parameter::ParameterRejected);

	T value;
};


typedef PODParameter<int> IntParameter;
typedef PODParameter<long> LongParameter;
typedef PODParameter<double> DoubleParameter;

#include "parameter.include.cc"

} //namespace

#endif
