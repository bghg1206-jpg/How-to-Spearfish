#pragma once

// Native stand-in for the subset of Unreal Engine *Core* used by How to Spearfish.
//
// Purpose: compile (and for Rules/, run) the game's C++ outside the editor so logic bugs and
// internal inconsistencies are caught in environments without an Unreal install (CI, cloud agents).
// It is intentionally STRICTER than UE in places (e.g. FMath::Max/Min/Clamp do not mix float and
// double) so code that passes here also compiles in UE. It does NOT prove engine API correctness.

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------------------------
// Basic types & macros
// ---------------------------------------------------------------------------------------------
using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using TCHAR = char;
using ANSICHAR = char;
using UTF8CHAR = char;
using SIZE_T = std::size_t;

#define TEXT(x) x
#define FORCEINLINE inline
#define FORCENOINLINE
#define UE_NODISCARD [[nodiscard]]
#define HOWTOSPEARFISH_API
#define ENGINE_API
#define CORE_API
#define NO_API

#define UE_SMALL_NUMBER (1.e-8f)
#define UE_KINDA_SMALL_NUMBER (1.e-4f)
#define UE_BIG_NUMBER (3.4e+38f)
#define UE_PI (3.1415926535897932f)
#define UE_TWO_PI (6.28318530717958647692f)
#define UE_HALF_PI (1.57079632679489661923f)
#define UE_DOUBLE_PI (3.141592653589793238462643383279502884197169399)
#define INDEX_NONE (-1)
#define MAX_int32 (2147483647)
#define MAX_uint8 (255)
#define MAX_flt (3.402823466e+38F)
#define UE_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define WITH_EDITOR 1
#define WITH_EDITORONLY_DATA 1
#ifndef WITH_DEV_AUTOMATION_TESTS
#define WITH_DEV_AUTOMATION_TESTS 0
#endif
#define UE_BUILD_SHIPPING 0
#define UE_BUILD_DEBUG 0
#define UE_SERVER 0
#define ENGINE_MAJOR_VERSION 5
#define ENGINE_MINOR_VERSION 5

inline void UEStub_CheckFail() {}
#define check(expr) ((void)(expr))
#define checkf(expr, ...) ((void)(expr), UEStub_FormatArgsUnused(__VA_ARGS__))
#define checkNoEntry() ((void)0)
#define verify(expr) ((void)(expr))
#define ensure(expr) (!!(expr))
#define ensureMsgf(expr, ...) ((!!(expr)) || UEStub_FormatArgsUnusedBool(__VA_ARGS__))
#define ensureAlways(expr) (!!(expr))
#define ensureAlwaysMsgf(expr, ...) ((!!(expr)) || UEStub_FormatArgsUnusedBool(__VA_ARGS__))

template <typename... TArgs>
inline void UEStub_FormatArgsUnused(TArgs&&...) {}
template <typename... TArgs>
inline bool UEStub_FormatArgsUnusedBool(TArgs&&...) { return false; }

template <typename T>
inline typename std::remove_reference<T>::type&& MoveTemp(T&& Obj) { return static_cast<typename std::remove_reference<T>::type&&>(Obj); }
template <typename T>
inline T CopyTemp(const T& Val) { return Val; }
template <typename T>
inline T&& Forward(typename std::remove_reference<T>::type& Obj) { return static_cast<T&&>(Obj); }

// --- Reflection macros (UHT) ---
#define UCLASS(...)
#define USTRUCT(...)
#define UENUM(...)
#define UINTERFACE(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define UMETA(...)
#define UPARAM(...)
#define UDELEGATE(...)
#define GENERATED_UINTERFACE_BODY() GENERATED_BODY()
#define GENERATED_IINTERFACE_BODY() GENERATED_BODY()
#define GENERATED_UCLASS_BODY() GENERATED_BODY()
#define GENERATED_USTRUCT_BODY() GENERATED_BODY()

// GENERATED_BODY works exactly like UE: the *.generated.h (produced by Tools/NativeCheck/gen_uht.py)
// defines CURRENT_FILE_ID and one FID_<File>_<Line>_GENERATED_BODY macro per GENERATED_BODY() line.
#define UESTUB_BODY_MACRO_COMBINE_INNER(A, B, C, D) A##B##C##D
#define UESTUB_BODY_MACRO_COMBINE(A, B, C, D) UESTUB_BODY_MACRO_COMBINE_INNER(A, B, C, D)
#define GENERATED_BODY(...) UESTUB_BODY_MACRO_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)

// ---------------------------------------------------------------------------------------------
// FString
// ---------------------------------------------------------------------------------------------
namespace ESearchCase
{
	enum Type
	{
		CaseSensitive,
		IgnoreCase
	};
}
namespace ESearchDir
{
	enum Type
	{
		FromStart,
		FromEnd
	};
}

class FString
{
public:
	std::string Data;

	FString() = default;
	FString(const char* In) : Data(In ? In : "") {}
	FString(const std::string& In) : Data(In) {}
	explicit FString(int32 Count, const char* In) : Data(In ? std::string(In, Count) : std::string()) {}

	const TCHAR* operator*() const { return Data.c_str(); }
	int32 Len() const { return static_cast<int32>(Data.size()); }
	bool IsEmpty() const { return Data.empty(); }
	void Empty() { Data.clear(); }
	void Reset() { Data.clear(); }

	FString& operator+=(const FString& Other) { Data += Other.Data; return *this; }
	FString& operator+=(const TCHAR* Other) { Data += Other; return *this; }
	FString& operator+=(TCHAR Other) { Data += Other; return *this; }
	FString& AppendChar(TCHAR C) { Data += C; return *this; }
	FString& Append(const FString& Other) { Data += Other.Data; return *this; }
	friend FString operator+(const FString& A, const FString& B) { return FString(A.Data + B.Data); }
	friend FString operator+(const FString& A, const TCHAR* B) { return FString(A.Data + B); }
	friend FString operator+(const TCHAR* A, const FString& B) { return FString(std::string(A) + B.Data); }
	bool operator==(const FString& Other) const { return Equals(Other, ESearchCase::IgnoreCase); }
	bool operator!=(const FString& Other) const { return !(*this == Other); }
	bool operator<(const FString& Other) const { return Data < Other.Data; }
	TCHAR operator[](int32 Index) const { return Data[static_cast<size_t>(Index)]; }

