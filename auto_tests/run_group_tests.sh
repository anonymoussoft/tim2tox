#!/bin/bash
# 运行所有群组相关测试用例

cd "$(dirname "$0")"

echo "Running group vs conference tests..."
flutter test test/scenarios/scenario_group_vs_conference_test.dart --no-pub --timeout=120s

echo ""
echo "Running multi-group tests..."
flutter test test/scenarios/scenario_group_multi_test.dart --no-pub --timeout=120s

echo ""
echo "Running group info modify tests..."
flutter test test/scenarios/scenario_group_info_modify_test.dart --no-pub --timeout=120s

echo ""
echo "Running group member info tests..."
flutter test test/scenarios/scenario_group_member_info_test.dart --no-pub --timeout=120s

echo ""
echo "All group tests completed!"
