// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Florian Kramer (florian.kramer@netpun.uni-freiburg.de)

#include <array>
#include <vector>

#include <gtest/gtest.h>
#include "../src/engine/MultiColumnJoin.h"

TEST(EngineTest, multiColumnJoinTest) {
  using std::array;
  using std::vector;

  IdTable a(3);
  a.push_back({4, 1, 2});
  a.push_back({2, 1, 3});
  a.push_back({1, 1, 4});
  a.push_back({2, 2, 1});
  a.push_back({1, 3, 1});
  IdTable b(3);
  b.push_back({3, 3, 1});
  b.push_back({1, 8, 1});
  b.push_back({4, 2, 2});
  b.push_back({1, 1, 3});
  IdTable res(4);
  vector<array<Id, 2>> jcls;
  jcls.push_back(array<Id, 2>{{1, 2}});
  jcls.push_back(array<Id, 2>{{2, 1}});

  // Join a and b on the column pairs 1,2 and 2,1 (entries from columns 1 of
  // a have to equal those of column 2 of b and vice versa).
  MultiColumnJoin::computeMultiColumnJoin(a, b, jcls, &res);

  ASSERT_EQ(2u, res.size());

  ASSERT_EQ(2u, res[0][0]);
  ASSERT_EQ(1u, res[0][1]);
  ASSERT_EQ(3u, res[0][2]);
  ASSERT_EQ(3u, res[0][3]);

  ASSERT_EQ(1u, res[1][0]);
  ASSERT_EQ(3u, res[1][1]);
  ASSERT_EQ(1u, res[1][2]);
  ASSERT_EQ(1u, res[1][3]);

  // Test the multi column join with variable sized data.
  IdTable va(6);
  va.push_back({1, 2, 3, 4, 5, 6});
  va.push_back({1, 2, 3, 7, 5, 6});
  va.push_back({7, 6, 5, 4, 3, 2});

  IdTable vb(3);
  vb.push_back({2, 3, 4});
  vb.push_back({2, 3, 5});
  vb.push_back({6, 7, 4});

  IdTable vres(7);
  jcls.clear();
  jcls.push_back({1, 0});
  jcls.push_back({2, 1});

  // The template size parameter can be at most 6 (the maximum number
  // of fixed size columns plus one).
  MultiColumnJoin::computeMultiColumnJoin(va, vb, jcls, &vres);

  ASSERT_EQ(4u, vres.size());
  ASSERT_EQ(7u, vres.cols());

  IdTable wantedRes(7);
  wantedRes.push_back({1, 2, 3, 4, 5, 6, 4});
  wantedRes.push_back({1, 2, 3, 4, 5, 6, 5});
  wantedRes.push_back({1, 2, 3, 7, 5, 6, 4});
  wantedRes.push_back({1, 2, 3, 7, 5, 6, 5});

  ASSERT_EQ(wantedRes[0], vres[0]);
  ASSERT_EQ(wantedRes[1], vres[1]);
  ASSERT_EQ(wantedRes[2], vres[2]);
  ASSERT_EQ(wantedRes[3], vres[3]);
}