	bool Equals(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const
	{
		if (SearchCase == ESearchCase::CaseSensitive)
		{
			return Data == Other.Data;
		}
		if (Data.size() != Other.Data.size())
		{
			return false;
		}
		for (size_t Index = 0; Index < Data.size(); ++Index)
		{
			if (std::tolower(static_cast<unsigned char>(Data[Index])) != std::tolower(static_cast<unsigned char>(Other.Data[Index])))
			{
				return false;
			}
		}
		return true;
	}

	bool Contains(const FString& Sub, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type Dir = ESearchDir::FromStart) const
	{
		return SearchCase == ESearchCase::CaseSensitive ? Data.find(Sub.Data) != std::string::npos : ToLower().Data.find(Sub.ToLower().Data) != std::string::npos;
	}
	bool StartsWith(const FString& Prefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const { return Data.rfind(Prefix.Data, 0) == 0; }
	FString TrimStartAndEnd() const
	{
		const size_t Start = Data.find_first_not_of(" \t\r\n");
		if (Start == std::string::npos)
		{
			return FString();
		}
		const size_t End = Data.find_last_not_of(" \t\r\n");
		return FString(Data.substr(Start, End - Start + 1));
	}
	FString TrimStart() const { const size_t Start = Data.find_first_not_of(" \t\r\n"); return Start == std::string::npos ? FString() : FString(Data.substr(Start)); }
	FString TrimEnd() const { const size_t End = Data.find_last_not_of(" \t\r\n"); return End == std::string::npos ? FString() : FString(Data.substr(0, End + 1)); }
	bool IsNumeric() const { return !Data.empty() && std::all_of(Data.begin(), Data.end(), [](char C) { return std::isdigit(static_cast<unsigned char>(C)) || C == '.' || C == '-'; }); }
	bool EndsWith(const FString& Suffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return Data.size() >= Suffix.Data.size() && Data.compare(Data.size() - Suffix.Data.size(), Suffix.Data.size(), Suffix.Data) == 0;
	}
	FString Left(int32 Count) const { return FString(Data.substr(0, static_cast<size_t>(std::max(Count, 0)))); }
	FString Right(int32 Count) const
	{
		const int32 Start = std::max(Len() - Count, 0);
		return FString(Data.substr(static_cast<size_t>(Start)));
	}
	FString Mid(int32 Start, int32 Count = MAX_int32) const
	{
		if (Start >= Len())
		{
			return FString();
		}
		return FString(Data.substr(static_cast<size_t>(std::max(Start, 0)), static_cast<size_t>(std::max(Count, 0))));
	}
	FString ToUpper() const
	{
		std::string Out = Data;
		std::transform(Out.begin(), Out.end(), Out.begin(), [](unsigned char C) { return static_cast<char>(std::toupper(C)); });
		return FString(Out);
	}
	FString ToLower() const
	{
		std::string Out = Data;
		std::transform(Out.begin(), Out.end(), Out.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
		return FString(Out);
	}
	FString Replace(const TCHAR* From, const TCHAR* To) const
	{
		std::string Out = Data;
		const std::string F(From);
		const std::string T(To);
		if (F.empty())
		{
			return FString(Out);
		}
		size_t Pos = 0;
		while ((Pos = Out.find(F, Pos)) != std::string::npos)
		{
			Out.replace(Pos, F.size(), T);
			Pos += T.size();
		}
		return FString(Out);
	}
	FString LeftPad(int32 Count) const
	{
		std::string Out = Data;
		while (static_cast<int32>(Out.size()) < Count)
		{
			Out.insert(Out.begin(), ' ');
		}
		return FString(Out);
	}

	static FString FromInt(int32 Value) { return FString(std::to_string(Value)); }
	static FString SanitizeFloat(double Value, int32 = 1)
	{
		char Buffer[64];
		std::snprintf(Buffer, sizeof(Buffer), "%g", Value);
		return FString(Buffer);
	}
	static FString Chr(TCHAR C) { return FString(std::string(1, C)); }

	template <typename... TArgs>
	static FString Printf(const TCHAR* Fmt, TArgs... Args)
	{
		char Buffer[2048];
		std::snprintf(Buffer, sizeof(Buffer), Fmt, Args...);
		return FString(Buffer);
	}

	template <typename TElem>
	static FString Join(const TElem& Items, const TCHAR* Separator)
	{
		FString Out;
		bool bFirst = true;
		for (const auto& Item : Items)
		{
			if (!bFirst)
			{
				Out += Separator;
			}
			Out += Item;
			bFirst = false;
		}
		return Out;
	}
};

struct FCString
{
	static int32 Atoi(const TCHAR* String) { return std::atoi(String); }
	static int64 Atoi64(const TCHAR* String) { return std::atoll(String); }
	static float Atof(const TCHAR* String) { return static_cast<float>(std::atof(String)); }
	static double Atod(const TCHAR* String) { return std::atof(String); }
	static int32 Strlen(const TCHAR* String) { return static_cast<int32>(std::strlen(String)); }
	static int32 Strcmp(const TCHAR* A, const TCHAR* B) { return std::strcmp(A, B); }
	static int32 Stricmp(const TCHAR* A, const TCHAR* B);
};

inline uint32 GetTypeHash(const FString& S) { return static_cast<uint32>(std::hash<std::string>()(S.ToLower().Data)); }

// ---------------------------------------------------------------------------------------------
// FName (case-insensitive like UE)
// ---------------------------------------------------------------------------------------------
enum EName : int32
{
	NAME_None = 0
};

class FName
{
public:
	std::string Data;

	FName() = default;
	FName(EName) {}
	FName(const char* In) : Data(Normalize(In ? In : "")) {}
	FName(const FString& In) : Data(Normalize(In.Data)) {}

	bool IsNone() const { return Data.empty(); }
	bool IsValid() const { return true; }
	FString ToString() const { return FString(Data.empty() ? std::string("None") : Data); }
	bool operator==(const FName& Other) const { return Key() == Other.Key(); }
	bool operator!=(const FName& Other) const { return !(*this == Other); }
	bool operator<(const FName& Other) const { return Key() < Other.Key(); }
	bool LexicalLess(const FName& Other) const { return Key() < Other.Key(); }
	bool FastLess(const FName& Other) const { return Key() < Other.Key(); }

	std::string Key() const
	{
		std::string Out = Data;
		std::transform(Out.begin(), Out.end(), Out.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
		return Out;
	}

private:
	static std::string Normalize(const std::string& In) { return In == "None" ? std::string() : In; }
};

inline uint32 GetTypeHash(const FName& N) { return static_cast<uint32>(std::hash<std::string>()(N.Key())); }
inline uint32 GetTypeHash(int32 V) { return static_cast<uint32>(V); }
inline uint32 GetTypeHash(uint32 V) { return V; }
inline uint32 GetTypeHash(uint8 V) { return V; }
inline uint32 GetTypeHash(const void* P) { return static_cast<uint32>(reinterpret_cast<std::uintptr_t>(P)); }
inline uint32 HashCombine(uint32 A, uint32 B) { return A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)); }

// ---------------------------------------------------------------------------------------------
// FText (thin wrapper)
// ---------------------------------------------------------------------------------------------
struct FFormatNamedArguments;
class FText
{
public:
	FString Str;

	FText() = default;
	static FText FromString(const FString& In) { FText T; T.Str = In; return T; }
	static FText FromName(const FName& In) { return FromString(In.ToString()); }
	// UE overloads AsNumber for every integer and floating point type.
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	static FText AsNumber(T V) { return FromString(FString::SanitizeFloat(static_cast<double>(V))); }
	static const FText& GetEmpty() { static FText Empty; return Empty; }
	static FText Format(const FText& Fmt, const FFormatNamedArguments&) { return Fmt; }
	// Like UE: ordered arguments must convert to FFormatArgumentValue (FText or numbers; FString/FName do not).
	template <typename... TArgs>
	static FText Format(const FText& Fmt, TArgs&&... Args);
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	static FText AsPercent(T V) { return FromString(FString::SanitizeFloat(static_cast<double>(V) * 100.0) + "%"); }
	template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
	static FText AsCurrency(T V) { return FromString(FString::SanitizeFloat(static_cast<double>(V))); }
	static FText AsCultureInvariant(const FString& In) { return FromString(In); }
	FText ToUpper() const { return FromString(Str.ToUpper()); }
	FText ToLower() const { return FromString(Str.ToLower()); }
	bool IsEmptyOrWhitespace() const { return Str.IsEmpty(); }
	const FString& ToString() const { return Str; }
	bool IsEmpty() const { return Str.IsEmpty(); }
	bool EqualTo(const FText& Other) const { return Str == Other.Str; }
};
struct FFormatArgumentValue
{
	FFormatArgumentValue(const FText& In) {}
	FFormatArgumentValue(int32 In) {}
	FFormatArgumentValue(uint32 In) {}
	FFormatArgumentValue(int64 In) {}
	FFormatArgumentValue(uint64 In) {}
	FFormatArgumentValue(float In) {}
	FFormatArgumentValue(double In) {}
};
template <typename... TArgs>
FText FText::Format(const FText& Fmt, TArgs&&... Args)
{
	const FFormatArgumentValue Values[] = { FFormatArgumentValue(Args)..., FFormatArgumentValue(0) };
	(void)Values;
	return Fmt;
}
#define LOCTEXT(Key, Text) FText::FromString(FString(Text))
#define NSLOCTEXT(Ns, Key, Text) FText::FromString(FString(Text))
#define INVTEXT(Text) FText::FromString(FString(Text))

#define ANSI_TO_TCHAR(x) (x)
#define TCHAR_TO_ANSI(x) (x)
#define TCHAR_TO_UTF8(x) (x)
#define UTF8_TO_TCHAR(x) (x)

// ---------------------------------------------------------------------------------------------
// Containers
// ---------------------------------------------------------------------------------------------
enum class EAllowShrinking : uint8
{
	No,
	Yes
};

template <typename T, typename TAllocator = void>
class TArray
{
public:
	using ElementType = T;
	// std::vector<bool> is bit-packed and cannot hand out bool&; UE's TArray<bool> can.
	using StorageType = typename std::conditional<std::is_same<T, bool>::value, std::deque<bool>, std::vector<T>>::type;
	StorageType Data;

	TArray() = default;
	TArray(std::initializer_list<T> List) : Data(List) {}
	TArray(const T* Ptr, int32 Count) : Data(Ptr, Ptr + Count) {}

	int32 Num() const { return static_cast<int32>(Data.size()); }
	bool IsEmpty() const { return Data.empty(); }
	bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < Num(); }
	T& operator[](int32 Index) { return Data.at(static_cast<size_t>(Index)); }
	const T& operator[](int32 Index) const { return Data.at(static_cast<size_t>(Index)); }
	T* GetData() { return Data.data(); }
	const T* GetData() const { return Data.data(); }

	int32 Add(const T& Item) { Data.push_back(Item); return Num() - 1; }
	int32 Add(T&& Item) { Data.push_back(std::move(Item)); return Num() - 1; }
	template <typename... TArgs>
	int32 Emplace(TArgs&&... Args) { Data.emplace_back(std::forward<TArgs>(Args)...); return Num() - 1; }
	template <typename... TArgs>
	T& Emplace_GetRef(TArgs&&... Args) { Data.emplace_back(std::forward<TArgs>(Args)...); return Data.back(); }
	T& Add_GetRef(const T& Item) { Data.push_back(Item); return Data.back(); }
	int32 AddDefaulted(int32 Count = 1) { const int32 Start = Num(); Data.resize(Data.size() + static_cast<size_t>(Count)); return Start; }
	T& AddDefaulted_GetRef() { Data.emplace_back(); return Data.back(); }
	int32 AddZeroed(int32 Count = 1) { return AddDefaulted(Count); }
	int32 AddUnique(const T& Item)
	{
		const int32 Existing = Find(Item);
		return Existing != INDEX_NONE ? Existing : Add(Item);
	}
	void Append(const TArray& Other) { Data.insert(Data.end(), Other.Data.begin(), Other.Data.end()); }
	void Append(std::initializer_list<T> List) { Data.insert(Data.end(), List.begin(), List.end()); }
	void Insert(const T& Item, int32 Index) { Data.insert(Data.begin() + Index, Item); }
	void RemoveAt(int32 Index, int32 Count = 1, EAllowShrinking = EAllowShrinking::Yes)
	{
		Data.erase(Data.begin() + Index, Data.begin() + Index + Count);
	}
	void RemoveAtSwap(int32 Index, int32 Count = 1, EAllowShrinking = EAllowShrinking::Yes)
	{
		for (int32 I = 0; I < Count; ++I)
		{
			std::swap(Data[static_cast<size_t>(Index)], Data.back());
			Data.pop_back();
		}
	}
	int32 Remove(const T& Item)
	{
		const size_t Before = Data.size();
		Data.erase(std::remove(Data.begin(), Data.end(), Item), Data.end());
		return static_cast<int32>(Before - Data.size());
	}
	int32 RemoveSingle(const T& Item)
	{
		const int32 Index = Find(Item);
		if (Index == INDEX_NONE)
		{
			return 0;
		}
		RemoveAt(Index);
		return 1;
	}
	int32 RemoveSwap(const T& Item) { return Remove(Item); }
	template <typename P>
	int32 RemoveAll(P Pred)
	{
		const size_t Before = Data.size();
		Data.erase(std::remove_if(Data.begin(), Data.end(), [&Pred](const T& Item) { return Pred(Item); }), Data.end());
		return static_cast<int32>(Before - Data.size());
	}
	template <typename P>
	int32 RemoveAllSwap(P Pred) { return RemoveAll(Pred); }
	void Reset(int32 = 0) { Data.clear(); }
	void Empty(int32 = 0) { Data.clear(); }
	void Reserve(int32 Count) { Data.reserve(static_cast<size_t>(Count)); }
	void Init(const T& Value, int32 Count) { Data.assign(static_cast<size_t>(Count), Value); }
	void SetNum(int32 Count, EAllowShrinking = EAllowShrinking::Yes) { Data.resize(static_cast<size_t>(Count)); }
	void SetNumZeroed(int32 Count) { Data.resize(static_cast<size_t>(Count)); }
	void Shrink() {}

	bool Contains(const T& Item) const { return std::find(Data.begin(), Data.end(), Item) != Data.end(); }
	template <typename P>
	bool ContainsByPredicate(P Pred) const { return std::any_of(Data.begin(), Data.end(), [&Pred](const T& Item) { return Pred(Item); }); }
	int32 Find(const T& Item) const
	{
		const auto It = std::find(Data.begin(), Data.end(), Item);
		return It == Data.end() ? INDEX_NONE : static_cast<int32>(It - Data.begin());
	}
	bool Find(const T& Item, int32& OutIndex) const { OutIndex = Find(Item); return OutIndex != INDEX_NONE; }
	template <typename P>
	T* FindByPredicate(P Pred)
	{
		for (T& Item : Data)
		{
			if (Pred(Item))
			{
				return &Item;
			}
		}
		return nullptr;
	}
	template <typename P>
	const T* FindByPredicate(P Pred) const
	{
		for (const T& Item : Data)
		{
			if (Pred(Item))
			{
				return &Item;
			}
		}
		return nullptr;
	}
	template <typename P>
	int32 IndexOfByPredicate(P Pred) const
	{
		for (int32 Index = 0; Index < Num(); ++Index)
		{
			if (Pred(Data[static_cast<size_t>(Index)]))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
	template <typename P>
	TArray FilterByPredicate(P Pred) const
	{
		TArray Out;
		for (const T& Item : Data)
		{
			if (Pred(Item))
			{
				Out.Add(Item);
			}
		}
		return Out;
	}

	T& Last(int32 IndexFromEnd = 0) { return Data.at(Data.size() - 1 - static_cast<size_t>(IndexFromEnd)); }
	const T& Last(int32 IndexFromEnd = 0) const { return Data.at(Data.size() - 1 - static_cast<size_t>(IndexFromEnd)); }
	T& Top() { return Last(); }
	T Pop(EAllowShrinking = EAllowShrinking::Yes)
	{
		T Out = std::move(Data.back());
		Data.pop_back();
		return Out;
	}
	void Push(const T& Item) { Add(Item); }
	void Swap(int32 A, int32 B) { std::swap(Data[static_cast<size_t>(A)], Data[static_cast<size_t>(B)]); }

	// UE dereferences pointer elements before handing them to sort predicates.
	template <typename P>
	void Sort(P Pred)
	{
		if constexpr (std::is_pointer<T>::value)
		{
			std::sort(Data.begin(), Data.end(), [&Pred](const T A, const T B) { return Pred(*A, *B); });
		}
		else
		{
			std::sort(Data.begin(), Data.end(), [&Pred](const T& A, const T& B) { return Pred(A, B); });
		}
	}
	void Sort() { std::sort(Data.begin(), Data.end()); }
	template <typename P>
	void StableSort(P Pred)
	{
		if constexpr (std::is_pointer<T>::value)
		{
			std::stable_sort(Data.begin(), Data.end(), [&Pred](const T A, const T B) { return Pred(*A, *B); });
		}
		else
		{
			std::stable_sort(Data.begin(), Data.end(), [&Pred](const T& A, const T& B) { return Pred(A, B); });
		}
	}

	bool operator==(const TArray& Other) const { return Data == Other.Data; }
	bool operator!=(const TArray& Other) const { return Data != Other.Data; }

	auto begin() { return Data.begin(); }
	auto end() { return Data.end(); }
	auto begin() const { return Data.begin(); }
	auto end() const { return Data.end(); }
};

template <typename T, uint32 N>
using TInlineAllocatorStub = void;
template <uint32 N>
class TInlineAllocator
{
};

template <typename K, typename V>
struct TPair
{
	K Key;
	V Value;
	TPair() = default;
	TPair(const K& InKey, const V& InValue) : Key(InKey), Value(InValue) {}
};
template <typename K, typename V>
using TTuple = TPair<K, V>;

template <typename T>
struct UEStubHasher
{
	size_t operator()(const T& V) const { return static_cast<size_t>(GetTypeHash(V)); }
};
template <typename T>
struct UEStubEq
{
	bool operator()(const T& A, const T& B) const { return A == B; }
};

template <typename K, typename V>
class TMap
{
public:
	using ElementType = TPair<K, V>;
	std::vector<TPair<K, V>> Pairs;

	int32 Num() const { return static_cast<int32>(Pairs.size()); }
	bool IsEmpty() const { return Pairs.empty(); }
	V& Add(const K& Key, const V& Value)
	{
		if (V* Existing = Find(Key))
		{
			*Existing = Value;
			return *Existing;
		}
		Pairs.emplace_back(Key, Value);
		return Pairs.back().Value;
	}
	V& Add(const K& Key) { return FindOrAdd(Key); }
	V& Emplace(const K& Key, const V& Value) { return Add(Key, Value); }
	V& FindOrAdd(const K& Key)
	{
		if (V* Existing = Find(Key))
		{
			return *Existing;
		}
		Pairs.emplace_back(Key, V());
		return Pairs.back().Value;
	}
	V* Find(const K& Key)
	{
		for (auto& Pair : Pairs)
		{
			if (Pair.Key == Key)
			{
				return &Pair.Value;
			}
		}
		return nullptr;
	}
	const V* Find(const K& Key) const
	{
		for (const auto& Pair : Pairs)
		{
			if (Pair.Key == Key)
			{
				return &Pair.Value;
			}
		}
		return nullptr;
	}
	V FindRef(const K& Key) const
	{
		const V* Found = Find(Key);
		return Found ? *Found : V();
	}
	V& FindChecked(const K& Key) { return *Find(Key); }
	const V& FindChecked(const K& Key) const { return *Find(Key); }
	bool Contains(const K& Key) const { return Find(Key) != nullptr; }
	int32 Remove(const K& Key)
	{
		const size_t Before = Pairs.size();
		Pairs.erase(std::remove_if(Pairs.begin(), Pairs.end(), [&Key](const TPair<K, V>& P) { return P.Key == Key; }), Pairs.end());
		return static_cast<int32>(Before - Pairs.size());
	}
	void Reset() { Pairs.clear(); }
	void Empty(int32 = 0) { Pairs.clear(); }
	V& operator[](const K& Key) { return FindChecked(Key); }
	const V& operator[](const K& Key) const { return FindChecked(Key); }
	template <typename TAlloc>
	int32 GetKeys(TArray<K, TAlloc>& OutKeys) const
	{
		OutKeys.Reset();
		for (const auto& Pair : Pairs)
		{
			OutKeys.Add(Pair.Key);
		}
		return OutKeys.Num();
	}
	template <typename TAlloc>
	void GenerateValueArray(TArray<V, TAlloc>& Out) const
	{
		Out.Reset();
		for (const auto& Pair : Pairs)
		{
			Out.Add(Pair.Value);
		}
	}
	auto begin() { return Pairs.begin(); }
	auto end() { return Pairs.end(); }
	auto begin() const { return Pairs.begin(); }
	auto end() const { return Pairs.end(); }
};

template <typename T>
class TSet
{
public:
	std::vector<T> Items;
	int32 Num() const { return static_cast<int32>(Items.size()); }
	void Add(const T& Item)
	{
		if (!Contains(Item))
		{
			Items.push_back(Item);
		}
	}
	bool Contains(const T& Item) const { return std::find(Items.begin(), Items.end(), Item) != Items.end(); }
	int32 Remove(const T& Item)
	{
		const size_t Before = Items.size();
		Items.erase(std::remove(Items.begin(), Items.end(), Item), Items.end());
		return static_cast<int32>(Before - Items.size());
	}
	void Reset() { Items.clear(); }
	void Empty() { Items.clear(); }
	TArray<T> Array() const
	{
		TArray<T> Out;
		for (const T& Item : Items)
		{
			Out.Add(Item);
		}
		return Out;
	}
	auto begin() { return Items.begin(); }
	auto end() { return Items.end(); }
	auto begin() const { return Items.begin(); }
	auto end() const { return Items.end(); }
};

template <typename T>
class TOptional
{
public:
	TOptional() = default;
	TOptional(const T& In) : Value(In), bSet(true) {}
	bool IsSet() const { return bSet; }
	const T& GetValue() const { return Value; }
	T& GetValue() { return Value; }
	const T& Get(const T& Default) const { return bSet ? Value : Default; }
	void Reset() { bSet = false; }
	void Emplace(const T& In) { Value = In; bSet = true; }
	explicit operator bool() const { return bSet; }
	const T* operator->() const { return &Value; }
	const T& operator*() const { return Value; }

private:
	T Value{};
	bool bSet = false;
};

template <typename T>
using TFunction = std::function<T>;
template <typename T>
using TFunctionRef = std::function<T>;
template <typename T>
using TUniquePtr = std::unique_ptr<T>;
template <typename T, typename... TArgs>
std::unique_ptr<T> MakeUnique(TArgs&&... Args) { return std::make_unique<T>(std::forward<TArgs>(Args)...); }

// ---------------------------------------------------------------------------------------------
// Math
// ---------------------------------------------------------------------------------------------
struct FVector;
struct FRotator;
struct FQuat;

struct FMath
{
	// Single-type templates on purpose: mixing float and double must be explicit.
	template <typename T>
	static constexpr T Max(const T A, const T B) { return (B < A) ? A : B; }
	template <typename T>
	static constexpr T Min(const T A, const T B) { return (A < B) ? A : B; }
	template <typename T>
	static constexpr T Max3(const T A, const T B, const T C) { return Max(Max(A, B), C); }
	template <typename T>
	static constexpr T Min3(const T A, const T B, const T C) { return Min(Min(A, B), C); }
	template <typename T>
	static constexpr T Clamp(const T X, const T MinV, const T MaxV) { return X < MinV ? MinV : (X < MaxV ? X : MaxV); }
	template <typename T>
	static constexpr T Abs(const T A) { return A < T(0) ? -A : A; }
	template <typename T>
	static constexpr T Sign(const T A) { return A > T(0) ? T(1) : (A < T(0) ? T(-1) : T(0)); }
	template <typename T>
	static constexpr T Square(const T A) { return A * A; }
	template <typename T, typename U>
	static T Lerp(const T& A, const T& B, const U& Alpha) { return static_cast<T>(A + (B - A) * Alpha); }

	static float Sqrt(float V) { return std::sqrt(V); }
	static double Sqrt(double V) { return std::sqrt(V); }
	static float InvSqrt(float V) { return 1.f / std::sqrt(V); }
	static float Pow(float A, float B) { return std::pow(A, B); }
	static double Pow(double A, double B) { return std::pow(A, B); }
	static float Exp(float V) { return std::exp(V); }
	static double Exp(double V) { return std::exp(V); }
	static float Loge(float V) { return std::log(V); }
	static float Sin(float V) { return std::sin(V); }
	static double Sin(double V) { return std::sin(V); }
	static float Cos(float V) { return std::cos(V); }
	static double Cos(double V) { return std::cos(V); }
	static float Tan(float V) { return std::tan(V); }
	static float Acos(float V) { return std::acos(Clamp(V, -1.f, 1.f)); }
	static double Acos(double V) { return std::acos(Clamp(V, -1.0, 1.0)); }
	static float Asin(float V) { return std::asin(Clamp(V, -1.f, 1.f)); }
	static float Atan(float V) { return std::atan(V); }
	static float Atan2(float Y, float X) { return std::atan2(Y, X); }
	static double Atan2(double Y, double X) { return std::atan2(Y, X); }
	static float Fmod(float X, float Y) { return std::fmod(X, Y); }
	static double Fmod(double X, double Y) { return std::fmod(X, Y); }
	static float Frac(float V) { return V - std::floor(V); }
	static int32 RoundToInt(float V) { return static_cast<int32>(std::floor(V + 0.5f)); }
	static int32 RoundToInt(double V) { return static_cast<int32>(std::floor(V + 0.5)); }
	static int32 FloorToInt(float V) { return static_cast<int32>(std::floor(V)); }
	static int32 FloorToInt(double V) { return static_cast<int32>(std::floor(V)); }
	static int32 CeilToInt(float V) { return static_cast<int32>(std::ceil(V)); }
	static int32 CeilToInt(double V) { return static_cast<int32>(std::ceil(V)); }
	static int32 TruncToInt(float V) { return static_cast<int32>(V); }
	static int32 TruncToInt(double V) { return static_cast<int32>(V); }
	static float FloorToFloat(float V) { return std::floor(V); }
	static double FloorToDouble(double V) { return std::floor(V); }
	static double CeilToDouble(double V) { return std::ceil(V); }
	static double RoundToDouble(double V) { return std::round(V); }
	static float RoundToFloat(float V) { return std::floor(V + 0.5f); }
	static float GridSnap(float V, float Grid) { return Grid == 0.f ? V : std::floor((V + 0.5f * Grid) / Grid) * Grid; }
	static bool IsNearlyZero(float V, float Tol = UE_SMALL_NUMBER) { return std::fabs(V) <= Tol; }
	static bool IsNearlyZero(double V, double Tol = UE_SMALL_NUMBER) { return std::fabs(V) <= Tol; }
	static bool IsNearlyEqual(float A, float B, float Tol = UE_SMALL_NUMBER) { return std::fabs(A - B) <= Tol; }
	static bool IsNearlyEqual(double A, double B, double Tol = UE_SMALL_NUMBER) { return std::fabs(A - B) <= Tol; }
	static bool IsFinite(float V) { return std::isfinite(V); }
	static float DegreesToRadians(float V) { return V * (UE_PI / 180.f); }
	static double DegreesToRadians(double V) { return V * (UE_DOUBLE_PI / 180.0); }
	static float RadiansToDegrees(float V) { return V * (180.f / UE_PI); }
	static double RadiansToDegrees(double V) { return V * (180.0 / UE_DOUBLE_PI); }
	static float SmoothStep(float A, float B, float X)
	{
		if (X < A)
		{
			return 0.f;
		}
		if (X >= B)
		{
			return 1.f;
		}
		const float F = (X - A) / (B - A);
		return F * F * (3.f - 2.f * F);
	}
	static float FInterpTo(float Current, float Target, float DeltaTime, float Speed)
	{
		if (Speed <= 0.f)
		{
			return Target;
		}
		const float Dist = Target - Current;
		if (Dist * Dist < UE_SMALL_NUMBER)
		{
			return Target;
		}
		return Current + Dist * Clamp(DeltaTime * Speed, 0.f, 1.f);
	}
	static float FInterpConstantTo(float Current, float Target, float DeltaTime, float Speed)
	{
		const float Dist = Target - Current;
		const float Step = Speed * DeltaTime;
		return std::fabs(Dist) <= Step ? Target : Current + (Dist > 0.f ? Step : -Step);
	}
	static FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float Speed);
	static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float Speed);
	static FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float Speed);
	static FRotator RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float Speed);
	static float FRand() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
	static float FRandRange(float A, float B) { return A + (B - A) * FRand(); }
	static int32 RandRange(int32 A, int32 B) { return A + (B > A ? std::rand() % (B - A + 1) : 0); }
	static int32 RandHelper(int32 A) { return A > 0 ? std::rand() % A : 0; }
	static int32 Rand() { return std::rand(); }
	static bool RandBool() { return (std::rand() & 1) == 1; }
	static FVector VRand();
	static FVector VRandCone(const FVector& Dir, float ConeHalfAngleRad);
	static float PerlinNoise1D(float X) { return std::sin(X * 1.7f) * 0.5f + std::sin(X * 0.37f) * 0.5f; }
	static float UnwindDegrees(float A)
	{
		while (A > 180.f)
		{
			A -= 360.f;
		}
		while (A < -180.f)
		{
			A += 360.f;
		}
		return A;
	}
	static double UnwindDegrees(double A)
	{
		while (A > 180.0)
		{
			A -= 360.0;
		}
		while (A < -180.0)
		{
			A += 360.0;
		}
		return A;
	}
	static float FindDeltaAngleDegrees(float A1, float A2) { return UnwindDegrees(A2 - A1); }
	static double FindDeltaAngleDegrees(double A1, double A2) { return UnwindDegrees(A2 - A1); }
};

