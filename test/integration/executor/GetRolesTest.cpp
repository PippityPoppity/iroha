
// WORK ON REMOVING TARGET ACCOUNT CAUSE NOT NEEDED// also output should only include roles and not account information.

#include "integration/executor/executor_fixture.hpp"

#include <gtest/gtest.h>
#include <boost/format.hpp>
#include "backend/protobuf/queries/proto_query.hpp"
#include "framework/common_constants.hpp"
#include "integration/executor/query_permission_test.hpp"
#include "module/shared_model/mock_objects_factories/mock_command_factory.hpp"
#include "module/shared_model/mock_objects_factories/mock_query_factory.hpp"

using namespace common_constants;
using namespace executor_testing;
using namespace framework::expected;
using namespace shared_model::interface::types;

using iroha::ametsuchi::QueryExecutorResult;
using shared_model::interface::Amount;
using shared_model::interface::permissions::Role;


struct GetRolesTest : public ExecutorTestBase {

  void prepareTargetAccount() {
    SCOPED_TRACE("GetAccountTest::prepareTargetAccount");
    const auto &detail = *details_.at(kAdminId).begin();
    IROHA_ASSERT_RESULT_VALUE(getItf().executeMaintenanceCommand(
        *getItf().getMockCommandFactory()->constructSetAccountDetail(
            kUserId, detail.first, detail.second)));
  }


  QueryExecutorResult query(const RoleIdType &query_issuer = kAdminId) {
    return getItf().executeQuery(
        *getItf().getMockQueryFactory()->constructGetRole(kUserId),
        query_issuer);
  }

  void validateResponse(const AccountResponse &response) {
    EXPECT_EQ(response.account().accountId(), kUserId);
    EXPECT_EQ(response.account().domainId(), kDomain);
    EXPECT_EQ(response.account().quorum(), kQuorum);
  }

 protected:
  const DetailsByKeyByWriter details_{{{kAdminId, permissions}}}};
};

using GetRolesBasicTest = BasicExecutorTest<GetRolesTest>;

/**
 * @given a user with all related permissions
 * @when GetRoles is queried on non existent user
 * @then there is an NoRolesErrorResponse
 */
TEST_P(GetRolesBasicTest, NonexistentAccount) {
  checkQueryError<shared_model::interface::NoRolesErrorResponse>(
      getItf().executeQuery(
          *getItf().getMockQueryFactory()->constructGetRoles(kUserId)),
      0);
}

INSTANTIATE_TEST_SUITE_P(Base,
                         GetRolesBasicTest,
                         executor_testing::getExecutorTestParams(),
                         executor_testing::paramToString);

using GetRolesPermissionTest =
    query_permission_test::QueryPermissionTest<GetRolesTest>;

TEST_P(GetRolesPermissionTest, QueryPermissionTest) {
  ASSERT_NO_FATAL_FAILURE(prepareState({}));
  ASSERT_NO_FATAL_FAILURE(prepareTargetAccount());
  checkResponse<shared_model::interface::AccountResponse>(
      query(getSpectator()),
      [this](const shared_model::interface::AccountResponse &response) {
        this->validateResponse(response);
      });
}

INSTANTIATE_TEST_SUITE_P(
    Common,
    GetRolesPermissionTest,
    query_permission_test::getParams({Role::kGetMyRoles},
                                     {Role::kGetPermissions},
                                     {Role::kGetAllRoles}),
    query_permission_test::paramToString);
