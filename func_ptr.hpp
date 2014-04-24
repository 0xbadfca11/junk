#pragma once
#include <windows.h>
#include <cstddef>

template <typename Ty>
struct func_ptr
{
	func_ptr( _In_opt_ Ty* proc = nullptr )
		: value( PTR_MANGLE( proc ) )
	{}
	func_ptr( _In_ HMODULE module, _In_z_ PCSTR proc )
		: value( PTR_MANGLE( ::GetProcAddress( module, proc ) ) )
	{}
	func_ptr( _In_z_ PCWSTR module, _In_z_ PCSTR proc )
		: value( PTR_MANGLE( ::GetProcAddress( dlopen( module ), proc ) ) )
	{}
	func_ptr( _In_z_ PCSTR module, _In_z_ PCSTR proc )
		: value( PTR_MANGLE( ::GetProcAddress( dlopen( module ), proc ) ) )
	{}
	func_ptr& operator=( _In_opt_ void* proc )
	{
		value = PTR_MANGLE( proc );
		return *this;
	}
	Ty* get() const
	{
		return static_cast<Ty*>( ::DecodePointer( value ) );
	}
	operator Ty*( ) const
	{
		return get();
	}
	explicit operator bool() const
	{
		return value != PTR_MANGLE( nullptr );
	}
protected:
	Ty* value;
	_Check_return_ Ty* PTR_MANGLE( _In_opt_ void* proc ) const
	{
		return static_cast<Ty*>( ::EncodePointer( proc ) );
	}
	_Check_return_ HMODULE dlopen( _In_z_ PCWSTR module ) const
	{
		return ::LoadLibraryExW( module, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
	}
	_Check_return_ HMODULE dlopen( _In_z_ PCSTR module ) const
	{
		return ::LoadLibraryExA( module, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
	}
	template <typename Ty>
	friend bool operator==( const func_ptr<Ty>&, std::nullptr_t );
};
template <typename Ty>
bool operator==( const func_ptr<Ty>& left, std::nullptr_t )
{
	return left.value == ::EncodePointer( nullptr );
}
template <typename Ty>
bool operator!=( const func_ptr<Ty>& left, std::nullptr_t )
{
	return !( left == nullptr );
}
template <typename Ty>
bool operator==( std::nullptr_t, const func_ptr<Ty>& right )
{
	return right == nullptr;
}
template <typename Ty>
bool operator!=( std::nullptr_t, const func_ptr<Ty>& right )
{
	return !( right == nullptr );
}