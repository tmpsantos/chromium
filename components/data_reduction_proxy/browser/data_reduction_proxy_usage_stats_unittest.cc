// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/browser/data_reduction_proxy_usage_stats.h"

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/request_priority.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::MessageLoop;
using base::MessageLoopProxy;
using data_reduction_proxy::DataReductionProxyParams;
using net::TestDelegate;
using net::TestURLRequestContext;
using net::URLRequest;
using net::URLRequestStatus;
using testing::Return;

namespace {

class DataReductionProxyParamsMock : public DataReductionProxyParams {
 public:
  DataReductionProxyParamsMock() : DataReductionProxyParams(0) {}
  virtual ~DataReductionProxyParamsMock() {}

  MOCK_METHOD1(IsDataReductionProxyEligible, bool(const net::URLRequest*));
  MOCK_CONST_METHOD2(
      WasDataReductionProxyUsed,
      bool(const net::URLRequest*,
           data_reduction_proxy::DataReductionProxyTypeInfo* proxy_servers));

 private:
  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyParamsMock);
};

}  // namespace

namespace data_reduction_proxy {

class DataReductionProxyUsageStatsTest : public testing::Test {
 public:
  DataReductionProxyUsageStatsTest()
      : loop_proxy_(MessageLoopProxy::current().get()),
        context_(true),
        unavailable_(false) {
    context_.Init();
    mock_url_request_ = context_.CreateRequest(GURL(), net::IDLE, &delegate_,
                                               NULL);
  }

  void NotifyUnavailable(bool unavailable) {
    unavailable_ = unavailable;
  }

  // Required for MessageLoopProxy::current().
  base::MessageLoopForUI loop_;
  MessageLoopProxy* loop_proxy_;

 protected:
  TestURLRequestContext context_;
  TestDelegate delegate_;
  DataReductionProxyParamsMock mock_params_;
  scoped_ptr<URLRequest> mock_url_request_;
  bool unavailable_;
};

TEST_F(DataReductionProxyUsageStatsTest, IsDataReductionProxyUnreachable) {
  struct TestCase {
    bool is_proxy_eligible;
    bool was_proxy_used;
    bool is_unreachable;
  };
  const TestCase test_cases[] = {
    {
      false,
      false,
      false
    },
    {
      true,
      true,
      false
    },
    {
      true,
      false,
      true
    }
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(test_cases); ++i) {
    TestCase test_case = test_cases[i];

    EXPECT_CALL(mock_params_,
                IsDataReductionProxyEligible(mock_url_request_.get()))
                    .WillRepeatedly(Return(test_case.is_proxy_eligible));
    EXPECT_CALL(mock_params_,
                WasDataReductionProxyUsed(mock_url_request_.get(), NULL))
        .WillRepeatedly(Return(test_case.was_proxy_used));

    scoped_ptr<DataReductionProxyUsageStats> usage_stats(
        new DataReductionProxyUsageStats(
            &mock_params_, loop_proxy_));
    usage_stats->set_unavailable_callback(
        base::Bind(&DataReductionProxyUsageStatsTest::NotifyUnavailable,
                   base::Unretained(this)));

    usage_stats->OnUrlRequestCompleted(mock_url_request_.get(), false);
    MessageLoop::current()->RunUntilIdle();

    EXPECT_EQ(test_case.is_unreachable, unavailable_);
  }
}

}  // namespace data_reduction_proxy
