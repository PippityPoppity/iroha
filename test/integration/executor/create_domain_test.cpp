#include "integration/executor/executor_fixture.hpp"

#include <gtest/gtest.h>
#include "common/result.hpp"
#include "framework/common_constants.hpp"
#include "framework/crypto_literals.hpp"
#include "integration/executor/command_permission_test.hpp"
#include "integration/executor/executor_fixture_param_provider.hpp"
#include "module/shared_model/mock_objects_factories/mock_command_factory.hpp"
#include "module/shared_model/mock_objects_factories/mock_query_factory.hpp"

using namespace common_constants;
using namespace executor_testing;
using namespace framework::expected;
using namespace shared_model::interface::types;

using shared_model::interface::permissions::Grantable;
using shared_model::interface::permissions::Role;

static const DomainIdType(kNewDomain = "newdomain");

class CreateDomainTest : public ExecutorTestBase {
 public:
  iroha::ametsuchi::CommandResult createDomain(
  const AccountIdType &issuer,
  const DomainIdType &target_domain = kNewDomain,
  const RoleIdType &default_role= kRole,
  bool validation_enabled = true){
  return getItf().executeCommandAsAccount(
        *getItf().getMockCommandFactory()->constructCreateDomain(
            target_domain, default_role),
        issuer,
        validation_enabled);
 
  void checkDomain(
      const DomainIdType &domain, 
      const RoleIdType &role,
      const shared_model::interface::RolePermissionSet &ref_permissions) {
    getRolePerms(role).specific_response.match(
        [&](const auto &test_permissions) {
          EXPECT_EQ(test_permissions.value.rolePermissions(), ref_permissions)
              << "Wrong set of permissions for domain " << role;
        },
        [](const auto &e) { ADD_FAILURE() << e.error->toString(); });
  } 

  void checkNoSuchDomain(const DomainIdType &domain){
    checkQueryError<shared_model::interface::NoDomainnErrorResponse>(
        getItf().executeQuery(
            *getItf().getMockQueryFactory()->constructGetDomain(
                &domain)),
        0);
  }

}

using CreateDomainBasicTest = BasicExecutorTest<CreateDomainTest>;

/**
 * given a user with all related permissions
 * when executes CreateDomain command with nonexistent role
 * then the command does not succeed and the domain is not added
 */

TEST_P(CreateAccountBasicTest, NoRole) {
  checkCommandError(createAccount(kAdminId, kNewDomain, "no_such_role"), 3);
  checkNoSuchDomain("no_such_domain");
 }

/**
 * given a user with all related permissions
 * when executes CreateDomain command with occupied domain name and another 
 * role
 * then the command does not succeed and the original account is not changed
 */

TEST_P(CreateDomainBasicTest, NameExists) {
  ASSERT_NO_FATAL_FAILURE(
      getItf().createUserWithPerms(kNewDomain, kRole, {}));
  ASSERT_NO_FATAL_FAILURE(checkDomain());
}

INSTANTIATE_TEST_SUITE_P(Base,
                         CreateDomainBasicTest,
                         executor_testing::getExecutorTestParams(),
                         executor_testing::paramToString);

using CreateDomainPermissionTest =
    command_permission_test::CommandPermissionTest<CreateDomainTest>;

TEST_P(CreateRolePermissionTest, CommandPermissionTest) {
  ASSERT_NO_FATAL_FAILURE(getItf().createDomain(kSecondDomain));
  ASSERT_NO_FATAL_FAILURE(prepareState({}, {Role::kCreateAsset}));

  if (checkResponse(createDomain(
          getActor(), (kSecondDomain, {Role::kCreateAsset}, getValidationEnabled())))) {
    checkDomain(kAnotherDomain, {Role::kCreateAsset});
  } else {
    checkNoSuchDomain(kAnotherDomain);
  }
}

INSTANTIATE_TEST_SUITE_P(Common,
                         CreateDomainPermissionTest,
                         command_permission_test::getParams(boost::none,
                                                            boost::none,
                                                            Domain::kCreateDomain,
                                                            boost::none),
                         command_permission_test::paramToString);





