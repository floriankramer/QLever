//
// Created by buchholb on 12.10.17.
//
#pragma once

class PermutationSettings {
public:

  enum {
    DO_NOT_USE,
    FROM_DISK,
    PRELOAD
  };
  PermutationSetting;

  PermutationSetting _spoSetting;
  PermutationSetting _sopSetting;
  PermutationSetting _ospSetting;
  PermutationSetting _opsSetting;

  PermutationSettings() : _spoSetting(DO_NOT_USE), _sopSetting(DO_NOT_USE),
                          _ospSetting(DO_NOT_USE), _opsSetting(DO_NOT_USE) {

  }
};