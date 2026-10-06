#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/SpearfishRulesTestCases.h"

namespace
{
	class FSpearfishAutomationContext final : public FSpearfishRulesTestContext
	{
	public:
		explicit FSpearfishAutomationContext(FAutomationTestBase& InTest)
			: Test(InTest)
		{
		}

		virtual void Check(bool bCondition, const TCHAR* Expression, const char* File, int32 Line) override
		{
			if (!bCondition)
			{
				Test.AddError(FString::Printf(TEXT("%s [%s] (%s:%d)"), Expression, *CaseName, ANSI_TO_TCHAR(File), Line));
			}
		}

		FString CaseName;

	private:
		FAutomationTestBase& Test;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpearfishRulesAutomationTest, "Spearfish.Rules.All",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)

bool FSpearfishRulesAutomationTest::RunTest(const FString& Parameters)
{
	FSpearfishAutomationContext Context(*this);
	for (const SpearfishRulesTests::FCase& Case : SpearfishRulesTests::AllCases())
	{
		Context.CaseName = Case.Name;
		Case.Run(Context);
	}
	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
