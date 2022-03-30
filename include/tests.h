#pragma once
#include <stdint.h>
#include <stdarg.h>

void fstest_run_all();

void fstest_dirs_create();
void fstest_files_create();

void fstest_dirs_delete();
void fstest_files_delete();

void fstest_files_rename();

void fstest_dirs_rename();