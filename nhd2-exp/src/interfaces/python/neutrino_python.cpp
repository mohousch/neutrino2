/*
	$Id: neutrino_python.cpp 20.01.2019 mohousch Exp $

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>
#include <Python.h>

#include <interfaces/python/neutrino_python.h>

extern "C" void init_neutrino();

neutrinoPython::neutrinoPython()
{
	Py_Initialize();
	PyEval_InitThreads();

	init_neutrino();
}

neutrinoPython::~neutrinoPython()
{
	//Py_Finalize();
}

int neutrinoPython::execFile(const char *filename)
{
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return -ENOENT;

	int ret = PyRun_SimpleFile(fp, filename);
	fclose(fp);

	return ret;
}

int neutrinoPython::execute(const std::string &moduleName, const std::string &funcName)
{
	PyObject* pName;
	PyObject* pModule;
	PyObject* pDict;
	PyObject* pFunc;
	PyObject* pArgs;
	PyObject* pValue;

	pName = PyString_FromString(moduleName.c_str());

	pModule = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModule)
	{
		pDict = PyModule_GetDict(pModule);

		pFunc = PyDict_GetItemString(pDict, funcName.c_str());

		if (pFunc && PyCallable_Check(pFunc))
		{
			pArgs = PyTuple_New(0);

			// implement arguments..
			pValue = PyObject_CallObject(pFunc, pArgs);
			Py_DECREF(pArgs);

			if (pValue)
			{
				printf("Result of call: %ld\n", PyInt_AsLong(pValue));
				Py_DECREF(pValue);
			} 
			else
			{
				Py_DECREF(pModule);
				PyErr_Print();

				return 1;
			}
		}
	} 
	else
	{
		if (PyErr_Occurred())
			PyErr_Print();

		return 1;
	}

	return 0;
}


