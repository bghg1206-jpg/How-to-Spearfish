#pragma once

// Remaining engine/plugin stand-ins: JSON, DataTable, paths and files, console commands, automation tests,
// Algo, modules, the procedural mesh component and FText/FString pieces the core stub keeps minimal.

#include "UEStub/UEStubEngine.h"

#include <algorithm>

// ----------------------------------------------------------------------------------- FString paths

FString operator/(const FString& Lhs, const FString& Rhs);
FString operator/(const FString& Lhs, const TCHAR* Rhs);

struct FPaths
{
	static FString ProjectDir();
	static FString ProjectContentDir();
	static FString ProjectSavedDir();
	static FString ProjectConfigDir();
	static FString EngineDir();
	static bool FileExists(const FString& InPath);
	static bool DirectoryExists(const FString& InPath);
	static FString ConvertRelativePathToFull(const FString& InPath);
	static FString GetBaseFilename(const FString& InPath, bool bRemovePath = true);
	static FString GetExtension(const FString& InPath, bool bIncludeDot = false);
	template <typename... PathTypes>
	static FString Combine(PathTypes&&... InPaths);
};

enum class EHashOptions
{
	None = 0,
	EnableVerify = 1 << 0,
	ErrorMissingHash = 1 << 1
};

struct FFileHelper
{
	enum class EEncodingOptions
	{
		AutoDetect,
		ForceAnsi,
		ForceUnicode,
		ForceUTF8,
		ForceUTF8WithoutBOM
	};
	static bool LoadFileToString(FString& Result, const TCHAR* Filename, EHashOptions VerifyFlags = EHashOptions::None, uint32 ReadFlags = 0);
	static bool LoadFileToStringArray(TArray<FString>& Result, const TCHAR* Filename);
	static bool SaveStringToFile(const FString& String, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect,
		class IFileManager* FileManager = nullptr, uint32 WriteFlags = 0);
};

// -------------------------------------------------------------------------------------------- JSON

class FJsonObject;

enum class EJson
{
	None,
	Null,
	String,
	Number,
	Boolean,
	Array,
	Object
};

class FJsonValue
{
public:
	virtual ~FJsonValue();
	EJson Type = EJson::None;
	virtual FString AsString() const;
	virtual double AsNumber() const;
	virtual bool AsBool() const;
	virtual const TArray<TSharedPtr<FJsonValue>>& AsArray() const;
	virtual const TSharedPtr<FJsonObject>& AsObject() const;
	virtual bool TryGetString(FString& OutString) const;
	virtual bool TryGetNumber(double& OutNumber) const;
	virtual bool TryGetNumber(int32& OutNumber) const;
	virtual bool TryGetBool(bool& OutBool) const;
	virtual bool TryGetArray(const TArray<TSharedPtr<FJsonValue>>*& OutArray) const;
	virtual bool TryGetObject(const TSharedPtr<FJsonObject>*& Object) const;
	bool IsNull() const;
};

class FJsonObject
{
public:
	TMap<FString, TSharedPtr<FJsonValue>> Values;
	bool HasField(const FString& FieldName) const;
	bool HasTypedField(EJson JsonType, const FString& FieldName) const;
	TSharedPtr<FJsonValue> TryGetField(const FString& FieldName) const;
	FString GetStringField(const FString& FieldName) const;
	bool TryGetStringField(const FString& FieldName, FString& OutString) const;
	double GetNumberField(const FString& FieldName) const;
	bool TryGetNumberField(const FString& FieldName, double& OutNumber) const;
	bool TryGetNumberField(const FString& FieldName, int32& OutNumber) const;
	bool GetBoolField(const FString& FieldName) const;
	bool TryGetBoolField(const FString& FieldName, bool& OutBool) const;
	const TArray<TSharedPtr<FJsonValue>>& GetArrayField(const FString& FieldName) const;
	bool TryGetArrayField(const FString& FieldName, const TArray<TSharedPtr<FJsonValue>>*& OutArray) const;
	const TSharedPtr<FJsonObject>& GetObjectField(const FString& FieldName) const;
	bool TryGetObjectField(const FString& FieldName, const TSharedPtr<FJsonObject>*& OutObject) const;
	void SetStringField(const FString& FieldName, const FString& StringValue);
	void SetNumberField(const FString& FieldName, double Number);
	void SetBoolField(const FString& FieldName, bool InValue);
	void SetField(const FString& FieldName, const TSharedPtr<FJsonValue>& Value);
	void RemoveField(const FString& FieldName);
};

template <class CharType = TCHAR>
class TJsonReader
{
public:
	virtual ~TJsonReader();
};

template <class CharType = TCHAR>
class TJsonReaderFactory
{
public:
	static TSharedRef<TJsonReader<CharType>> Create(const FString& JsonString);
};

