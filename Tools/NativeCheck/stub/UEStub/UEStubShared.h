#pragma once

// TSharedPtr / TSharedRef / TSharedFromThis stand-ins with the UE surface (Get, IsValid, ToSharedRef, ...).

#include "UEStub/UEStubCore.h"

#include <memory>
#include <type_traits>

template <class T>
class TSharedRef;

template <class T>
class TSharedPtr
{
public:
	TSharedPtr() = default;
	TSharedPtr(std::nullptr_t) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TSharedPtr(const TSharedPtr<U>& Other) : Impl(Other.Impl) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TSharedPtr(const TSharedRef<U>& Other);
	explicit TSharedPtr(std::shared_ptr<T> In) : Impl(std::move(In)) {}
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	explicit TSharedPtr(U* InObject) : Impl(InObject) {}

	T* Get() const { return Impl.get(); }
	bool IsValid() const { return Impl != nullptr; }
	explicit operator bool() const { return Impl != nullptr; }
	T* operator->() const { return Impl.get(); }
	T& operator*() const { return *Impl; }
	void Reset() { Impl.reset(); }
	TSharedRef<T> ToSharedRef() const;
	bool operator==(const TSharedPtr& Other) const { return Impl == Other.Impl; }
	bool operator!=(const TSharedPtr& Other) const { return Impl != Other.Impl; }
	bool operator==(std::nullptr_t) const { return Impl == nullptr; }
	bool operator!=(std::nullptr_t) const { return Impl != nullptr; }

	std::shared_ptr<T> Impl;
};

template <class T>
class TSharedRef
{
public:
	TSharedRef() = delete;
	template <class U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
	TSharedRef(const TSharedRef<U>& Other) : Impl(Other.Impl) {}
	explicit TSharedRef(std::shared_ptr<T> In) : Impl(std::move(In)) {}

	T& Get() const { return *Impl; }
	T* operator->() const { return Impl.get(); }
	T& operator*() const { return *Impl; }
	TSharedPtr<T> ToSharedPtr() const { return TSharedPtr<T>(Impl); }
	bool operator==(const TSharedRef& Other) const { return Impl == Other.Impl; }

	std::shared_ptr<T> Impl;
};

template <class T>
template <class U, typename>
TSharedPtr<T>::TSharedPtr(const TSharedRef<U>& Other) : Impl(Other.Impl)
{
}

template <class T>
TSharedRef<T> TSharedPtr<T>::ToSharedRef() const
{
	return TSharedRef<T>(Impl);
}

template <class T>
class TWeakPtr
{
public:
	TWeakPtr() = default;
	template <class U>
	TWeakPtr(const TSharedPtr<U>& Other) : Impl(Other.Impl) {}
	template <class U>
	TWeakPtr(const TSharedRef<U>& Other) : Impl(Other.Impl) {}
	TSharedPtr<T> Pin() const { return TSharedPtr<T>(Impl.lock()); }
	bool IsValid() const { return !Impl.expired(); }
	void Reset() { Impl.reset(); }

	std::weak_ptr<T> Impl;
};

template <class T>
class TSharedFromThis
{
public:
	TSharedRef<T> AsShared();
	TSharedRef<const T> AsShared() const;

protected:
	template <class OtherType>
	static TSharedRef<OtherType> SharedThis(OtherType* ThisPtr);
	template <class OtherType>
	static TSharedRef<const OtherType> SharedThis(const OtherType* ThisPtr);
};

template <class T, typename... TArgs>
TSharedRef<T> MakeShared(TArgs&&... Args)
{
	return TSharedRef<T>(std::make_shared<T>(std::forward<TArgs>(Args)...));
}

template <class T>
TSharedPtr<T> MakeShareable(T* Object)
{
	return TSharedPtr<T>(std::shared_ptr<T>(Object));
}

template <class To, class From>
TSharedPtr<To> StaticCastSharedPtr(const TSharedPtr<From>& In)
{
	return TSharedPtr<To>(std::static_pointer_cast<To>(In.Impl));
}

template <class To, class From>
TSharedRef<To> StaticCastSharedRef(const TSharedRef<From>& In)
{
	return TSharedRef<To>(std::static_pointer_cast<To>(In.Impl));
}
