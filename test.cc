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
#include <iostream>

using namespace std;

int main(int argc, const char* argv[]) {
	OptionsParser optp;

	/* An alternative option is to simply extend the options parser and set all this up
	 * in the constructor.
	 */
	SwitchParameter f('f', "foo", 	"Enable the foo system (no argument)");
	StringParameter b('b', "bar", 	"Enable the bar system (string argument)");
	PODParameter<double> z('z', "baz", "Enable the baz system (floating point argument)");
	PODParameter<int> i('i', "foobar", "Enable the foobar system (integer argument)");

	i.setDefault(15);

	optp.addParameter(&f);
	optp.addParameter(&b);
	optp.addParameter(&z);
	optp.addParameter(&i);

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