/** Deterministic stream identical to UE's FRandomStream algorithm. */
struct FRandomStream
{
	FRandomStream() : InitialSeed(0), Seed(0) {}
	explicit FRandomStream(int32 InSeed) { Initialize(InSeed); }
	void Initialize(int32 InSeed)
	{
		InitialSeed = InSeed;
		Seed = static_cast<uint32>(InSeed);
	}
	void Reset() { Seed = static_cast<uint32>(InitialSeed); }
	int32 GetInitialSeed() const { return InitialSeed; }
	int32 GetCurrentSeed() const { return static_cast<int32>(Seed); }
	void GenerateNewSeed() { Initialize(std::rand()); }
	float GetFraction() const
	{
		MutateSeed();
		float Result;
		const uint32 Bits = 0x3F800000U | (Seed >> 9);
		std::memcpy(&Result, &Bits, sizeof(float));
		return Result - 1.0f;
	}
	float FRand() const { return GetFraction(); }
	uint32 GetUnsignedInt() const
	{
		MutateSeed();
		return Seed;
	}
	int32 RandHelper(int32 A) const { return A > 0 ? FMath::Min(static_cast<int32>(GetFraction() * static_cast<float>(A)), A - 1) : 0; }
	int32 RandRange(int32 Min, int32 Max) const
	{
		const int32 Range = (Max - Min) + 1;
		return Min + RandHelper(Range);
	}
	float FRandRange(float InMin, float InMax) const { return InMin + (InMax - InMin) * FRand(); }
	bool RandBool() const { return RandRange(0, 1) == 1; }
	FVector VRand() const;
	FVector GetUnitVector() const;

private:
	void MutateSeed() const { Seed = (Seed * 196314165U) + 907633515U; }
	int32 InitialSeed;
	mutable uint32 Seed;
};

