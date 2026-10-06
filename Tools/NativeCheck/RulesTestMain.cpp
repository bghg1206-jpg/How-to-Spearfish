// Native runner for Source/HowToSpearfish/Tests/SpearfishRulesTestCases.h (see run_rules_tests.sh).
#include "Tests/SpearfishRulesTestCases.h"

#include <cstdio>

class FNativeContext final : public FSpearfishRulesTestContext
{
public:
	int32 Failures = 0;
	int32 Checks = 0;
	const TCHAR* CurrentCase = TEXT("");

	virtual void Check(bool bCondition, const TCHAR* Expression, const char* File, int32 Line) override
	{
		++Checks;
		if (!bCondition)
		{
			++Failures;
			std::printf("  FAIL [%s] %s (%s:%d)\n", CurrentCase, Expression, File, Line);
		}
	}
};

int main()
{
	FNativeContext Context;
	int32 FailedCases = 0;
	const TArray<SpearfishRulesTests::FCase> Cases = SpearfishRulesTests::AllCases();
	for (const SpearfishRulesTests::FCase& Case : Cases)
	{
		const int32 Before = Context.Failures;
		Context.CurrentCase = Case.Name;
		Case.Run(Context);
		const bool bPassed = Context.Failures == Before;
		FailedCases += bPassed ? 0 : 1;
		std::printf("%s %s\n", bPassed ? "[PASS]" : "[FAIL]", Case.Name);
	}
	std::printf("\n%d cases, %d checks, %d failed checks, %d failed cases\n", Cases.Num(), Context.Checks, Context.Failures, FailedCases);
	return FailedCases == 0 ? 0 : 1;
}
