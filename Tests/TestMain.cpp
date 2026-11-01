#include <juce_core/juce_core.h>

int main()
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("Bonsai");

    int failures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* result = runner.getResult (i))
        {
            failures += result->failures;
            std::cout << (result->failures == 0 ? "  PASS  " : "  FAIL  ")
                      << result->unitTestName << " / " << result->subcategoryName
                      << "  (" << result->passes << " checks";

            if (result->failures > 0)
                std::cout << ", " << result->failures << " failed: "
                          << result->messages.joinIntoString ("; ");

            std::cout << ")\n";
        }
    }

    std::cout << (failures == 0 ? "\nAll Bonsai tests passed.\n"
                                : "\n" + std::to_string (failures) + " assertion(s) failed.\n");

    return failures == 0 ? 0 : 1;
}
