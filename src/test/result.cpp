#include <cstddef>
#include <cstring>

#include <common/result.hpp>
#include <test/include/acutest.h>

using namespace pgb::common;

void test_result_basic(void);
void test_result_custom(void);
void test_resultset(void);
void test_resultset_casting(void);

TEST_LIST = {
    {"Provided Results", test_result_basic},
    {"Custom Results", test_result_custom},
    {"ResultSet", test_resultset},
    {"ResultSet casting", test_resultset_casting},
    {NULL, NULL}
};

void test_result_basic(void)
{
    TEST_CHECK(std::strcmp(ResultSuccess::GetDescription(), "Success") == 0);
    TEST_CHECK(std::strcmp(ResultFailure::GetDescription(), "Failure") == 0);
}

void test_result_custom(void)
{
    using ResultTest0 = Result<"Test0">;
    using ResultTest1 = Result<"Test1 has multiple words">;
    using ResultTest2 = Result<"Test2">;
    TEST_CHECK(std::strcmp(ResultTest0::GetDescription(), "Test0") == 0);
    TEST_CHECK(std::strcmp(ResultTest1::GetDescription(), "Test1 has multiple words") == 0);
    TEST_CHECK(std::strcmp(ResultTest2::GetDescription(), "Test2") == 0);
}

void test_resultset(void)
{
    {
        using ResultSetTest = ResultSet<int, ResultSuccess>;
        auto result         = ResultSetTest(ResultSuccess(true), -1);
        TEST_CHECK(strcmp(result.GetStatusDescription(), "Success") == 0);
        TEST_CHECK(result.IsSuccess());
    }

    {
        using ResultSetTest = ResultSet<void, ResultSuccess, ResultFailure>;
        auto resultF        = ResultSetTest(ResultFailure(false));
        TEST_CHECK(strcmp(resultF.GetStatusDescription(), "Failure") == 0);
        TEST_CHECK(resultF.IsFailure());
        TEST_CHECK(!resultF.IsResult<ResultSuccess>());
        TEST_CHECK(resultF.IsResult<ResultFailure>());

        auto resultS = ResultSetTest(ResultSuccess(true));
        TEST_CHECK(strcmp(resultS.GetStatusDescription(), "Success") == 0);
        TEST_CHECK(resultS.IsResult<ResultSuccess>());
        TEST_CHECK(!resultS.IsResult<ResultFailure>());
    }
}

void test_resultset_casting(void)
{
    // Basic integer type
    {
        using ResultSetTest = ResultSet<int, ResultSuccess>;
        auto result         = ResultSetTest(ResultSuccess(true), 1);
        // Implicit casting
        TEST_CHECK(result == 1);
        // Explicit casting
        TEST_CHECK(static_cast<int>(result) == 1);
    }

    // Reference type
    {
        using ResultSetTest = ResultSet<int&, ResultSuccess>;
        int  a              = 0xFF;
        auto result         = ResultSetTest(ResultSuccess(true), a);

        TEST_CHECK(a == 0xFF);
        TEST_CHECK(result == 0xFF);
        a = 0xAF;
        TEST_CHECK(result == 0xAF);
        TEST_CHECK(result == a);

        const_cast<int&>(static_cast<const int&>(result)) = 0x23;
        TEST_CHECK(a == 0x23);
        TEST_CHECK(result == a);
    }

    // To different result type
    {
        using ResultSetTestInt   = ResultSet<int, ResultSuccess>;
        using ResultSetTestFloat = ResultSet<float, ResultSuccess, ResultFailure>;

        int  a                   = 255;
        auto resultInt           = ResultSetTestInt::DefaultResultSuccess(a);
        TEST_CHECK(static_cast<int>(resultInt) == 0xFF);

        auto resultFloat = static_cast<ResultSetTestFloat>(resultInt);
        TEST_CHECK(static_cast<float>(resultFloat) == 255.0f);
    }
}