struct FVector2D
{
	double X = 0.0;
	double Y = 0.0;
	FVector2D() = default;
	FVector2D(double InX, double InY) : X(InX), Y(InY) {}
	explicit FVector2D(double V) : X(V), Y(V) {}
	FVector2D operator+(const FVector2D& O) const { return FVector2D(X + O.X, Y + O.Y); }
	FVector2D operator-(const FVector2D& O) const { return FVector2D(X - O.X, Y - O.Y); }
	FVector2D operator*(double S) const { return FVector2D(X * S, Y * S); }
	FVector2D operator/(double S) const { return FVector2D(X / S, Y / S); }
	double Size() const { return std::sqrt(X * X + Y * Y); }
	double SizeSquared() const { return X * X + Y * Y; }
	FVector2D GetSafeNormal() const
	{
		const double S = Size();
		return S > 1e-8 ? FVector2D(X / S, Y / S) : FVector2D();
	}
	static const FVector2D ZeroVector;
	static const FVector2D UnitVector;
	static double DotProduct(const FVector2D& A, const FVector2D& B) { return A.X * B.X + A.Y * B.Y; }
	static double Distance(const FVector2D& A, const FVector2D& B) { return (A - B).Size(); }
	bool Equals(const FVector2D& O, double Tol = UE_KINDA_SMALL_NUMBER) const { return std::fabs(X - O.X) <= Tol && std::fabs(Y - O.Y) <= Tol; }
	bool IsNearlyZero(double Tol = UE_KINDA_SMALL_NUMBER) const { return std::fabs(X) <= Tol && std::fabs(Y) <= Tol; }
	FString ToString() const { return FString::Printf("X=%.3f Y=%.3f", X, Y); }
};
inline const FVector2D FVector2D::ZeroVector(0.0, 0.0);
inline const FVector2D FVector2D::UnitVector(1.0, 1.0);

