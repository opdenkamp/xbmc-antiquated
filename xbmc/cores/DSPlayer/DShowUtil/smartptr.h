// SmartPtr.h
//
// Defines a smart pointer class that does not depend on any ATL headers

#pragma once

namespace Com
{

  ///////////////////////////////////////////////////////////////////////
  // Name: AreCOMObjectsEqual [template]
  // Desc: Tests two COM pointers for equality.
  ///////////////////////////////////////////////////////////////////////

  template <class T1, class T2>
  bool AreComObjectsEqual(T1 *p1, T2 *p2)
  {
      bool bResult = false;
      if (p1 == NULL && p2 == NULL)
      {
          // Both are NULL
          bResult = true;
      }
      else if (p1 == NULL || p2 == NULL)
      {
          // One is NULL and one is not
          bResult = false;
      }
      else 
      {
          // Both are not NULL. Compare IUnknowns.
          IUnknown *pUnk1 = NULL;
          IUnknown *pUnk2 = NULL;
          if (SUCCEEDED(p1->QueryInterface(IID_IUnknown, (void**)&pUnk1)))
          {
              if (SUCCEEDED(p2->QueryInterface(IID_IUnknown, (void**)&pUnk2)))
              {
                  bResult = (pUnk1 == pUnk2);
                  pUnk2->Release();
              }
              pUnk1->Release();
          }
      }
      return bResult;
  }
  
  inline __declspec(nothrow) IUnknown* __stdcall SmartPtrAssign(IUnknown** pp, IUnknown* lp)
  {
    if (pp == NULL)
      return NULL;
      
    if (lp != NULL)
      lp->AddRef();
    if (*pp)
      (*pp)->Release();
    *pp = lp;
    return lp;
  }

  inline __declspec(nothrow) IUnknown* __stdcall SmartQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
  {
    if (pp == NULL)
      return NULL;

    IUnknown* pTemp = *pp;
    *pp = NULL;
    if (lp != NULL)
      lp->QueryInterface(riid, (void**)pp);
    if (pTemp)
      pTemp->Release();
    return *pp;
  }

  // _NoAddRefOrRelease:
  // This is a version of our COM interface that dis-allows AddRef
  // and Release. All ref-counting should be done by the SmartPtr 
  // object, so we want to dis-allow calling AddRef or Release 
  // directly. The operator-> returns a _NoAddRefOrRelease pointer
  // instead of returning the raw COM pointer. (This behavior is the
  // same as ATL's CComPtr class.)
  template <class T>
  class _NoAddRefOrRelease : public T
  {
  private:
      STDMETHOD_(ULONG, AddRef)() = 0;
      STDMETHOD_(ULONG, Release)() = 0;
  };

  template <class T>
  class SmartPtrBase
  {
  protected:

    // Ctor
    SmartPtrBase() : m_ptr(NULL)
    {
    }

    // Ctor
    SmartPtrBase(T *ptr)
    {
        m_ptr = ptr;
        if (m_ptr)
        {
            m_ptr->AddRef();
        }
    }

    // Copy ctor
    SmartPtrBase(const SmartPtrBase& sptr)
    {
        m_ptr = sptr.m_ptr;
        if (m_ptr)
        {
            m_ptr->AddRef();
        }
    }
    
  public:
    typedef T _PtrClass;
    // Dtor
    ~SmartPtrBase() 
    { 
        if (m_ptr)
        {
            m_ptr->Release();
        }
    }
    T* operator=(_In_opt_ T* lp) throw()
    {
        if(*this!=lp)
        {
        return static_cast<T*>(AtlComPtrAssign((IUnknown**)&m_ptr, lp));
        }
        return *this;
    }
    // Assignment
    SmartPtrBase& operator=(const SmartPtrBase& sptr)
    {
        
        if (!AreComObjectsEqual(m_ptr, sptr.m_ptr))
        {
            if (m_ptr)
            {
                m_ptr->Release();
            }

            m_ptr = sptr.m_ptr;
            if (m_ptr)
            {
                m_ptr->AddRef();
            }
        }
        return *this;
    }

    // address-of operator
    T** operator&()
    {
      ASSERT( m_ptr == NULL );
      return &m_ptr;
    }

    // dereference operator
    _NoAddRefOrRelease<T>* operator->()
    {
        return (_NoAddRefOrRelease<T>*)m_ptr;
    }

    // coerce to underlying pointer type.
    operator T*()
    {
        return m_ptr;
    }
    
    T& operator*() const
    {
      ASSERT( m_ptr != NULL );
      return *m_ptr;
    }
    
    bool operator!()
    {
      return (m_ptr == NULL);
    }
    
    bool operator<(T* pT)
    {
      return m_ptr < pT;
    }

    // Templated version of QueryInterface