template <class CharType = TCHAR>
class TJsonWriter
{
public:
	virtual ~TJsonWriter();
	bool Close();
};

template <class CharType = TCHAR>
class TJsonWriterFactory
{
public:
	static TSharedRef<TJsonWriter<CharType>> Create(FString* const Stream, int32 InitialIndentLevel = 0);
};

class FJsonSerializer
{
public:
	template <class CharType>
	static bool Deserialize(const TSharedRef<TJsonReader<CharType>>& Reader, TSharedPtr<FJsonObject>& OutObject);
	template <class CharType>
	static bool Deserialize(const TSharedRef<TJsonReader<CharType>>& Reader, TArray<TSharedPtr<FJsonValue>>& OutArray);
	template <class CharType>
	static bool Deserialize(const TSharedRef<TJsonReader<CharType>>& Reader, TSharedPtr<FJsonValue>& OutValue);
	template <class CharType>
	static bool Serialize(const TSharedRef<FJsonObject>& Object, const TSharedRef<TJsonWriter<CharType>>& Writer, bool bCloseWriter = true);
};

class FJsonObjectConverter
{
public:
	static bool JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, const UStruct* StructDefinition, void* OutStruct, int64 CheckFlags = 0,
		int64 SkipFlags = 0, const bool bStrictMode = false, FText* OutFailReason = nullptr);
	template <typename OutStructType>
	static bool JsonObjectToUStruct(const TSharedRef<FJsonObject>& JsonObject, OutStructType* OutStruct, int64 CheckFlags = 0, int64 SkipFlags = 0,
		const bool bStrictMode = false, FText* OutFailReason = nullptr)
	{
		// The real template calls OutStructType::StaticStruct(): only reflected USTRUCTs are accepted.
		return JsonObjectToUStruct(JsonObject, OutStructType::StaticStruct(), OutStruct, CheckFlags, SkipFlags, bStrictMode, OutFailReason);
	}
	template <typename InStructType>
	static TSharedPtr<FJsonObject> UStructToJsonObject(const InStructType& InStruct, int64 CheckFlags = 0, int64 SkipFlags = 0);
	template <typename InStructType>
	static bool UStructToJsonObjectString(const InStructType& InStruct, FString& OutJsonString, int64 CheckFlags = 0, int64 SkipFlags = 0, int32 Indent = 0);
};

// --------------------------------------------------------------------------------------- DataTable

struct FTableRowBase
{
	virtual ~FTableRowBase() {}
	virtual void OnPostDataImport(const class UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) {}
};

class UDataTable : public UObject
{
public:
	TObjectPtr<UScriptStruct> RowStruct;
	const UScriptStruct* GetRowStruct() const;
	TArray<FName> GetRowNames() const;
	template <class T>
	T* FindRow(FName RowName, const TCHAR* ContextString, bool bWarnIfRowMissing = true) const;
	template <class T>
	T* FindRow(FName RowName, const FString& ContextString, bool bWarnIfRowMissing = true) const;
	template <class T>
	void GetAllRows(const TCHAR* ContextString, TArray<T*>& OutRowArray) const;
	template <class T>
	void ForeachRow(const TCHAR* ContextString, TFunctionRef<void(const FName& Key, const T& Value)> Predicate) const;
};

// ----------------------------------------------------------------------------------------- Console

enum EConsoleVariableFlags
{
	ECVF_Default = 0x0,
	ECVF_Cheat = 0x1,
	ECVF_ReadOnly = 0x4,
	ECVF_Unregistered = 0x8
};

class IConsoleObject
{
public:
	virtual ~IConsoleObject();
};

class IConsoleCommand : public IConsoleObject
{
};

typedef TDelegate<void()> FConsoleCommandDelegate;
typedef TDelegate<void(const TArray<FString>&)> FConsoleCommandWithArgsDelegate;
typedef TDelegate<void(UWorld*)> FConsoleCommandWithWorldDelegate;
typedef TDelegate<void(const TArray<FString>&, UWorld*)> FConsoleCommandWithWorldAndArgsDelegate;

class IConsoleManager
{
public:
	static IConsoleManager& Get();
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleCommand* RegisterConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command,
		uint32 Flags = ECVF_Default) = 0;
	virtual void UnregisterConsoleObject(IConsoleObject* ConsoleObject, bool bKeepState = true) = 0;
	virtual class IConsoleVariable* FindConsoleVariable(const TCHAR* Name, bool bTrackFrequentCalls = true) const = 0;
};

class FAutoConsoleCommand
{
public:
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandDelegate& Command, uint32 Flags = ECVF_Default);
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithArgsDelegate& Command, uint32 Flags = ECVF_Default);
	FAutoConsoleCommand(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags = ECVF_Default);
};