struct FIntPoint
{
	int32 X = 0;
	int32 Y = 0;
	FIntPoint() = default;
	FIntPoint(int32 InX, int32 InY) : X(InX), Y(InY) {}
	bool operator==(const FIntPoint& O) const { return X == O.X && Y == O.Y; }
};
inline uint32 GetTypeHash(const FIntPoint& P) { return HashCombine(static_cast<uint32>(P.X), static_cast<uint32>(P.Y)); }

struct FIntVector
{
	int32 X = 0;
	int32 Y = 0;
	int32 Z = 0;
	FIntVector() = default;
	FIntVector(int32 InX, int32 InY, int32 InZ) : X(InX), Y(InY), Z(InZ) {}
	bool operator==(const FIntVector& O) const { return X == O.X && Y == O.Y && Z == O.Z; }
};
inline uint32 GetTypeHash(const FIntVector& P) { return HashCombine(HashCombine(static_cast<uint32>(P.X), static_cast<uint32>(P.Y)), static_cast<uint32>(P.Z)); }

struct FVector
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;

	FVector() = default;
	FVector(double InX, double InY, double InZ) : X(InX), Y(InY), Z(InZ) {}
	explicit FVector(double V) : X(V), Y(V), Z(V) {}
	explicit FVector(const FVector2D& V, double InZ) : X(V.X), Y(V.Y), Z(InZ) {}

	FVector operator+(const FVector& O) const { return FVector(X + O.X, Y + O.Y, Z + O.Z); }
	FVector operator-(const FVector& O) const { return FVector(X - O.X, Y - O.Y, Z - O.Z); }
	FVector operator-() const { return FVector(-X, -Y, -Z); }
	FVector operator*(double S) const { return FVector(X * S, Y * S, Z * S); }
	FVector operator*(const FVector& O) const { return FVector(X * O.X, Y * O.Y, Z * O.Z); }
	FVector operator/(double S) const { return FVector(X / S, Y / S, Z / S); }
	FVector& operator+=(const FVector& O) { X += O.X; Y += O.Y; Z += O.Z; return *this; }
	FVector& operator-=(const FVector& O) { X -= O.X; Y -= O.Y; Z -= O.Z; return *this; }
	FVector& operator*=(double S) { X *= S; Y *= S; Z *= S; return *this; }
	FVector& operator/=(double S) { X /= S; Y /= S; Z /= S; return *this; }
	bool operator==(const FVector& O) const { return X == O.X && Y == O.Y && Z == O.Z; }
	bool operator!=(const FVector& O) const { return !(*this == O); }
	double operator|(const FVector& O) const { return X * O.X + Y * O.Y + Z * O.Z; }
	FVector operator^(const FVector& O) const { return FVector(Y * O.Z - Z * O.Y, Z * O.X - X * O.Z, X * O.Y - Y * O.X); }
	double& operator[](int32 I) { return I == 0 ? X : (I == 1 ? Y : Z); }
	double operator[](int32 I) const { return I == 0 ? X : (I == 1 ? Y : Z); }

	double Size() const { return std::sqrt(X * X + Y * Y + Z * Z); }
	double Length() const { return Size(); }
	double SizeSquared() const { return X * X + Y * Y + Z * Z; }
	double SquaredLength() const { return SizeSquared(); }
	double Size2D() const { return std::sqrt(X * X + Y * Y); }
	double SizeSquared2D() const { return X * X + Y * Y; }
	bool IsNearlyZero(double Tol = UE_KINDA_SMALL_NUMBER) const { return std::fabs(X) <= Tol && std::fabs(Y) <= Tol && std::fabs(Z) <= Tol; }
	bool IsZero() const { return X == 0.0 && Y == 0.0 && Z == 0.0; }
	bool Equals(const FVector& O, double Tol = UE_KINDA_SMALL_NUMBER) const { return std::fabs(X - O.X) <= Tol && std::fabs(Y - O.Y) <= Tol && std::fabs(Z - O.Z) <= Tol; }
	bool Normalize(double Tol = UE_SMALL_NUMBER)
	{
		const double S = SizeSquared();
		if (S > Tol)
		{
			const double Inv = 1.0 / std::sqrt(S);
			X *= Inv;
			Y *= Inv;
			Z *= Inv;
			return true;
		}
		return false;
	}
	FVector GetSafeNormal(double Tol = UE_SMALL_NUMBER, const FVector& ResultIfZero = FVector(0.0)) const
	{
		const double S = SizeSquared();
		if (S <= Tol)
		{
			return ResultIfZero;
		}
		return *this / std::sqrt(S);
	}
	FVector GetSafeNormal2D(double Tol = UE_SMALL_NUMBER, const FVector& ResultIfZero = FVector(0.0)) const
	{
		const double S = X * X + Y * Y;
		if (S <= Tol)
		{
			return ResultIfZero;
		}
		const double Inv = 1.0 / std::sqrt(S);
		return FVector(X * Inv, Y * Inv, 0.0);
	}
	FVector GetUnsafeNormal() const { return *this / Size(); }
	FVector GetClampedToMaxSize(double MaxSize) const
	{
		const double S = Size();
		return S > MaxSize && S > 0.0 ? *this * (MaxSize / S) : *this;
	}
	FVector GetClampedToSize(double Min, double Max) const
	{
		const double S = Size();
		if (S <= 0.0)
		{
			return *this;
		}
		const double C = std::clamp(S, Min, Max);
		return *this * (C / S);
	}
	FVector GetAbs() const { return FVector(std::fabs(X), std::fabs(Y), std::fabs(Z)); }
	double GetMax() const { return std::max(X, std::max(Y, Z)); }
	double GetMin() const { return std::min(X, std::min(Y, Z)); }
	FVector ProjectOnTo(const FVector& A) const { return A * ((*this | A) / (A | A)); }
	FVector ProjectOnToNormal(const FVector& N) const { return N * (*this | N); }
	FRotator Rotation() const;
	FRotator ToOrientationRotator() const;
	FQuat ToOrientationQuat() const;
	FVector2D UnitCartesianToSpherical() const;
	FString ToString() const { return FString::Printf("X=%.3f Y=%.3f Z=%.3f", X, Y, Z); }
	FString ToCompactString() const { return FString::Printf("X=%.2f Y=%.2f Z=%.2f", X, Y, Z); }

	static double DotProduct(const FVector& A, const FVector& B) { return A | B; }
	static FVector CrossProduct(const FVector& A, const FVector& B) { return A ^ B; }
	static double Dist(const FVector& A, const FVector& B) { return (A - B).Size(); }
	static double Distance(const FVector& A, const FVector& B) { return (A - B).Size(); }
	static double DistSquared(const FVector& A, const FVector& B) { return (A - B).SizeSquared(); }
	static double Dist2D(const FVector& A, const FVector& B) { return (A - B).Size2D(); }
	static double DistSquared2D(const FVector& A, const FVector& B) { return (A - B).SizeSquared2D(); }
	static FVector VectorPlaneProject(const FVector& V, const FVector& N) { return V - V.ProjectOnToNormal(N); }
	static FVector PointPlaneProject(const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal)
	{
		return Point - PlaneNormal * ((Point - PlaneBase) | PlaneNormal);
	}

	static const FVector ZeroVector;
	static const FVector OneVector;
	static const FVector UpVector;
	static const FVector DownVector;
	static const FVector ForwardVector;
	static const FVector BackwardVector;
	static const FVector RightVector;
	static const FVector LeftVector;
	static const FVector XAxisVector;
	static const FVector YAxisVector;
	static const FVector ZAxisVector;
};
inline FVector operator*(double S, const FVector& V) { return V * S; }
inline const FVector FVector::ZeroVector(0.0, 0.0, 0.0);
inline const FVector FVector::OneVector(1.0, 1.0, 1.0);
inline const FVector FVector::UpVector(0.0, 0.0, 1.0);
inline const FVector FVector::DownVector(0.0, 0.0, -1.0);
inline const FVector FVector::ForwardVector(1.0, 0.0, 0.0);
inline const FVector FVector::BackwardVector(-1.0, 0.0, 0.0);
inline const FVector FVector::RightVector(0.0, 1.0, 0.0);
inline const FVector FVector::LeftVector(0.0, -1.0, 0.0);
inline const FVector FVector::XAxisVector(1.0, 0.0, 0.0);
inline const FVector FVector::YAxisVector(0.0, 1.0, 0.0);
inline const FVector FVector::ZAxisVector(0.0, 0.0, 1.0);
inline uint32 GetTypeHash(const FVector& V) { return static_cast<uint32>(std::hash<double>()(V.X) ^ (std::hash<double>()(V.Y) << 1) ^ (std::hash<double>()(V.Z) << 2)); }

