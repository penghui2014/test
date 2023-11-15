#pragma once

#include <iostream>
#include <dlfcn.h>
#include <map>
#include <string>
#include <functional>
#include "mangle.hpp"

namespace miniRPC
{

class CDllHelper{
public:
	
    CDllHelper(const std::string &dllPath,bool load = true):m_dllPath(dllPath)
	{
		if(load)
		{
			Load(false);
		}
		
	}
	
    ~CDllHelper()
    {
        Unload();
    }
	
    bool Load(bool loadType = false)
    {
		Unload();
		
        m_isLoad = false;
        void* pHandle = nullptr;

        const char* dlsymError = dlerror();
        if(loadType)
        {
            pHandle = dlopen(m_dllPath.c_str(), RTLD_NOW|RTLD_GLOBAL);
        }
        else
        {
            pHandle = dlopen(m_dllPath.c_str(), RTLD_NOW|RTLD_LOCAL);
        }

        if (pHandle == nullptr) 
        {
            std::cout<< "000[ERROR][ERROR][ERROR]Cannot load library  " <<m_dllPath << "  " << dlerror() << std::endl;
            return false;
        }
        dlsymError = dlerror();
        if(dlsymError)
        {
            std::cout<< "111[ERROR][ERROR][ERROR]Cannot load library  " <<m_dllPath << "  " << dlerror() << std::endl;
            return false;
        }
        std::cout<< "dlopen " << m_dllPath << " success" << std::endl;
        m_dllHandle = pHandle;
        m_isLoad = true;
	    return true;         
    }
    
    void Unload(void)
    {
		m_isLoad = false;
		
		m_funcMap.clear();
		
        if (nullptr == m_dllHandle)
        {
            return;
        }
        
		dlclose(m_dllHandle);
		m_dllHandle = nullptr;
        
        return ;
    }


    template <typename T>
    T* GetFunction(const std::string& funcName)
    {
        if (!m_isLoad)
        {
            return nullptr;
        }
        if (nullptr == m_dllHandle)
        {
            return nullptr;
        }
        
        auto iter = m_funcMap.find(funcName);
        void *addr = nullptr;
        if (iter == m_funcMap.end())
        {
			const char* dlsymError = dlerror();
            addr = dlsym(m_dllHandle, funcName.c_str());
			dlsymError = dlerror();
            if (nullptr == addr)
            {
                std::cout<< dlsymError<<std::endl;
                return nullptr;
            }
            m_funcMap.insert(std::make_pair(funcName,(void*)addr));
            iter = m_funcMap.find(funcName);
        }
        addr = (void*)(iter->second);
        if (nullptr == addr)
        {
            std::cout<<"can not find function: " + funcName<<std::endl;
            return nullptr;
        }
        return (T*) (addr);
    }

    template <typename T, typename... Args>
	auto InvokeC(const std::string& funcName, Args&&... args)
    ->typename std::result_of<std::function<T>(Args...)>::type 
    {
        auto f = GetFunction<T>(funcName);
        if (f == nullptr)
        {
            std::string s = "can not find this function: " + funcName;
            std::cout<< s << std::endl;
			typename std::result_of<std::function<T>(Args...)>::type result;
			return result;
            //throw std::exception(s.c_str());
        }
        else
        {
            return f(std::forward<Args>(args)...);
        }
    }
	
	template <typename T, typename... Args>
	auto InvokeCpp(const std::string& funcName, Args&&... args)
    ->typename std::result_of<std::function<T>(Args...)>::type 
    {
		std::string cppName = MangleCpp<T>(funcName);
        auto f = GetFunction<T>(cppName);
        if (f == nullptr)
        {
            std::string s = "can not find this function: " + cppName;
            std::cout<< s << std::endl;
			typename std::result_of<std::function<T>(Args...)>::type result;
			return result;
            //throw std::exception(s.c_str());
        }
        else
        {
            return f(std::forward<Args>(args)...);
        }
    }
	
private:
	void *m_dllHandle{nullptr};
	std::map<std::string,void*> m_funcMap;
	bool m_isLoad{false};
	std::string m_dllPath;
};



}