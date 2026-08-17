/*
 * This file is part of romHEX14.
 * Copyright (C) 2026 Cristian Tabuyo <contact@romhex14.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>
#include <cassert>
#include <iostream>

#include "checksums/IChecksumPlugin.h"
#include "checksums/BoschMED17.h"

static int g_testsRun = 0;
static int g_testsPassed = 0;

#define TEST_ASSERT(cond, desc) do { \
    g_testsRun++; \
    if (cond) { \
        g_testsPassed++; \
        std::cout << "  [PASS] " << desc << std::endl; \
    } else { \
        std::cerr << "  [FAIL] " << desc << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    } \
} while(0)

// ── Test 1: ABI Version Definition ───────────────────────────────────────────
static void testAbiVersion() {
    std::cout << "\n=== Test Suite 1: Plugin ABI Versioning ===" << std::endl;
    TEST_ASSERT(RX14_CHECKSUM_PLUGIN_VERSION == 1, "RX14_CHECKSUM_PLUGIN_VERSION equals 1");
    TEST_ASSERT(std::string(RX14_CHECKSUM_PLUGIN_IID) == "com.romhex14.ChecksumPlugin/1.0", "Plugin IID is valid");
}

// ── Test 2: Fail-Closed Protection on Empty/Invalid Binaries ─────────────────
static void testFailClosedOnInvalidRom() {
    std::cout << "\n=== Test Suite 2: Fail-Closed Protection (Invalid Binaries) ===" << std::endl;
    QByteArray emptyRom;
    QByteArray randomSmall(1024, '\xAA');
    QByteArray random512K(512 * 1024, '\x55');

    QString err;
    TEST_ASSERT(!Checksum::BoschMED17::canHandle(emptyRom), "BoschMED17 rejects empty ROM");
    TEST_ASSERT(!Checksum::BoschMED17::canHandle(randomSmall), "BoschMED17 rejects small random ROM");
    TEST_ASSERT(!Checksum::BoschMED17::canHandle(random512K), "BoschMED17 rejects unsigned 512K random data");
}

// ── Test 4: Transactional Patch Generation Diff Logic ─────────────────────────
static void testPatchDiffCalculation() {
    std::cout << "\n=== Test Suite 4: Transactional Patch Generation ===" << std::endl;
    QByteArray original(1024, '\x00');
    QByteArray modified = original;

    // Mutate 4 bytes at offset 0x100 and 4 bytes at offset 0x300
    modified[0x100] = '\xDE'; modified[0x101] = '\xAD'; modified[0x102] = '\xBE'; modified[0x103] = '\xEF';
    modified[0x300] = '\x12'; modified[0x301] = '\x34'; modified[0x302] = '\x56'; modified[0x303] = '\x78';

    QVector<QPair<int, QByteArray>> patches;
    int diffStart = -1;
    for (int i = 0; i < original.size(); ++i) {
        if (original[i] != modified[i]) {
            if (diffStart < 0) diffStart = i;
        } else {
            if (diffStart >= 0) {
                patches.append({diffStart, modified.mid(diffStart, i - diffStart)});
                diffStart = -1;
            }
        }
    }
    if (diffStart >= 0) {
        patches.append({diffStart, modified.mid(diffStart, original.size() - diffStart)});
    }

    TEST_ASSERT(patches.size() == 2, "Exactly 2 separate patch spans generated");
    TEST_ASSERT(patches[0].first == 0x100 && patches[0].second.size() == 4, "Patch 1 offset and length match");
    TEST_ASSERT(patches[1].first == 0x300 && patches[1].second.size() == 4, "Patch 2 offset and length match");
}

// ── Test 5: Bosch MED17 & Modular 1024-Bit RSA Arithmetic ──────────────────
#include "checksums/common/RsaMath1024.h"
#include "checksums/common/BleichenbacherForge.h"
#include "checksums/Med17Keys.h"

static void testMed17AndRsaMath() {
    std::cout << "\n=== Test Suite 5: Bosch MED17 & Modular 1024-Bit RSA Math ===" << std::endl;

    // 1. Validate 1024-bit Word Import & Export
    std::array<uint8_t, 128> rawBytes{};
    rawBytes[0] = 0x12; rawBytes[127] = 0xFE;
    auto wordsBe = Checksum::Common::RsaMath1024::importBigEndian(rawBytes.data());
    auto exportedBe = Checksum::Common::RsaMath1024::exportBigEndian(wordsBe);
    TEST_ASSERT(exportedBe[0] == 0x12 && exportedBe[127] == 0xFE, "1024-bit Big-Endian import/export roundtrip matches");

    // 2. Validate Public Key Modulus Availability (140 keys)
    const auto key0 = Checksum::MED17::publicKeyForIndex(0);
    const auto key1 = Checksum::MED17::publicKeyForIndex(1);
    const auto key111 = Checksum::MED17::publicKeyForIndex(111);
    TEST_ASSERT(key0.has_value(), "MED17 Key 0 exists");
    TEST_ASSERT(key1.has_value(), "MED17 Key 1 exists");
    TEST_ASSERT(key111.has_value(), "MED17 Key 111 exists");

    // 3. Test 1024-bit Modular Math (S^3 mod N)
    Checksum::Common::RsaMath1024::Words1024 base{};
    base[0] = 2;
    Checksum::Common::RsaMath1024::Words1024 mod{};
    mod[0] = 1000;
    auto cubed = Checksum::Common::RsaMath1024::modCube(base, mod);
    TEST_ASSERT(cubed[0] == 8, "1024-bit modular exponentiation computes base^3 mod N");

    // 4. Test RSA Signature Formatter & Parser Bounds Safety
    std::array<uint8_t, 128> testSig{};
    auto verifyRes = Checksum::Common::RsaMath1024::verifySignature(testSig.data(), key111->data());
    TEST_ASSERT(verifyRes.status == Checksum::Common::RsaVerifyStatus::InvalidPadding, "Zeroed signature rejected with InvalidPadding");
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "==========================================================" << std::endl;
    std::cout << "romHEX14 Dynamic Checksum Plugin & Engine Regression Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    testAbiVersion();
    testFailClosedOnInvalidRom();
    testPatchDiffCalculation();
    testMed17AndRsaMath();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "Test Summary: " << g_testsPassed << "/" << g_testsRun << " passed." << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_testsPassed == g_testsRun) ? 0 : 1;
}
