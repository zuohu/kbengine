/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pickler.hpp"
#include "py_memorystream.hpp"

namespace KBEngine{ namespace script{

PySequenceMethods PyMemoryStream::seqMethods =
{
	seq_length,		// inquiry sq_length;				len(x)
	0,				// binaryfunc sq_concat;			x + y
	0,				// intargfunc sq_repeat;			x * n
	0,				// intargfunc sq_item;				x[i]
	0,				//seq_slice,				// intintargfunc sq_slice;			x[i:j]
	0,				// intobjargproc sq_ass_item;		x[i] = v
	0,				//seq_ass_slice,			// intintobjargproc sq_ass_slice;	x[i:j] = v
	0,				// objobjproc sq_contains;			v in x
	0,				// binaryfunc sq_inplace_concat;	x += y
	0				// intargfunc sq_inplace_repeat;	x *= n
};

SCRIPT_METHOD_DECLARE_BEGIN(PyMemoryStream)
SCRIPT_METHOD_DECLARE("append",				append,			METH_VARARGS, 0)
SCRIPT_METHOD_DECLARE("pop",				pop,			METH_VARARGS, 0)
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(PyMemoryStream)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(PyMemoryStream)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(PyMemoryStream, 0, &PyMemoryStream::seqMethods, 0, 0, 0)		
	
//-------------------------------------------------------------------------------------
PyMemoryStream::PyMemoryStream(PyTypeObject* pyType, bool isInitialised):
ScriptObject(pyType, isInitialised)
{
}

//-------------------------------------------------------------------------------------
PyMemoryStream::~PyMemoryStream()
{
}

//-------------------------------------------------------------------------------------
void PyMemoryStream::addToStream(MemoryStream* mstream)
{
	ArraySize size = stream().size();

	(*mstream) << size;
	if(size > 0)
	{
		ArraySize rpos = stream().rpos(), wpos = stream().wpos();
		(*mstream) << rpos;
		(*mstream) << wpos;
		(*mstream).append(stream().data(), size);
	}
}

//-------------------------------------------------------------------------------------
void PyMemoryStream::createFromStream(MemoryStream* mstream)
{
	ArraySize size;
	ArraySize rpos, wpos;

	(*mstream) >> size;
	if(size > 0)
	{
		(*mstream) >> rpos;
		(*mstream) >> wpos;

		stream().append(mstream->data() + mstream->rpos(), size);
		stream().rpos(rpos);
		stream().wpos(wpos);

		mstream->read_skip(size);
	}
}

//-------------------------------------------------------------------------------------
PyObject* PyMemoryStream::tp_repr()
{
	PyObject* pybytes = this->pyBytes();
	PyObject* pyStr = PyObject_Str(pybytes);
	Py_DECREF(pybytes);
	return pyStr;
}

//-------------------------------------------------------------------------------------
PyObject* PyMemoryStream::tp_str()
{
	return tp_repr();
}

//-------------------------------------------------------------------------------------
Py_ssize_t PyMemoryStream::seq_length(PyObject* self)
{
	PyMemoryStream* seq = static_cast<PyMemoryStream*>(self);
	return seq->length();
}

//-------------------------------------------------------------------------------------
PyObject* PyMemoryStream::__py_append(PyObject* self, PyObject* args, PyObject* kwargs)
{
	PyMemoryStream* pyobj = static_cast<PyMemoryStream*>(self);

	int argCount = PyTuple_Size(args);
	if(argCount != 2)
	{
		PyErr_Format(PyExc_AssertionError, "Blob::append: args is error! arg1 is type[UINT8|STRING|...], arg2 is val.");
		PyErr_PrintEx(0);
	}
	
	char* type;
	PyObject* pyVal = NULL;

	if(PyArg_ParseTuple(args, "s|O", &type, &pyVal) == -1)
	{
		PyErr_Format(PyExc_TypeError, "Blob::append: args is error!");
		PyErr_PrintEx(0);
		return NULL;
	}

	if(strcmp(type, "UINT8") == 0)
	{
		uint8 v = (uint8)PyLong_AsUnsignedLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "UINT16") == 0)
	{
		uint16 v = (uint16)PyLong_AsUnsignedLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "UINT32") == 0)
	{
		uint32 v = PyLong_AsUnsignedLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "UINT64") == 0)
	{
		uint64 v = PyLong_AsUnsignedLongLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "INT8") == 0)
	{
		int8 v = (int8)PyLong_AsLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "INT16") == 0)
	{
		int16 v = (int16)PyLong_AsLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "INT32") == 0)
	{
		int32 v = PyLong_AsLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "INT64") == 0)
	{
		int64 v = PyLong_AsLongLong(pyVal);
		pyobj->stream() << v;
	}
	else if(strcmp(type, "STRING") == 0)
	{
		wchar_t* ws = PyUnicode_AsWideCharString(pyVal, NULL);					
		char* s = strutil::wchar2char(ws);									
		PyMem_Free(ws);

		pyobj->stream() << s;
		free(s);
	}
	else if(strcmp(type, "UNICODE") == 0)
	{
		PyObject* obj = PyUnicode_AsUTF8String(pyVal);
		if(obj == NULL)
		{
			PyErr_Format(PyExc_TypeError, "Blob::append: val is not UNICODE!");
			PyErr_PrintEx(0);
			return NULL;
		}	

		pyobj->stream().appendBlob(PyBytes_AS_STRING(obj), PyBytes_GET_SIZE(obj));
		Py_DECREF(obj);
	}
	else if(strcmp(type, "PYTHON") == 0 || strcmp(type, "PY_DICT") == 0
		 || strcmp(type, "PY_TUPLE") == 0  || strcmp(type, "PY_LIST") == 0)
	{
		std::string datas = Pickler::pickle(pyVal);
		pyobj->stream().appendBlob(datas);
	}
	else if(strcmp(type, "BLOB") == 0)
	{
		if(!PyObject_TypeCheck(pyVal, PyMemoryStream::getScriptType()))
		{
			PyErr_Format(PyExc_TypeError, "Blob::append: val is not BLOB!");
			PyErr_PrintEx(0);
			return NULL;
		}

		PyMemoryStream* obj = static_cast<PyMemoryStream*>(pyVal);
		pyobj->stream().append(obj->stream());
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "Blob::append: type %s no support!", type);
		PyErr_PrintEx(0);
		return NULL;
	}

	S_Return;	
}

//-------------------------------------------------------------------------------------
PyObject* PyMemoryStream::__py_pop(PyObject* self, PyObject* args, PyObject* kwargs)
{
	PyMemoryStream* pyobj = static_cast<PyMemoryStream*>(self);
	return NULL;
}

//-------------------------------------------------------------------------------------
}
}