using FVector3d = FVector;

struct FVector3f
{
	float X = 0.f;
	float Y = 0.f;
	float Z = 0.f;
	FVector3f() = default;
	FVector3f(float InX, float InY, float InZ) : X(InX), Y(InY), Z(InZ) {}
	explicit FVector3f(const FVector& V) : X(static_cast<float>(V.X)), Y(static_cast<float>(V.Y)), Z(static_cast<float>(V.Z)) {}
};

struct FVector4
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
	double W = 0.0;
	FVector4() = default;
	FVector4(double InX, double InY, double InZ, double InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
};

struct FRotator
{
	double Pitch = 0.0;
	double Yaw = 0.0;
	double Roll = 0.0;

	FRotator() = default;
	FRotator(double InPitch, double InYaw, double InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
	explicit FRotator(double V) : Pitch(V), Yaw(V), Roll(V) {}
	explicit FRotator(const FQuat& Q);

	FRotator operator+(const FRotator& O) const { return FRotator(Pitch + O.Pitch, Yaw + O.Yaw, Roll + O.Roll); }
	FRotator operator-(const FRotator& O) const { return FRotator(Pitch - O.Pitch, Yaw - O.Yaw, Roll - O.Roll); }
	FRotator operator*(double S) const { return FRotator(Pitch * S, Yaw * S, Roll * S); }
	FRotator& operator+=(const FRotator& O) { Pitch += O.Pitch; Yaw += O.Yaw; Roll += O.Roll; return *this; }
	bool operator==(const FRotator& O) const { return Pitch == O.Pitch && Yaw == O.Yaw && Roll == O.Roll; }
	bool Equals(const FRotator& O, double Tol = UE_KINDA_SMALL_NUMBER) const
	{
		return std::fabs(FMath::UnwindDegrees(Pitch - O.Pitch)) <= Tol && std::fabs(FMath::UnwindDegrees(Yaw - O.Yaw)) <= Tol
			&& std::fabs(FMath::UnwindDegrees(Roll - O.Roll)) <= Tol;
	}
	bool IsNearlyZero(double Tol = UE_KINDA_SMALL_NUMBER) const { return std::fabs(Pitch) <= Tol && std::fabs(Yaw) <= Tol && std::fabs(Roll) <= Tol; }

	FVector Vector() const
	{
		const double CP = std::cos(Pitch * UE_DOUBLE_PI / 180.0);
		const double SP = std::sin(Pitch * UE_DOUBLE_PI / 180.0);
		const double CY = std::cos(Yaw * UE_DOUBLE_PI / 180.0);
		const double SY = std::sin(Yaw * UE_DOUBLE_PI / 180.0);
		return FVector(CP * CY, CP * SY, SP);
	}
	FQuat Quaternion() const;
	FVector RotateVector(const FVector& V) const;
	FVector UnrotateVector(const FVector& V) const;
	FRotator GetNormalized() const { return FRotator(FMath::UnwindDegrees(Pitch), FMath::UnwindDegrees(Yaw), FMath::UnwindDegrees(Roll)); }
	FRotator GetInverse() const;
	void Normalize() { *this = GetNormalized(); }
	FVector Euler() const { return FVector(Roll, Pitch, Yaw); }
	static FRotator MakeFromEuler(const FVector& E) { return FRotator(E.Y, E.Z, E.X); }
	static double ClampAxis(double A)
	{
		A = std::fmod(A, 360.0);
		return A < 0.0 ? A + 360.0 : A;
	}
	static double NormalizeAxis(double A) { return FMath::UnwindDegrees(A); }
	FString ToString() const { return FString::Printf("P=%.2f Y=%.2f R=%.2f", Pitch, Yaw, Roll); }

	static const FRotator ZeroRotator;
};
inline const FRotator FRotator::ZeroRotator(0.0, 0.0, 0.0);

struct FQuat
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
	double W = 1.0;

