// Copyright 2008-2018 Yolo Technologies, Inc. All Rights Reserved. https://www.comblockengine.com

#include "script.h"
#include "pyurl.h"
#include "scriptstdouterr.h"
#include "py_macros.h"
#include "helper/profile.h"

namespace KBEngine{ namespace script {

bool PyUrl::isInit = false;
std::map<PyObject*, PyObjectPtr> PyUrl::pyCallbacks;

//-------------------------------------------------------------------------------------
bool PyUrl::initialize(Script* pScript)
{
	if(isInit)
		return true;
	
	isInit = true;

	// 注册产生uuid方法到py
	APPEND_SCRIPT_MODULE_METHOD(pScript->getModule(),	urlopen,	__py_urlopen,	METH_VARARGS,	0);
	return isInit;
}

//-------------------------------------------------------------------------------------
void PyUrl::finalise(void)
{
	if (!isInit)
		return;

	isInit = false;
	pyCallbacks.clear();
}

//-------------------------------------------------------------------------------------
void PyUrl::onHttpCallback(bool success, const Network::Http::Request& pRequest, const std::string& data)
{
	if (!isInit)
		return;

	KBE_ASSERT(pRequest.getUserargs());

	// httpcode, data, headers, opt_success, url 
	PyObject* pyargs = PyTuple_New(5);

	PyTuple_SET_ITEM(pyargs, 0, PyLong_FromLong(pRequest.getHttpCode()));
	PyTuple_SET_ITEM(pyargs, 1, PyUnicode_FromString(pRequest.getReceivedContent()));
	PyTuple_SET_ITEM(pyargs, 2, PyUnicode_FromString(pRequest.getReceivedHeader()));
	PyTuple_SET_ITEM(pyargs, 3, PyBool_FromLong(success));
	PyTuple_SET_ITEM(pyargs, 4, PyUnicode_FromString(pRequest.url()));

	PyObject* pyRet = PyObject_CallObject((PyObject*)pRequest.getUserargs(), pyargs);
	if (pyRet == NULL)
	{
		SCRIPT_ERROR_CHECK();
	}
	else
	{
		Py_DECREF(pyRet);
	}

	Py_DECREF(pyargs);
	pyCallbacks.erase((PyObject*)pRequest.getUserargs());
}

//-------------------------------------------------------------------------------------
PyObject* PyUrl::__py_urlopen(PyObject* self, PyObject* args)
{
    char* surl = NULL;
    PyObject* pyCallback = NULL;
    PyObject* pyPostData = NULL;
    PyObject* pyHeaders = NULL;
    float timeout = -1.0;
 
    int argCount = (int)PyTuple_Size(args);
 
    if (argCount < 1 || argCount > 5)
    {
        PyErr_SetString(PyExc_TypeError, "KBEngine::urlopen: takes 1 to 5 arguments (url[, callback, postdata, headers, timeout])");
        return NULL;
    }
 
    if (!PyArg_ParseTuple(args, "s|OOOOf", &surl, &pyCallback, &pyPostData, &pyHeaders, &timeout))
    {
        PyErr_SetString(PyExc_AssertionError, "KBEngine::urlopen: args error!");
		return NULL;
    }
 
    if (!surl)
    {
        PyErr_SetString(PyExc_ValueError, "KBEngine::urlopen: URL cannot be null or empty.");
        return NULL;
    }
 
    if (pyCallback && !PyCallable_Check(pyCallback))
    {
        PyErr_SetString(PyExc_TypeError, "KBEngine::urlopen: callback must be callable.");
        return NULL;
    }
 
    std::map<std::string, std::string> map_headers;
    char* postData = NULL;
    Py_ssize_t postDataLength = 0;
 
    if (pyHeaders && PyDict_Check(pyHeaders))
    {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(pyHeaders, &pos, &key, &value))
        {
            if (!PyUnicode_Check(key) || !PyUnicode_Check(value))
            {
                PyErr_SetString(PyExc_AssertionError,
                    "KBEngine::urlopen: headers keys and values must be strings.");
                return NULL;
            }
            std::string k = PyUnicode_AsUTF8AndSize(key, NULL);
            std::string v = PyUnicode_AsUTF8AndSize(value, NULL);
            map_headers[k] = v;
        }
    }
    else if (pyHeaders && !pyHeaders->ob_type->tp_name)
    {
        PyErr_SetString(PyExc_TypeError, "KBEngine::urlopen: headers must be a dictionary of string -> string.");
        return NULL;
    }
 
    if (pyPostData)
    {
        if (PyBytes_Check(pyPostData))
        {
            if (PyBytes_AsStringAndSize(pyPostData, &postData, &postDataLength) < 0)
            {
                SCRIPT_ERROR_CHECK();
                return NULL;
            }
        }
        else if (pyPostData != Py_None)
        {
            PyErr_SetString(PyExc_TypeError, "KBEngine::urlopen: POST data must be bytes or None.");
            return NULL;
        }
    }
 
    Network::Http::Request* pRequest = new Network::Http::Request();
 
    if (pyCallback)
    {
        pyCallbacks[pyCallback] = PyObjectPtr(pyCallback);
        pRequest->setUserargs(pyCallback);
        pRequest->setCallback(onHttpCallback);
    }
 
    if (!map_headers.empty()) 
    {
        Network::Http::Request::Status result = pRequest->setHeader(map_headers);
        if (result != Network::Http::Request::OK)
        {
            delete pRequest;
            return PyLong_FromLong(result);
        }
    }
 
    if (postData && postDataLength > 0)
    {
        Network::Http::Request::Status result = pRequest->setPostData(postData, postDataLength);
        if (result != Network::Http::Request::OK)
        {
            delete pRequest;
            return PyLong_FromLong(result);
        }
    }
 
    Network::Http::Request::Status result = pRequest->setURL(surl);
    if (result != Network::Http::Request::OK)
    {
        delete pRequest;
        return PyLong_FromLong(result);
    }
 
    if (timeout > 0)
    {
        result = pRequest->setTimeout(static_cast<uint32>(timeout * 1000));
        if (result != Network::Http::Request::OK)
        {
            delete pRequest;
            return PyLong_FromLong(result);
        }
    }
 
    result = Network::Http::perform(pRequest);
    return PyLong_FromLong(result);

}

//-------------------------------------------------------------------------------------

}
}
