#pragma once
// The runner overrides this prefix with a fresh, disposable test directory.
// No test is routed into an Android application's save directory.
#ifndef QSAOW3_TEST_TMP
#ifdef __ANDROID__
#define QSAOW3_TEST_TMP "/data/local/tmp/"
#else
#define QSAOW3_TEST_TMP "/tmp/"
#endif
#endif