	FQuat() = default;
	FQuat(double InX, double InY, double InZ, double InW) : X(InX), Y(InY), Z(InZ), W(InW) {}
	explicit FQuat(const FRotator& R)
	{
		const double DegToRadHalf = UE_DOUBLE_PI / 360.0;
		const double SP = std::sin(R.Pitch * DegToRadHalf), CP = std::cos(R.Pitch * DegToRadHalf);
		const double SY = std::sin(R.Yaw * DegToRadHalf), CY = std::cos(R.Yaw * DegToRadHalf);
		const double SR = std::sin(R.Roll * DegToRadHalf), CR = std::cos(R.Roll * DegToRadHalf);
		X = CR * SP * SY - SR * CP * CY;
		Y = -CR * SP * CY - SR * CP * SY;
		Z = CR * CP * SY - SR * SP * CY;
		W = CR * CP * CY + SR * SP * SY;
	}
	FQuat(const FVector& Axis, double AngleRad)
	{
		const double S = std::sin(AngleRad * 0.5);
		X = Axis.X * S;
		Y = Axis.Y * S;
		Z = Axis.Z * S;
		W = std::cos(AngleRad * 0.5);
	}
	FQuat operator*(const FQuat& Q) const
	{
		return FQuat(W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y, W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X, W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
			W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z);
	}
	FVector RotateVector(const FVector& V) const
	{
		const FVector Q(X, Y, Z);
		const FVector T = (Q ^ V) * 2.0;
		return V + (T * W) + (Q ^ T);
	}
	FVector UnrotateVector(const FVector& V) const { return Inverse().RotateVector(V); }
	FQuat Inverse() const { return FQuat(-X, -Y, -Z, W); }
	FQuat GetNormalized() const
	{
		const double S = std::sqrt(X * X + Y * Y + Z * Z + W * W);
		return S > 0.0 ? FQuat(X / S, Y / S, Z / S, W / S) : FQuat();
	}
	void Normalize() { *this = GetNormalized(); }
	FRotator Rotator() const
	{
		const double SingularityTest = Z * X - W * Y;
		const double YawY = 2.0 * (W * Z + X * Y);
		const double YawX = (1.0 - 2.0 * (Y * Y + Z * Z));
		const double RadToDeg = 180.0 / UE_DOUBLE_PI;
		FRotator R;
		if (SingularityTest < -0.4999995)
		{
			R.Pitch = -90.0;
			R.Yaw = std::atan2(YawY, YawX) * RadToDeg;
			R.Roll = FMath::UnwindDegrees(-R.Yaw - (2.0 * std::atan2(X, W) * RadToDeg));
		}
		else if (SingularityTest > 0.4999995)
		{
			R.Pitch = 90.0;
			R.Yaw = std::atan2(YawY, YawX) * RadToDeg;
			R.Roll = FMath::UnwindDegrees(R.Yaw - (2.0 * std::atan2(X, W) * RadToDeg));
		}
		else
		{
			R.Pitch = std::asin(2.0 * SingularityTest) * RadToDeg;
			R.Yaw = std::atan2(YawY, YawX) * RadToDeg;
			R.Roll = std::atan2(-2.0 * (W * X + Y * Z), (1.0 - 2.0 * (X * X + Y * Y))) * RadToDeg;
		}
		return R;
	}
	FVector GetForwardVector() const { return RotateVector(FVector::ForwardVector); }
	FVector GetRightVector() const { return RotateVector(FVector::RightVector); }
	FVector GetUpVector() const { return RotateVector(FVector::UpVector); }
	FVector Vector() const { return GetForwardVector(); }
	static FQuat Slerp(const FQuat& A, const FQuat& B, double Alpha)
	{
		double Dot = A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
		FQuat BB = B;
		if (Dot < 0.0)
		{
			Dot = -Dot;
			BB = FQuat(-B.X, -B.Y, -B.Z, -B.W);
		}
		return FQuat(A.X + (BB.X - A.X) * Alpha, A.Y + (BB.Y - A.Y) * Alpha, A.Z + (BB.Z - A.Z) * Alpha, A.W + (BB.W - A.W) * Alpha).GetNormalized();
	}
	static FQuat FindBetweenNormals(const FVector& A, const FVector& B)
	{
		const double NormAB = 1.0;
		double W = NormAB + (A | B);
		FQuat Result;
		if (W >= 1e-6 * NormAB)
		{
			const FVector C = A ^ B;
			Result = FQuat(C.X, C.Y, C.Z, W);
		}
		else
		{
			W = 0.0;
			Result = std::fabs(A.X) > std::fabs(A.Y) ? FQuat(-A.Z, 0.0, A.X, W) : FQuat(0.0, -A.Z, A.Y, W);
		}
		return Result.GetNormalized();
	}
	static FQuat FindBetweenVectors(const FVector& A, const FVector& B) { return FindBetweenNormals(A.GetSafeNormal(), B.GetSafeNormal()); }
	static const FQuat Identity;
};
inline const FQuat FQuat::Identity(0.0, 0.0, 0.0, 1.0);

inline FRotator::FRotator(const FQuat& Q) { *this = Q.Rotator(); }
inline FQuat FRotator::Quaternion() const { return FQuat(*this); }
inline FVector FRotator::RotateVector(const FVector& V) const { return FQuat(*this).RotateVector(V); }
inline FVector FRotator::UnrotateVector(const FVector& V) const { return FQuat(*this).UnrotateVector(V); }
inline FRotator FRotator::GetInverse() const { return FQuat(*this).Inverse().Rotator(); }
inline FRotator FVector::Rotation() const
{
	const double RadToDeg = 180.0 / UE_DOUBLE_PI;
	return FRotator(std::atan2(Z, std::sqrt(X * X + Y * Y)) * RadToDeg, std::atan2(Y, X) * RadToDeg, 0.0);
}
inline FRotator FVector::ToOrientationRotator() const { return Rotation(); }
inline FQuat FVector::ToOrientationQuat() const { return FQuat(Rotation()); }
inline FVector2D FVector::UnitCartesianToSpherical() const { return FVector2D(std::acos(Z), std::atan2(Y, X)); }

inline FVector FMath::VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float Speed)
{
	if (Speed <= 0.f)
	{
		return Target;
	}
	const FVector Dist = Target - Current;
	if (Dist.SizeSquared() < UE_KINDA_SMALL_NUMBER)
	{
		return Target;
	}
	return Current + Dist * static_cast<double>(Clamp(DeltaTime * Speed, 0.f, 1.f));
}
inline FVector FMath::VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float Speed)
{
	const FVector Delta = Target - Current;
	const double Dist = Delta.Size();
	const double Step = static_cast<double>(Speed * DeltaTime);
	return Dist <= Step ? Target : Current + Delta / Dist * Step;
}
inline FRotator FMath::RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float Speed)
{
	if (Speed <= 0.f)
	{
		return Target;
	}
	const FRotator Delta = (Target - Current).GetNormalized();
	return Current + Delta * static_cast<double>(Clamp(DeltaTime * Speed, 0.f, 1.f));
}
inline FRotator FMath::RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float Speed)
{
	const FRotator Delta = (Target - Current).GetNormalized();
	const double Step = static_cast<double>(Speed * DeltaTime);
	auto StepAxis = [Step](double D) { return std::fabs(D) <= Step ? D : (D > 0.0 ? Step : -Step); };
	return Current + FRotator(StepAxis(Delta.Pitch), StepAxis(Delta.Yaw), StepAxis(Delta.Roll));
}
inline FVector FMath::VRand()
{
	FVector V;
	do
	{
		V = FVector(FRandRange(-1.f, 1.f), FRandRange(-1.f, 1.f), FRandRange(-1.f, 1.f));
	} while (V.SizeSquared() > 1.0 || V.SizeSquared() < 1e-4);
	return V.GetSafeNormal();
}
inline FVector FMath::VRandCone(const FVector& Dir, float) { return (Dir + VRand() * 0.2).GetSafeNormal(); }
inline FVector FRandomStream::VRand() const
{
	FVector V;
	do
	{
		V = FVector(FRandRange(-1.f, 1.f), FRandRange(-1.f, 1.f), FRandRange(-1.f, 1.f));
	} while (V.SizeSquared() > 1.0 || V.SizeSquared() < 1e-4);
	return V.GetSafeNormal();
}
inline FVector FRandomStream::GetUnitVector() const { return VRand(); }

struct FTransform
{
	FQuat Rotation;
	FVector Translation;
	FVector Scale3D = FVector(1.0);

	FTransform() = default;
	explicit FTransform(const FVector& InTranslation) : Translation(InTranslation) {}
	explicit FTransform(const FRotator& R) : Rotation(R) {}
	FTransform(const FRotator& R, const FVector& T, const FVector& S = FVector(1.0)) : Rotation(R), Translation(T), Scale3D(S) {}
	FTransform(const FQuat& R, const FVector& T, const FVector& S = FVector(1.0)) : Rotation(R), Translation(T), Scale3D(S) {}

	FVector GetLocation() const { return Translation; }
	FVector GetTranslation() const { return Translation; }
	FQuat GetRotation() const { return Rotation; }
	FRotator Rotator() const { return Rotation.Rotator(); }
	FVector GetScale3D() const { return Scale3D; }
	void SetLocation(const FVector& V) { Translation = V; }
	void SetTranslation(const FVector& V) { Translation = V; }
	void SetRotation(const FQuat& Q) { Rotation = Q; }
	void SetScale3D(const FVector& S) { Scale3D = S; }
	FVector TransformPosition(const FVector& V) const { return Rotation.RotateVector(V * Scale3D) + Translation; }
	FVector TransformPositionNoScale(const FVector& V) const { return Rotation.RotateVector(V) + Translation; }
	FVector InverseTransformPosition(const FVector& V) const
	{
		const FVector Local = Rotation.UnrotateVector(V - Translation);
		return FVector(Local.X / Scale3D.X, Local.Y / Scale3D.Y, Local.Z / Scale3D.Z);
	}
	FVector InverseTransformPositionNoScale(const FVector& V) const { return Rotation.UnrotateVector(V - Translation); }
	FVector TransformVector(const FVector& V) const { return Rotation.RotateVector(V * Scale3D); }
	FVector TransformVectorNoScale(const FVector& V) const { return Rotation.RotateVector(V); }
	FVector InverseTransformVector(const FVector& V) const { return Rotation.UnrotateVector(V); }
	FVector InverseTransformVectorNoScale(const FVector& V) const { return Rotation.UnrotateVector(V); }
	FQuat TransformRotation(const FQuat& Q) const { return Rotation * Q; }
	FQuat InverseTransformRotation(const FQuat& Q) const { return Rotation.Inverse() * Q; }
	FVector GetUnitAxis(int32 Axis) const
	{
		return Axis == 0 ? Rotation.GetForwardVector() : (Axis == 1 ? Rotation.GetRightVector() : Rotation.GetUpVector());
	}
	FTransform operator*(const FTransform& Parent) const
	{
		FTransform Out;
		Out.Rotation = Parent.Rotation * Rotation;
		Out.Scale3D = Scale3D * Parent.Scale3D;
		Out.Translation = Parent.TransformPosition(Translation);
		return Out;
	}
	FTransform GetRelativeTransform(const FTransform& Other) const
	{
		FTransform Out;
		Out.Rotation = Other.Rotation.Inverse() * Rotation;
		Out.Translation = Other.InverseTransformPosition(Translation);
		return Out;
	}
	static const FTransform Identity;
};
inline const FTransform FTransform::Identity;

