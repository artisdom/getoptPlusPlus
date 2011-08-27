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
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace vlofgren;

/*
 *
 * Two ways of adding new parameter types (you only need to override two functions)
 *
 */


class AlphabeticParameter : public CommonParameter<UniquelySwitchable> {
public:
	AlphabeticParameter(char shortName, const char* longName, const char* description) :
		CommonParameter<UniquelySwitchable>(shortName, longName, description) {}


	void receiveSwitch() throw(Parameter::ParameterRejected) {
		throw Parameter::ParameterRejected();
	}


	/* isalpha may be a macro */
	static bool isNotAlpha(char c) { return !isalpha(c); }

	void receiveArgument(const string& arg) throw(Parameter::ParameterRejected) {
		int nonalpha = count_if(arg.begin(), arg.end(), isNotAlpha);

		if(nonalpha) throw Parameter::ParameterRejected("I only want numbers");

		value = arg;

		set();
	}

	const string& getValue() const { return value; }

private:
	string value;
};

/*
 *
 * The other way is to specialize the PODParameter class
 *
 */

enum RockPaperScissor { ROCK, PAPER, SCISSOR } ;

namespace vlofgren {
	// needs to live in the vlofgren namespace for whatever reason
	template<> enum RockPaperScissor
	PODParameter<enum RockPaperScissor>::validate(const string &s) throw(Parameter::ParameterRejected)
	{
		if(s == "rock")
			return ROCK;
		else if(s == "paper")
			return PAPER;
		else if(s == "scissor")
			return SCISSOR;
		else {
			throw ParameterRejected("Invalid argument");
		}

	}
}
typedef PODParameter<enum RockPaperScissor> RockPaperScissorParameter;

int main(int argc, const char* argv[]) {
	OptionsParser optp;

	/* An alternative option is to simply extend the options parser and set all this up
	 * in the constructor.
	 */
	SwitchParameter f('f', "foo", 	"Enable the foo system (no argument)");
	StringParameter b('b', "bar", 	"Enable the bar system (string argument)");
	PODParameter<double> z('z', "baz", "Enable the baz system (floating point argument)");
	PODParameter<int> i('i', "foobar", "Enable the foobar system (integer argument)");
	AlphabeticParameter alpha('a', "alpha", "Custom parameter that requires a string of letters");
	RockPaperScissorParameter rps('r', "rps", "Takes the values rock, paper and scissor");
	i.setDefault(15);

	optp.addParameter(&f);
	optp.addParameter(&b);
	optp.addParameter(&z);
	optp.addParameter(&i);
	optp.addParameter(&alpha);
	optp.addParameter(&rps);


	try {
		optp.parse(argc, argv);


		cout << "The following parameters were set:" << endl;

		cout << "foo: " << (f.isSet() ? "true" : "false") << endl;
		cout << "bar: \"" << b.stringValue() << "\""<< endl;
		cout << "baz: ";
		if(z.isSet()) {
			cout << z.getValue() << endl;
		} else {
			cout << "not set" << endl;
		}
		cout << "foobar: ";
		if(i.isSet()) {
			cout << i.getValue() << endl;
		} else {
			cout << "not set" << endl;
		}
		cout << "alpha: ";
		if(alpha.isSet()) {
			cout << alpha.getValue() << endl;
		} else {
			cout << "not set" << endl;
		}

		cout << "rps: ";
		if(rps.isSet()) {
			cout << rps.getValue() << endl;
		} else {
			cout << "not set" << endl;
		}




	} catch(Parameter::ParameterRejected &p){
		// This will happen if the user has fed some malformed parameter to the program
		cerr << p.what() << endl;
		optp.usage("An example program (that also runs some tests)");
		return EXIT_FAILURE;
	} catch(runtime_error &e) {
		// This will happen if you try to access a parameter that hasn't been set
		cerr << e.what() << endl;

		return EXIT_FAILURE;
	}


	cout << "The following file arguments were given:" << endl;

	vector<string> files = optp.getFiles();
	for(vector<string>::iterator i = files.begin(); i != files.end(); i++) {
		cout << "File: " << *i << endl;
	}


	return EXIT_SUCCESS;
}

