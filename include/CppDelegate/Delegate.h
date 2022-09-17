/*

	Copyright (C) 2017 by Sergey A Kryukov: derived work
	http://www.SAKryukov.org
	http://www.codeproject.com/Members/SAKryukov

	Based on original work by Sergey Ryazanov:
	"The Impossibly Fast C++ Delegates", 18 Jul 2005
	https://www.codeproject.com/articles/11015/the-impossibly-fast-c-delegates

	MIT license:
	http://en.wikipedia.org/wiki/MIT_License

	Original publication: https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed

*/

#pragma once
#include "DelegateBase.h"
#include <RVCore/utils.h>

namespace SA
{

	template <typename T>
	class delegate;
	template <typename T>
	class multicast_delegate;

	template <typename RET, typename... PARAMS>
	class delegate<RET(PARAMS...)> final : private delegate_base<RET(PARAMS...)>
	{
	  public:
		delegate() = default;

		bool isNull() const { return invocation.stub == nullptr; }
		bool operator==(void* ptr) const { return (ptr == nullptr) && this->isNull(); }	   // operator ==
		bool operator!=(void* ptr) const { return (ptr != nullptr) || (!this->isNull()); } // operator !=

		delegate(const delegate& another) { another.invocation.Clone(invocation); }

		template <typename LAMBDA>
		delegate(const LAMBDA& lambda)
		{
			assign((void*)(&lambda), lambda_stub<LAMBDA>);
		} // delegate

		delegate& operator=(const delegate& another)
		{
			another.invocation.Clone(invocation);
			return *this;
		} // operator =

		template <typename LAMBDA> // template instantiation is not needed, will be deduced (inferred):
		delegate& operator=(const LAMBDA& instance)
		{
			assign((void*)(&instance), lambda_stub<LAMBDA>);
			return *this;
		} // operator =

		bool operator==(const delegate& another) const { return invocation == another.invocation; }
		bool operator!=(const delegate& another) const { return invocation != another.invocation; }

		bool operator==(const multicast_delegate<RET(PARAMS...)>& another) const { return another == (*this); }
		bool operator!=(const multicast_delegate<RET(PARAMS...)>& another) const { return another != (*this); }

		template <class T>
		static delegate create(T* instance, RET (T::*method)(PARAMS...))
		{
			// TODO: Handle garbage collection
			member_proxy<T>* proxy = new member_proxy<T>{instance, method};
			return delegate(proxy, member_stub<T>);
		} // create

		template <class T>
		static delegate create(T const* instance, RET (T::*method)(PARAMS...) const)
		{
			// TODO: Handle garbage collection
			const_member_proxy<T>* proxy = new const_member_proxy<T>{instance, method};
			return delegate(proxy, const_member_stub<T>);
		} // create

		template <typename TFn, TFn func, class T>
		static delegate create(T* instance)
		{
			return delegate(instance, method_stub<T, func>);
		} // create

		template <typename TFn, TFn const func, class T>
		static delegate create(T const* instance)
		{
			return delegate(const_cast<T*>(instance), const_method_stub<T, func>);
		} // create

		template <class T, RET (T::*TMethod)(PARAMS...)>
		static delegate create(T* instance)
		{
			return delegate(instance, method_stub<T, TMethod>);
		} // create

		template <class T, RET (T::*TMethod)(PARAMS...) const>
		static delegate create(T const* instance)
		{
			return delegate(const_cast<T*>(instance), const_method_stub<T, TMethod>);
		} // create

		template <RET (*TMethod)(PARAMS...)>
		static delegate create()
		{
			return delegate(nullptr, function_stub<TMethod>);
		} // create

		template <typename LAMBDA>
		static delegate create(const LAMBDA& instance)
		{
			// TODO: Handle garbage collection
			return delegate((void*)(&instance), lambda_stub<LAMBDA>);
		} // create

		RET operator()(PARAMS... arg) const
		{
			return (*invocation.stub)(invocation.object, arg...);
		} // operator()

	  private:
		template <class TClass>
		struct member_proxy
		{
			using TMember = RET (TClass::*)(PARAMS...);
			TClass* instance;
			TMember member;
		};

		template <class TClass>
		struct const_member_proxy
		{
			using TConstMember = RET (TClass::*)(PARAMS...) const;
			TClass const* instance;
			TConstMember member;
		};

		delegate(void* anObject, typename delegate_base<RET(PARAMS...)>::stub_type aStub)
		{
			invocation.object = anObject;
			invocation.stub = aStub;
		} // delegate

		void assign(void* anObject, typename delegate_base<RET(PARAMS...)>::stub_type aStub)
		{
			this->invocation.object = anObject;
			this->invocation.stub = aStub;
		} // assign

		template <class T>
		static RET member_stub(void* this_ptr, PARAMS... params)
		{
			member_proxy<T>* p = static_cast<member_proxy<T>*>(this_ptr);
			return ((p->instance)->*(p->member))(params...);
		} // member_stub

		template <class T>
		static RET const_member_stub(void* this_ptr, PARAMS... params)
		{
			const_member_proxy<T>* p = static_cast<const_member_proxy<T>*>(this_ptr);
			return ((p->instance)->*(p->member))(params...);
		} // const_member_stub

		template <class T, RET (T::*TMethod)(PARAMS...)>
		static RET method_stub(void* this_ptr, PARAMS... params)
		{
			T* p = static_cast<T*>(this_ptr);
			return (p->*TMethod)(params...);
		} // method_stub

		template <class T, RET (T::*TMethod)(PARAMS...) const>
		static RET const_method_stub(void* this_ptr, PARAMS... params)
		{
			T* const p = static_cast<T*>(this_ptr);
			return (p->*TMethod)(params...);
		} // const_method_stub

		template <RET (*TMethod)(PARAMS...)>
		static RET function_stub(void* this_ptr, PARAMS... params)
		{
			return (TMethod)(params...);
		} // function_stub

		template <typename LAMBDA>
		static RET lambda_stub(void* this_ptr, PARAMS... arg)
		{
			LAMBDA* p = static_cast<LAMBDA*>(this_ptr);
			return (p->operator())(arg...);
		} // lambda_stub

		friend class multicast_delegate<RET(PARAMS...)>;
		typename delegate_base<RET(PARAMS...)>::InvocationElement invocation;

	}; // class delegate

} /* namespace SA */