class FAutoConsoleCommandWithWorld
{
public:
	FAutoConsoleCommandWithWorld(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldDelegate& Command, uint32 Flags = ECVF_Default);
};

class FAutoConsoleCommandWithWorldAndArgs
{
public:
	FAutoConsoleCommandWithWorldAndArgs(const TCHAR* Name, const TCHAR* Help, const FConsoleCommandWithWorldAndArgsDelegate& Command, uint32 Flags = ECVF_Default);
};

// -------------------------------------------------------------------------------------- Automation

enum class EAutomationTestFlags : uint32
{
	None = 0,
	EditorContext = 0x00000001,
	ClientContext = 0x00000002,
	ServerContext = 0x00000004,
	CommandletContext = 0x00000008,
	ProgramContext = 0x00000010,
	SmokeFilter = 0x01000000,
	EngineFilter = 0x02000000,
	ProductFilter = 0x04000000,
	PerfFilter = 0x08000000,
	StressFilter = 0x10000000
};
inline EAutomationTestFlags operator|(EAutomationTestFlags A, EAutomationTestFlags B)
{
	return static_cast<EAutomationTestFlags>(static_cast<uint32>(A) | static_cast<uint32>(B));
}

class FAutomationTestBase
{
public:
	FAutomationTestBase(const FString& InName, const bool bInComplexTask);
	virtual ~FAutomationTestBase();
	virtual void AddError(const FString& InError, int32 StackOffset = 0);
	virtual void AddWarning(const FString& InWarning, int32 StackOffset = 0);
	virtual void AddInfo(const FString& InLogItem, int32 StackOffset = 0, bool bCaptureStack = false);
	bool HasAnyErrors() const;
	bool TestTrue(const TCHAR* What, bool Value);
	bool TestFalse(const TCHAR* What, bool Value);
	template <typename T>
	bool TestEqual(const TCHAR* What, const T& Actual, const T& Expected);

protected:
	virtual bool RunTest(const FString& Parameters) = 0;
	virtual uint32 GetTestFlags() const = 0;
	virtual FString GetBeautifiedTestName() const = 0;
};

#define IMPLEMENT_SIMPLE_AUTOMATION_TEST(TClass, PrettyName, TFlags) \
	class TClass : public FAutomationTestBase \
	{ \
	public: \
		TClass(const FString& InName) : FAutomationTestBase(InName, false) { (void)static_cast<EAutomationTestFlags>(TFlags); } \
		virtual uint32 GetTestFlags() const override { return static_cast<uint32>(TFlags); } \
		virtual FString GetBeautifiedTestName() const override { return FString(PrettyName); } \
	protected: \
		virtual bool RunTest(const FString& Parameters) override; \
	};

// ------------------------------------------------------------------------------------------- Algo

namespace Algo
{
	template <typename RangeType>
	void Sort(RangeType& Range)
	{
		std::sort(Range.begin(), Range.end());
	}
	template <typename RangeType, typename PredicateType>
	void Sort(RangeType& Range, PredicateType Predicate)
	{
		std::sort(Range.begin(), Range.end(), Predicate);
	}
	template <typename RangeType, typename PredicateType>
	auto FindByPredicate(RangeType& Range, PredicateType Predicate) -> decltype(&*Range.begin());
	template <typename RangeType, typename ValueType>
	bool Contains(const RangeType& Range, const ValueType& Value);
}

// ---------------------------------------------------------------------------------------- Modules

class IModuleInterface
{
public:
	virtual ~IModuleInterface();
	virtual void StartupModule();
	virtual void ShutdownModule();
};

class FModuleManager
{
public:
	static FModuleManager& Get();
	bool IsModuleLoaded(const FName InModuleName) const;
	template <typename TModuleInterface>
	static TModuleInterface& LoadModuleChecked(const FName InModuleName);
};

// ---------------------------------------------------------------------------------- Procedural mesh

struct FProcMeshTangent
{
	FVector TangentX;
	bool bFlipTangentY = false;
	FProcMeshTangent();
	FProcMeshTangent(float X, float Y, float Z);
	FProcMeshTangent(FVector InTangentX, bool bInFlipTangentY);
};

class UProceduralMeshComponent : public UMeshComponent
{
public:
	bool bUseComplexAsSimpleCollision = true;
	bool bUseAsyncCooking = false;

	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);
	void CreateMeshSection_LinearColor(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision,
		bool bSRGBConversion = false);
	void UpdateMeshSection_LinearColor(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
		const TArray<FLinearColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bSRGBConversion = false);
	void ClearMeshSection(int32 SectionIndex);
	void ClearAllMeshSections();
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);
	int32 GetNumSections() const;
	void AddCollisionConvexMesh(TArray<FVector> ConvexVerts);
	void ClearCollisionConvexMeshes();
};