struct FBox
{
	FVector Min;
	FVector Max;
	uint8 IsValid = 0;
	FBox() = default;
	FBox(const FVector& InMin, const FVector& InMax) : Min(InMin), Max(InMax), IsValid(1) {}
	explicit FBox(int) {}
	FVector GetCenter() const { return (Min + Max) * 0.5; }
	FVector GetExtent() const { return (Max - Min) * 0.5; }
	FVector GetSize() const { return Max - Min; }
	bool IsInside(const FVector& P) const { return P.X > Min.X && P.X < Max.X && P.Y > Min.Y && P.Y < Max.Y && P.Z > Min.Z && P.Z < Max.Z; }
	bool IsInsideOrOn(const FVector& P) const { return P.X >= Min.X && P.X <= Max.X && P.Y >= Min.Y && P.Y <= Max.Y && P.Z >= Min.Z && P.Z <= Max.Z; }
	FBox ExpandBy(double W) const { return FBox(Min - FVector(W), Max + FVector(W)); }
	FVector GetClosestPointTo(const FVector& P) const
	{
		return FVector(std::clamp(P.X, Min.X, Max.X), std::clamp(P.Y, Min.Y, Max.Y), std::clamp(P.Z, Min.Z, Max.Z));
	}
	static FBox BuildAABB(const FVector& Origin, const FVector& Extent) { return FBox(Origin - Extent, Origin + Extent); }
};

struct FSphere
{
	FVector Center;
	double W = 0.0;
	FSphere() = default;
	FSphere(const FVector& C, double R) : Center(C), W(R) {}
};

struct FPlane
{
	double X = 0.0;
	double Y = 0.0;
	double Z = 0.0;
	double W = 0.0;
};

struct FColor
{
	uint8 R = 0;
	uint8 G = 0;
	uint8 B = 0;
	uint8 A = 255;
	FColor() = default;
	FColor(uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255) : R(InR), G(InG), B(InB), A(InA) {}
	static const FColor White;
	static const FColor Black;
	static const FColor Red;
	static const FColor Green;
	static const FColor Blue;
	static const FColor Yellow;
	static const FColor Cyan;
	static const FColor Orange;
	static const FColor Silver;
	static const FColor Emerald;
	static const FColor Turquoise;
};
inline const FColor FColor::White(255, 255, 255);
inline const FColor FColor::Black(0, 0, 0);
inline const FColor FColor::Red(255, 0, 0);
inline const FColor FColor::Green(0, 255, 0);
inline const FColor FColor::Blue(0, 0, 255);
inline const FColor FColor::Yellow(255, 255, 0);
inline const FColor FColor::Cyan(0, 255, 255);
inline const FColor FColor::Orange(243, 156, 18);
inline const FColor FColor::Silver(189, 195, 199);
inline const FColor FColor::Emerald(46, 204, 113);
inline const FColor FColor::Turquoise(26, 188, 156);

struct FLinearColor
{
	float R = 0.f;
	float G = 0.f;
	float B = 0.f;
	float A = 1.f;
	FLinearColor() = default;
	FLinearColor(float InR, float InG, float InB, float InA = 1.f) : R(InR), G(InG), B(InB), A(InA) {}
	FLinearColor(const FColor& C) : R(C.R / 255.f), G(C.G / 255.f), B(C.B / 255.f), A(C.A / 255.f) {}
	explicit FLinearColor(const FVector& V) : R(static_cast<float>(V.X)), G(static_cast<float>(V.Y)), B(static_cast<float>(V.Z)), A(1.f) {}
	FLinearColor operator+(const FLinearColor& O) const { return FLinearColor(R + O.R, G + O.G, B + O.B, A + O.A); }
	FLinearColor operator-(const FLinearColor& O) const { return FLinearColor(R - O.R, G - O.G, B - O.B, A - O.A); }
	FLinearColor operator*(float S) const { return FLinearColor(R * S, G * S, B * S, A * S); }
	FLinearColor operator*(const FLinearColor& O) const { return FLinearColor(R * O.R, G * O.G, B * O.B, A * O.A); }
	bool operator==(const FLinearColor& O) const { return R == O.R && G == O.G && B == O.B && A == O.A; }
	FColor ToFColor(bool) const
	{
		auto Q = [](float V) { return static_cast<uint8>(std::clamp(V, 0.f, 1.f) * 255.f + 0.5f); };
		return FColor(Q(R), Q(G), Q(B), Q(A));
	}
	FColor ToFColorSRGB() const { return ToFColor(true); }
	FLinearColor CopyWithNewOpacity(float NewA) const { return FLinearColor(R, G, B, NewA); }
	FLinearColor Desaturate(float) const { return *this; }
	FLinearColor LinearRGBToHSV() const { return *this; }
	FLinearColor HSVToLinearRGB() const { return *this; }
	static FLinearColor MakeFromHSV8(uint8 H, uint8 S, uint8 V) { return FLinearColor(H / 255.f, S / 255.f, V / 255.f); }
	static FLinearColor LerpUsingHSV(const FLinearColor& A, const FLinearColor& B, float T) { return A + (B - A) * T; }
	static const FLinearColor White;
	static const FLinearColor Black;
	static const FLinearColor Red;
	static const FLinearColor Green;
	static const FLinearColor Blue;
	static const FLinearColor Yellow;
	static const FLinearColor Gray;
	static const FLinearColor Transparent;
};
inline const FLinearColor FLinearColor::White(1.f, 1.f, 1.f);
inline const FLinearColor FLinearColor::Black(0.f, 0.f, 0.f);
inline const FLinearColor FLinearColor::Red(1.f, 0.f, 0.f);
inline const FLinearColor FLinearColor::Green(0.f, 1.f, 0.f);
inline const FLinearColor FLinearColor::Blue(0.f, 0.f, 1.f);
inline const FLinearColor FLinearColor::Yellow(1.f, 1.f, 0.f);
inline const FLinearColor FLinearColor::Gray(0.5f, 0.5f, 0.5f);
inline const FLinearColor FLinearColor::Transparent(0.f, 0.f, 0.f, 0.f);

struct FGuid
{
	uint32 A = 0, B = 0, C = 0, D = 0;
	static FGuid NewGuid() { FGuid G; G.A = static_cast<uint32>(std::rand()); return G; }
	FString ToString() const { return FString::Printf("%08X", A); }
	bool IsValid() const { return (A | B | C | D) != 0; }
};

struct FDateTime
{
	static FDateTime Now() { return FDateTime(); }
	static FDateTime UtcNow() { return FDateTime(); }
	FString ToString() const { return FString("2026.01.01-00.00.00"); }
	int64 GetTicks() const { return 0; }
};

struct FPlatformTime
{
	static double Seconds() { return 0.0; }
};

// ---------------------------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------------------------
struct FLogCategoryStub
{
	const char* Name;
};
#define DECLARE_LOG_CATEGORY_EXTERN(Name, Verb, Compile) extern FLogCategoryStub Name
#define DEFINE_LOG_CATEGORY(Name) FLogCategoryStub Name{#Name}
#define DEFINE_LOG_CATEGORY_STATIC(Name, Verb, Compile) static FLogCategoryStub Name{#Name}
template <typename... TArgs>
inline void UEStub_Log(const FLogCategoryStub&, const TCHAR*, TArgs&&...) {}
#define UE_LOG(Category, Verbosity, Format, ...) UEStub_Log(Category, Format, ##__VA_ARGS__)
#define UE_CLOG(Cond, Category, Verbosity, Format, ...) ((void)(Cond), UEStub_Log(Category, Format, ##__VA_ARGS__))

#define IMPLEMENT_PRIMARY_GAME_MODULE(Impl, Name, GameName)
#define IMPLEMENT_MODULE(Impl, Name)
class FDefaultGameModuleImpl
{
};