    template <class Q> // Q is another interface type
    HRESULT QueryInterface(Q **ppQ)
    {
      return m_ptr->QueryInterface(__uuidof(Q), (void**)ppQ);
    }
 
    HRESULT CreateInstance(REFCLSID clsid, LPUNKNOWN pUnkOuter = NULL,DWORD dwClsContext = CLSCTX_ALL)
    {
    ASSERT(m_ptr == NULL);
      return CoCreateInstance(clsid,pUnkOuter, dwClsContext, __uuidof(T),(void **)&m_ptr);
    }
    // safe Release() method
    ULONG Release()
    {
      T *ptr = m_ptr;
      ULONG result = 0;
      if (ptr)
      {
        m_ptr = NULL;
        result = ptr->Release();
      }
      return result;
    }

    // Attach to an existing interface (does not AddRef)
    void Attach(T* p) 
    {
      if (m_ptr)
      {
        m_ptr->Release();
      }
      m_ptr = p;
    }
        
    // Detach the interface (does not Release)
    T* Detach()
    {
        T* p = m_ptr;
        m_ptr = NULL;
        return p;
    }

    HRESULT CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
    {
      ASSERT(m_ptr == NULL);
      return ::CoCreateInstance(rclsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&m_ptr);
    }
    HRESULT CoCreateInstance(LPCOLESTR szProgID, LPUNKNOWN pUnkOuter = NULL, DWORD dwClsContext = CLSCTX_ALL)
    {
      CLSID clsid;
      HRESULT hr = CLSIDFromProgID(szProgID, &clsid);
      ASSERT(m_ptr == NULL);
      if (SUCCEEDED(hr))
        hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, __uuidof(T), (void**)&m_ptr);
      return hr;
    }
    template <class Q>
    HRESULT QueryInterface(Q** pp) const
    {
      ASSERT(pp != NULL);
      return m_ptr->QueryInterface(__uuidof(Q), (void**)pp);
    }
    
    // equality operator
    bool operator==(T *ptr) const
    {
      return m_ptr == ptr;
        //return AreComObjectsEqual(m_ptr, ptr);
    }

    // inequality operator
    bool operator!=(T *ptr) const
    {
        return !operator==(ptr);
    }

  public:
    T *m_ptr;
  };
  
  template <class T>
  class SmartPtr : public SmartPtrBase<T>
  {
  public:
    SmartPtr() throw()
    {
    }
    SmartPtr(T* lp) throw() :
      SmartPtrBase<T>(lp)

    {
    }
    SmartPtr(const SmartPtr<T>& lp) throw() :
      SmartPtrBase<T>(lp)
    {
    }
    T* operator=(T* lp) throw()
    {
      if(*this!=lp)
      {
        return static_cast<T*>(SmartPtrAssign((IUnknown**)&m_ptr, lp));
      }
      return *this;
    }
    template <typename Q>
    T* operator=(const SmartPtr<Q>& lp) throw()
    {
      if( !AreComObjectsEqual(*this, lp) )
      {
        return static_cast<T*>(SmartQIPtrAssign((IUnknown**)&m_ptr, lp.m_ptr, __uuidof(T)));
      }
      return *this;
    }
    T* operator=(const SmartPtr<T>& lp) throw()
    {
      if( !AreComObjectsEqual(m_ptr, lp.m_ptr) )
      {
        return static_cast<T*>(SmartPtrAssign((IUnknown**)&m_ptr, lp.m_ptr));
      }
      return *this;
    }
  };

  template <class T, const IID* piid = &__uuidof(T)>
  class SmartQIPtr : public SmartPtr<T>
  {
  public:
    SmartQIPtr() throw()
    {
    }
    SmartQIPtr(T* lp) throw() :
    SmartPtr<T>(lp)
    {
    }
    SmartQIPtr(const SmartQIPtr<T, piid>& lp) throw() :
    SmartPtr<T>(lp)
    {
    }
    SmartQIPtr(IUnknown* lp) throw()
    {
      if (lp != NULL)
        lp->QueryInterface(*piid, (void **)&m_ptr);
    }
    T* operator=(T* lp) throw()
    {
      if(*this!=lp)
      {
        return static_cast<T*>(SmartPtrAssign((IUnknown**)&m_ptr, lp));
      }
      return *this;
    }
    T* operator=(const SmartQIPtr<T, piid>& lp) throw()
    {
      if(*this!=lp)
      {
        return static_cast<T*>(SmartPtrAssign((IUnknown**)&m_ptr, lp.m_ptr));
      }
      return *this;
    }
    T* operator=(_In_opt_ IUnknown* lp) throw()
    {
      if(*this!=lp)
      {
        return static_cast<T*>(SmartQIPtrAssign((IUnknown**)&m_ptr, lp, *piid));
      }
      return *this;
    }
  };
}
